#pragma once

#include "player.hpp"
#include "data.hpp"
#include "../ui/cfg.hpp"
#include "../other/memory.hpp"
#include "../other/vector3.h"
#include "Offsets.h"
#include "imgui.h"
#include <vector>
#include <cmath>

extern float g_sw;
extern float g_sh;

namespace aim {

    inline void update() {
        if (!cfg::aim::enable) return;
        if (!data::inMatch.load(std::memory_order_relaxed)) return;

        uint64_t localPlayer = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (!localPlayer) return;

        // Check trigger condition
        if (cfg::aim::trigger_firing) {
            bool isFiring = rpm<bool>(localPlayer + Offsets::Player_IsFiring);
            if (!isFiring) return;
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

        float bestDistSq = fovLimit * fovLimit;
        const PlayerData* bestTarget = nullptr;
        Vector3 targetScreenPos = Vector3::Zero();

        for (const auto& enemy : enemies) {
            if (enemy.Head.z < 0.001f) continue;
            if (enemy.curHP <= 0 && !cfg::aim::target_knock) continue;

            Vector3 aimPos = enemy.Head;
            if (cfg::aim::target_bone == 1 && enemy.Toe.z >= 0.001f) {
                // Chest / Neck estimation: between head and toe
                aimPos.y = enemy.Head.y + (enemy.Toe.y - enemy.Head.y) * 0.22f;
            }

            float dx = aimPos.x - screenCx;
            float dy = aimPos.y - screenCy;
            float distSq = dx * dx + dy * dy;

            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTarget = &enemy;
                targetScreenPos = aimPos;
            }
        }

        if (!bestTarget) return;

        // Vector delta towards enemy
        float deltaX = targetScreenPos.x - screenCx;
        float deltaY = targetScreenPos.y - screenCy;

        // Smooth factor
        float smoothVal = ImClamp(cfg::aim::smooth, 1.0f, 20.0f);
        float stepX = deltaX / smoothVal;
        float stepY = deltaY / smoothVal;

        // Max single-frame step to prevent jitter
        stepX = ImClamp(stepX, -25.0f, 25.0f);
        stepY = ImClamp(stepY, -25.0f, 25.0f);

        // Apply aim to camera rotation in memory (Safe process_vm_write, no hook)
        // Free Fire camera matrix / rotation:
        uint64_t cameraFollow = rpm<uint64_t>(localPlayer + Offsets::Player_Camera);
        if (cameraFollow) {
            // Write to Player Rotation if available
            uint64_t rotAddr = localPlayer + Offsets::Player_Rotation;
            if (rotAddr > 0x1000) {
                Vector3 curRot = rpm<Vector3>(rotAddr);
                // Adjust yaw and pitch based on delta
                curRot.x += stepY * 0.08f; // Pitch
                curRot.y += stepX * 0.08f; // Yaw
                wpm<Vector3>(rotAddr, curRot);
            }
        }
    }

    inline void render_fov(ImDrawList* dl) {
        if (!cfg::aim::enable || !cfg::aim::show_fov) return;
        if (!data::inMatch.load(std::memory_order_relaxed)) return;

        float cx = g_sw * 0.5f;
        float cy = g_sh * 0.5f;
        float r  = cfg::aim::fov_radius;

        dl->AddCircle(ImVec2(cx, cy), r, IM_COL32(255, 255, 255, 80), 64, 1.0f);
        dl->AddCircle(ImVec2(cx, cy), r + 1.0f, IM_COL32(255, 60, 60, 160), 64, 1.2f);
    }
}
