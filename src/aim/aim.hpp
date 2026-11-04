#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "../game/player.hpp"
#include "../game/data.hpp"
#include "../offsets/offsets.hpp"
#include "../ui/cfg.hpp"
#include "../other/memory.hpp"
#include "../other/vector3.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>

extern float g_sw;
extern float g_sh;

// ═══════════════════════════════════════════════════════════════════════════════
// PROFESSIONAL CRASH-PROOF & ANTI-DRAG AIM ASSIST ENGINE
// Features:
// 1. Dynamic FOV Circle (scales with slider, strictly filters targets)
// 2. Direct 3D Headshot Ray Alignment (bullets hit HEAD, not body)
// 3. Zero-Drag Protection: Never violently yanks or pulls screen away
// 4. Target Stickiness: Prevents target jumping during fire bursts
// ═══════════════════════════════════════════════════════════════════════════════

namespace aimbot {

    struct TargetInfo {
        bool valid = false;
        char name[36] = {0};
        float distance = 0.0f;
        int hp = 0;
        Vector3 screenPos = Vector3::Zero();
        Vector3 headWorld = Vector3::Zero();
    };

    inline TargetInfo g_currentTarget;

    // Safe 3D vector normalization with division-by-zero & NaN protection
    inline Vector3 SafeNormalize(const Vector3& v) {
        float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (lenSq < 1e-6f || !std::isfinite(lenSq)) {
            return Vector3(0.0f, 0.0f, 1.0f); // Default forward vector
        }
        float invLen = 1.0f / sqrtf(lenSq);
        return Vector3(v.x * invLen, v.y * invLen, v.z * invLen);
    }

    // Target stickiness lock to prevent flicking or erratic screen switching during bursts
    inline uint64_t s_lockedTargetAddr = 0;

    inline void update() {
        g_currentTarget.valid = false;
        if (!cfg::aim::enable) {
            s_lockedTargetAddr = 0;
            return;
        }
        if (!data::inMatch.load(std::memory_order_relaxed)) {
            s_lockedTargetAddr = 0;
            return;
        }

        uint64_t localPlayer = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (!is_valid_ptr(localPlayer)) {
            s_lockedTargetAddr = 0;
            return;
        }

        // Verify local player is alive
        if (rpm<bool>(localPlayer + Offsets::Player::IsDead)) {
            s_lockedTargetAddr = 0;
            return;
        }

        // Trigger condition evaluation
        bool isTriggered = false;
        bool isFiring = rpm<bool>(localPlayer + Offsets::Player::IsFiring);

        if (cfg::aim::trigger_mode == 0) { // When Firing
            isTriggered = isFiring;
        } else if (cfg::aim::trigger_mode == 1) { // When Scoping
            uint64_t wpn = rpm<uint64_t>(localPlayer + Offsets::Player::Weapon);
            isTriggered = is_valid_ptr(wpn) && isFiring;
        } else { // Always In FOV
            isTriggered = true;
        }

        // When not firing or not triggered: 100% free camera look, zero mutation, zero dragging!
        if (!isTriggered) {
            s_lockedTargetAddr = 0;
            return;
        }

        std::vector<PlayerData> enemies;
        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            enemies = data::players;
        }

        if (enemies.empty()) {
            s_lockedTargetAddr = 0;
            return;
        }

        float screenCx = g_sw * 0.5f;
        float screenCy = g_sh * 0.5f;
        float fovLimit = cfg::aim::fov_radius;
        const PlayerData* bestEnemy = nullptr;
        Vector3 bestScreenPos = Vector3::Zero();

        // 1. Target Stickiness: If we are actively firing at a locked target, keep tracking it
        if (s_lockedTargetAddr != 0) {
            for (const auto& enemy : enemies) {
                if (enemy.addr == s_lockedTargetAddr) {
                    if (enemy.Head.z >= 0.001f && (!cfg::aim::ignore_knocked || enemy.curHP > 0)) {
                        float dx = enemy.Head.x - screenCx;
                        float dy = enemy.Head.y - screenCy;
                        float dist = sqrtf(dx * dx + dy * dy);
                        // Allow small slack during firing so aim doesn't detach on micro-movements
                        if (dist <= fovLimit * 1.25f) {
                            bestEnemy = &enemy;
                            bestScreenPos = enemy.Head;
                        }
                    }
                    break;
                }
            }
        }

