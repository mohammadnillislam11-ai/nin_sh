#include "bar.hpp"
#include "styles/styles.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>
#include <cstdio>

namespace ui::bar {
    using namespace style;

    static float lrp(float a, float b, float t) { return a + (b - a) * t; }
    static float ease(float t) { return -(cosf(3.14159265f * t) - 1.f) / 2.f; }

    void set_game_alpha(float a) { g_game_alpha = a; }
    float game_alpha() { return g_game_alpha; }

    // ============================================
    // ICON FPS / FLOATING LOGO
    // ============================================
    static float  g_fps_x = -1.f;
    static float  g_fps_y = -1.f;
    static bool   g_fps_drag = false;
    static bool   g_fps_potential = false;
    static ImVec2 g_fps_off(0, 0);
    static ImVec2 g_fps_down(0, 0);
    static constexpr float FPS_DRAG_THRESHOLD = 10.f;

    static void get_fps_pos(float w, float h) {
        if (g_fps_x < 0.f || g_fps_y < 0.f) {
            g_fps_x = (g_sw > 0 ? g_sw - w - 25.f : 100.f);
            g_fps_y = (g_sh > 0 ? g_sh - h - 25.f : 100.f);
        }
        if (g_sw > 0) g_fps_x = ImMax(10.f, ImMin(g_fps_x, g_sw - w - 10.f));
        if (g_sh > 0) g_fps_y = ImMax(10.f, ImMin(g_fps_y, g_sh - h - 10.f));
    }

    static void render_fps_icon(float alpha) {
        const float w = 105.f, h = 38.f;
        get_fps_pos(w, h);

        ImGui::SetNextWindowPos(ImVec2(g_fps_x, g_fps_y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0,0,0,0));

        ImGui::Begin("##FloatingLogo", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoSavedSettings);

        ImVec2 wp = ImGui::GetWindowPos();
        ImGui::InvisibleButton("##fps_drag", ImVec2(w, h));
        bool active = ImGui::IsItemActive();

        if (active && ImGui::IsMouseDragging(0, 3.0f)) {
            g_fps_drag = true;
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            g_fps_x += delta.x;
            g_fps_y += delta.y;
            get_fps_pos(w, h);
        }

        if (ImGui::IsItemDeactivated()) {
            if (!g_fps_drag) {
                g_open = true;
            }
            g_fps_drag = false;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        float fps = ImGui::GetIO().Framerate;

        ImU32 bg_col     = IM_COL32(12, 12, 16, (int)(240 * alpha));
        ImU32 border_col = (active || g_fps_drag) ? col(UITheme::accent_light, alpha)
                                                 : col(UITheme::border, alpha);

        // Background pill
        float pillR = h * 0.5f;
        dl->AddRectFilled({wp.x + 2, wp.y + 2}, {wp.x + w + 2, wp.y + h + 2},
                          IM_COL32(0, 0, 0, (int)(70 * alpha)), pillR);
        dl->AddRectFilled(wp, {wp.x + w, wp.y + h}, bg_col, pillR);
        dl->AddRect(wp, {wp.x + w, wp.y + h}, border_col, pillR, 0, g_fps_drag ? 1.8f : 1.0f);

        // Accent strip left
        dl->AddRectFilled({wp.x + 4.f, wp.y + 5.f},
                          {wp.x + 6.5f, wp.y + h - 5.f},
                          col(UITheme::accent, alpha), 1.5f);

        // Vector 'N' Logo
        float cx = wp.x + 20.f, cy = wp.y + h * 0.5f, radius = 12.f;
        UITheme::DrawNLogo(dl, ImVec2(cx, cy), radius, alpha);

        // FPS Text
        int fpsInt = (int)(fps + 0.5f);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d FPS", fpsInt);

        ImFont* font = ImGui::GetFont();
        if (font) {
            ImU32 text_col = fps >= 50.f ? IM_COL32(52, 199, 89, 255)
                            : fps >= 30.f ? IM_COL32(255, 180, 40, 255)
                                          : IM_COL32(255, 69, 58, 255);
            dl->AddText(font, 13.f,
                        {wp.x + 38.f, wp.y + (h - 13.f) * 0.5f},
                        text_col, buf);
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
    }

    // ============================================
    // RENDER — icon FPS hanya saat menu tertutup
    // ============================================
    void render() {
        static float prog = 0.f;
        float dt = ImGui::GetIO().DeltaTime;

        float tgt = g_open ? 1.f : 0.f;
        float spd = 12.f * dt;
        if (prog < tgt) prog = ImMin(prog + spd, tgt);
        else if (prog > tgt) prog = ImMax(prog - spd, tgt);
        prog = ImClamp(prog, 0.f, 1.f);

        g_alpha = ease(prog);

        if (g_game_alpha > 0.001f && !g_open) {
            render_fps_icon(1.f);
        }
    }
}
