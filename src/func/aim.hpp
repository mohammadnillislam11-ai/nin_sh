#pragma once

#include "../game/player.hpp"
#include "../game/data.hpp"
#include "../game/Offsets.h"
#include "../ui/cfg.hpp"
#include "../other/memory.hpp"
#include "../other/vector3.h"
#include "imgui.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>

extern float g_sw;
extern float g_sh;

namespace func::aim {

    struct TargetInfo {
        bool valid = false;
        char name[36] = {0};
        float distance = 0.0f;
        int hp = 0;
        Vector3 screenPos = Vector3::Zero();
    };

    inline TargetInfo g_currentTarget;

    inline void update() {
        g_currentTarget.valid = false;
        if (!cfg::aim::enable) return;
        if (!data::inMatch.load(std::memory_order_relaxed)) return;

        uint64_t localPlayer = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (!localPlayer || localPlayer < 0x10000000) return;

        // Trigger condition check
        if (cfg::aim::trigger_mode == 0) { // When Firing
            bool isFiring = rpm<bool>(localPlayer + Offsets::Player_IsFiring);
            if (!isFiring) return;
        } else if (cfg::aim::trigger_mode == 1) { // When Scoping
            uint64_t wpn = rpm<uint64_t>(localPlayer + Offsets::Player_Weapon);
            if (!wpn) return;
        }

        std::vector<PlayerData> enemies;
        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            enemies = data::players;
        }

        if (enemies.empty()) return;

        float screenCx = g_sw * 0.5f;
        float screenCy = g_sh * 0.5f;
        float fovLimit = cfg::aim::fov_radius;
        float bestScore = 9999999.0f;
        const PlayerData* bestEnemy = nullptr;
        Vector3 bestScreenPos = Vector3::Zero();

        for (const auto& enemy : enemies) {
            if (enemy.Head.z < 0.001f) continue;
            if (cfg::aim::ignore_knocked && enemy.curHP <= 0) continue;
            if (cfg::aim::ignore_bots && enemy.isBot) continue;
            if (cfg::aim::visible_check && !enemy.isVisible) continue;
            if (enemy.distance > cfg::aim::max_distance) continue;

            // Target bone selection
            Vector3 aimPos = enemy.Head;
            if (cfg::aim::target_bone == 1 && enemy.Toe.z >= 0.001f) {
                // Neck / Upper Chest
                aimPos.y = enemy.Head.y + (enemy.Toe.y - enemy.Head.y) * 0.22f;
            } else if (cfg::aim::target_bone == 2 && enemy.Toe.z >= 0.001f) {
                // Pelvis / Mid Body
                aimPos.y = enemy.Head.y + (enemy.Toe.y - enemy.Head.y) * 0.45f;
            }

            // Calculate distance to crosshair in 2D screen space
            float dx = aimPos.x - screenCx;
            float dy = aimPos.y - screenCy;
            float crosshairDist = sqrtf(dx * dx + dy * dy);

            if (crosshairDist > fovLimit) continue;

            // Priority mode evaluation
            float score = crosshairDist;
            if (cfg::aim::priority_mode == 1) { // Lowest HP
                score = (float)enemy.curHP;
            } else if (cfg::aim::priority_mode == 2) { // 3D Distance
                score = enemy.distance;
            }

            if (score < bestScore) {
                bestScore = score;
                bestEnemy = &enemy;
                bestScreenPos = aimPos;
            }
        }

        if (!bestEnemy) return;

        // Record target diagnostic telemetry
        g_currentTarget.valid = true;
        snprintf(g_currentTarget.name, sizeof(g_currentTarget.name), "%s",
                 bestEnemy->name[0] ? bestEnemy->name : (bestEnemy->isBot ? "BOT" : "Player"));
        g_currentTarget.distance = bestEnemy->distance;
        g_currentTarget.hp = bestEnemy->curHP;
        g_currentTarget.screenPos = bestScreenPos;

        // Calculate delta vector towards target
        float deltaX = bestScreenPos.x - screenCx;
        float deltaY = bestScreenPos.y - screenCy;

        // Smooth factor (1.0 = instant snap, 5.0 to 20.0 = human-like smooth glide)
        float smoothVal = ImClamp(cfg::aim::smooth, 1.0f, 30.0f);
        float stepX = deltaX / smoothVal;
        float stepY = deltaY / smoothVal;

        // Clamp step to prevent sudden sharp jumps
        stepX = ImClamp(stepX, -25.0f, 25.0f);
        stepY = ImClamp(stepY, -25.0f, 25.0f);

        // Safe aim adjustment via Player rotation fields
        uint64_t rotAddr = localPlayer + Offsets::Player_Rotation;
        if (rotAddr > 0x10000000) {
            Vector3 curRot = rpm<Vector3>(rotAddr);
            if (std::isfinite(curRot.x) && std::isfinite(curRot.y)) {
                curRot.x += stepY * 0.05f; // Pitch
                curRot.y += stepX * 0.05f; // Yaw
                wpm<Vector3>(rotAddr, curRot);
            }
        }

        // AimInfo Direction vector alignment for projectile line
        uint64_t aimInfo = rpm<uint64_t>(localPlayer + Offsets::Player_AimInfo);
        if (aimInfo > 0x10000000) {
            Vector3 aimDir = rpm<Vector3>(aimInfo + Offsets::AimInfo_Direction);
            if (std::isfinite(aimDir.x) && std::isfinite(aimDir.y) && std::isfinite(aimDir.z)) {
                aimDir.x += (stepX * 0.002f);
                aimDir.y -= (stepY * 0.002f);
                aimDir = Vector3::Normalize(aimDir);
                wpm<Vector3>(aimInfo + Offsets::AimInfo_Direction, aimDir);
            }
        }
    }

    inline void render_fov(ImDrawList* dl) {
        if (!cfg::aim::enable || !cfg::aim::show_fov) return;
        if (!data::inMatch.load(std::memory_order_relaxed)) return;

        float cx = g_sw * 0.5f;
        float cy = g_sh * 0.5f;
        float r  = cfg::aim::fov_radius;

        // Smooth Orchid / White FOV circle
        dl->AddCircle(ImVec2(cx, cy), r, IM_COL32(255, 255, 255, 45), 64, 1.0f);
        dl->AddCircle(ImVec2(cx, cy), r + 1.0f, IM_COL32(225, 0, 153, 160), 64, 1.5f);

        // If target is actively locked inside FOV, draw target lock ring & line
        if (g_currentTarget.valid && g_currentTarget.screenPos.z >= 0.001f) {
            dl->AddCircle(ImVec2(g_currentTarget.screenPos.x, g_currentTarget.screenPos.y),
                          10.0f, IM_COL32(255, 40, 80, 220), 16, 2.0f);
            dl->AddLine(ImVec2(cx, cy),
                        ImVec2(g_currentTarget.screenPos.x, g_currentTarget.screenPos.y),
                        IM_COL32(225, 0, 153, 90), 1.0f);
        }
    }

} // namespace func::aim