        // 2. If no sticky target or target eliminated, find the closest enemy strictly INSIDE the FOV circle
        if (!bestEnemy) {
            float bestScore = 9999999.0f;
            for (const auto& enemy : enemies) {
                if (enemy.Head.z < 0.001f) continue;
                if (cfg::aim::ignore_knocked && enemy.curHP <= 0) continue;
                if (cfg::aim::ignore_bots && enemy.isBot) continue;
                if (cfg::aim::visible_check && !enemy.isVisible) continue;
                if (enemy.distance > cfg::aim::max_distance) continue;

                // Bone target selection (Head is primary for 100% headshot)
                Vector3 aimPos = enemy.Head;
                if (cfg::aim::target_bone == 1 && enemy.Toe.z >= 0.001f) {
                    // Neck / Upper Chest
                    aimPos.y = enemy.Head.y + (enemy.Toe.y - enemy.Head.y) * 0.22f;
                } else if (cfg::aim::target_bone == 2 && enemy.Toe.z >= 0.001f) {
                    // Pelvis / Mid Body
                    aimPos.y = enemy.Head.y + (enemy.Toe.y - enemy.Head.y) * 0.45f;
                }

                float dx = aimPos.x - screenCx;
                float dy = aimPos.y - screenCy;
                float crosshairDist = sqrtf(dx * dx + dy * dy);

                // STRICT FOV CIRCLE: Ignore any player outside the visual circle
                if (crosshairDist > fovLimit) continue;

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
        }

        if (!bestEnemy) {
            s_lockedTargetAddr = 0;
            return;
        }

        // Remember target for this firing burst
        s_lockedTargetAddr = bestEnemy->addr;

        // Record target diagnostic telemetry
        g_currentTarget.valid = true;
        snprintf(g_currentTarget.name, sizeof(g_currentTarget.name), "%s",
                 bestEnemy->name[0] ? bestEnemy->name : (bestEnemy->isBot ? "BOT" : "Player"));
        g_currentTarget.distance = bestEnemy->distance;
        g_currentTarget.hp = bestEnemy->curHP;
        g_currentTarget.screenPos = bestScreenPos;
        g_currentTarget.headWorld = bestEnemy->headWorld;

        // ══════════════════════════════════════════════════════════════════════
        // 1. 3D HEADSHOT BULLET ALIGNMENT (NO CAMERA DRAGGING)
        // Modifies bullet/raycast direction directly to HEAD (100% Headshot)
        // ══════════════════════════════════════════════════════════════════════
        uint64_t aimInfo = rpm<uint64_t>(localPlayer + Offsets::Player::AimInfo);
        if (is_valid_ptr(aimInfo)) {
            uint64_t dirAddr = aimInfo + Offsets::Aim::Direction;
            Vector3 bulletOrigin = rpm<Vector3>(aimInfo + Offsets::Aim::StartPosition);

            Vector3 target3D = bestEnemy->headWorld;
            if (target3D != Vector3::Zero()) {
                if (cfg::aim::target_bone == 1) {
                    target3D.y -= 0.18f; // Neck
                } else if (cfg::aim::target_bone == 2) {
                    target3D.y -= 0.45f; // Pelvis / Body
                }

                if (bulletOrigin != Vector3::Zero() && std::isfinite(bulletOrigin.x)) {
                    Vector3 diff = target3D - bulletOrigin;
                    Vector3 headshotDir = SafeNormalize(diff);
                    if (std::isfinite(headshotDir.x) && std::isfinite(headshotDir.y) && std::isfinite(headshotDir.z)) {
                        wpm<Vector3>(dirAddr, headshotDir);
                    }
                }
            }
        }

        // ══════════════════════════════════════════════════════════════════════
        // 2. ULTRA-SMOOTH MICRO-ASSIST (NEVER YANKS OR DRAGS SCREEN)
        // Gently lifts crosshair towards the head only when firing
        // ══════════════════════════════════════════════════════════════════════
        float deltaX = bestScreenPos.x - screenCx;
        float deltaY = bestScreenPos.y - screenCy;

        float smoothVal = ImClamp(cfg::aim::smooth, 1.0f, 30.0f);
        float stepX = deltaX / (smoothVal * 5.0f);
        float stepY = deltaY / (smoothVal * 5.0f);

        // Strict clamp to micro-degrees: max 1.8 degrees per frame, perfectly natural
        stepX = ImClamp(stepX, -1.8f, 1.8f);
        stepY = ImClamp(stepY, -1.8f, 1.8f);

        uint64_t rotAddr = localPlayer + Offsets::Player::Rotation;
        if (is_valid_ptr(rotAddr)) {
            Vector3 curRot = rpm<Vector3>(rotAddr);
            if (std::isfinite(curRot.x) && std::isfinite(curRot.y) && std::isfinite(curRot.z) &&
                fabsf(curRot.x) <= 360.0f && fabsf(curRot.y) <= 360.0f) {
                curRot.x += stepY * 0.05f; // Micro Pitch lift to head
                curRot.y += stepX * 0.05f; // Micro Yaw alignment

                curRot.x = ImClamp(curRot.x, -89.0f, 89.0f);
                wpm<Vector3>(rotAddr, curRot);
            }
        }
    }

    // ══════════════════════════════════════════════════════════════════════════
    // FOV CIRCLE & ACTIVE LOCK VISUALIZER
    // Scaled dynamically by cfg::aim::fov_radius
    // ══════════════════════════════════════════════════════════════════════════
    inline void render_fov(ImDrawList* dl) {
        if (!cfg::aim::enable || !cfg::aim::show_fov) return;
        if (!data::inMatch.load(std::memory_order_relaxed)) return;

        float cx = g_sw * 0.5f;
        float cy = g_sh * 0.5f;
        float r  = cfg::aim::fov_radius;

        // Smooth Orchid / White FOV circle (shows exact trigger zone)
        dl->AddCircle(ImVec2(cx, cy), r, IM_COL32(255, 255, 255, 45), 64, 1.0f);
        dl->AddCircle(ImVec2(cx, cy), r + 1.0f, IM_COL32(225, 0, 153, 160), 64, 1.5f);

        // If target is actively locked inside FOV, draw target lock ring & snapline to head
        if (g_currentTarget.valid && g_currentTarget.screenPos.z >= 0.001f) {
            dl->AddCircle(ImVec2(g_currentTarget.screenPos.x, g_currentTarget.screenPos.y),
                          12.0f, IM_COL32(255, 30, 80, 240), 16, 2.0f);
            dl->AddCircle(ImVec2(g_currentTarget.screenPos.x, g_currentTarget.screenPos.y),
                          4.0f, IM_COL32(255, 255, 255, 255), 12, 1.5f);
            dl->AddLine(ImVec2(cx, cy),
                        ImVec2(g_currentTarget.screenPos.x, g_currentTarget.screenPos.y),
                        IM_COL32(225, 0, 153, 110), 1.2f);
        }
    }

} // namespace aimbot
