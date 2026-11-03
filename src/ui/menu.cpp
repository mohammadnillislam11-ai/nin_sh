#include "menu.hpp"
#include "imgui.h"
#include "Android_draw/draw.h"
#include "Android_touch/Touch.hpp"
#include "func/esp.hpp"
#include "cfg.hpp"
#include "game/data.hpp"
#include "encrypt.h"
#include <string>
#include <algorithm>

using namespace ImGui;

extern bool main_thread_flag;
static int activeTab = 0;
static float ui_opacity = 1.0f;

// Floating icons toggles (Brutal UI feature)
static bool showFloatingIcons = false;
static bool floatAim = false;
static bool floatESP = false;

// Custom Brutal Toggle Switch
static bool ToggleSwitch(const char* id, bool* v)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const float height = 38.0f;   
    const float width  = 85.0f;  
    const float radius = height * 0.5f;

    ImGui::InvisibleButton(id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
        *v = !(*v);

    ImU32 col_bg = *v ? IM_COL32(0, 255, 150, 255) : IM_COL32(45, 45, 45, 255);

    if (*v)
    {
        draw->AddRectFilled(
            ImVec2(p.x - 2, p.y - 2),
            ImVec2(p.x + width + 2, p.y + height + 2),
            IM_COL32(0, 255, 150, 60),
            height * 0.6f);
    }

    draw->AddRectFilled(
        p,
        ImVec2(p.x + width, p.y + height),
        col_bg,
        height * 0.6f);

    float t = *v ? (width - radius) : radius;

    draw->AddCircleFilled(
        ImVec2(p.x + t, p.y + radius),
        radius - 3.0f,
        IM_COL32(255, 255, 255, 255));

    return *v;
}

namespace ui::menu {

    void render() {
        // Draw ESP overlay on background
        func::esp::draw();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::GetStyle().Alpha = ui_opacity;

        float realW = (float)displayInfo.width;
        float realH = (float)displayInfo.height;
        if (realW <= 0) realW = 1920.0f;
        if (realH <= 0) realH = 1080.0f;

        ImVec2 panelSize = ImVec2(560, 480);
        ImGui::SetNextWindowSize(panelSize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(
            ImVec2(realW * 0.5f, realH * 0.5f),
            ImGuiCond_FirstUseEver,
            ImVec2(0.5f, 0.5f)
        );

        if (ImGui::Begin(ENC_STR("ninmods | VIP External Panel"), nullptr, ImGuiWindowFlags_None)) 
        {
            ImGui::BeginChild("##TabBar", ImVec2(0, 50), false);
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));

            float padding = 8.0f;
            float totalWidth = ImGui::GetContentRegionAvail().x - (padding * 2);
            float tabWidth = totalWidth / 4.0f; 

            ImGui::SetCursorPosX(padding);
            if (ImGui::Button("AIMBOT", ImVec2(tabWidth - 4.0f, 40))) activeTab = 0;
            ImGui::SameLine();
            if (ImGui::Button("ESP", ImVec2(tabWidth - 4.0f, 40))) activeTab = 1;
            ImGui::SameLine();
            if (ImGui::Button("SETTINGS", ImVec2(tabWidth - 4.0f, 40))) activeTab = 2;
            ImGui::SameLine();
            if (ImGui::Button("ABOUT", ImVec2(tabWidth - 4.0f, 40))) activeTab = 3;

            ImGui::PopStyleVar(2);
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 8));

            // TAB 0: AIMBOT
            if (activeTab == 0) {
                ImGui::SeparatorText("Aimbot Configuration");
                ImGui::Dummy(ImVec2(0, 5));

                ImGui::Checkbox("ENABLE AIM LOCK", &cfg::aim::enable);
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::Checkbox("SHOW FOV CIRCLE", &cfg::aim::show_fov);
                ImGui::Checkbox("AIM ONLY ON FIRE", &cfg::aim::trigger_firing);
                ImGui::Checkbox("TARGET KNOCKED", &cfg::aim::target_knock);

                ImGui::Dummy(ImVec2(0, 5));
                ImGui::Text("TARGET BONE");
                const char* bones[] = { "Head", "Neck / Chest" };
                ImGui::SetNextItemWidth(250);
                ImGui::Combo("##AimBone", &cfg::aim::target_bone, bones, 2);

                ImGui::Dummy(ImVec2(0, 5));
                ImGui::SliderFloat("FOV Radius", &cfg::aim::fov_radius, 30.0f, 400.0f, "%.0f px");
                ImGui::SliderFloat("Smoothness", &cfg::aim::smooth, 1.0f, 20.0f, "%.1f");
            }
            // TAB 1: VISUALS / ESP
            else if (activeTab == 1) {
                ImGui::SeparatorText("Visuals / ESP");
                ImGui::Dummy(ImVec2(0, 5));

                ImGui::Checkbox("ESP LINE (TRACER)", &cfg::esp::tracer);
                ImGui::Checkbox("ESP BOX", &cfg::esp::box);
                ImGui::Checkbox("ESP HEALTH", &cfg::esp::health);
                ImGui::Checkbox("ESP ARMOR", &cfg::esp::armor);
                ImGui::Checkbox("ESP NAME", &cfg::esp::name);
                ImGui::Checkbox("ESP DISTANCE", &cfg::esp::distance);
                ImGui::Checkbox("ESP WEAPON", &cfg::esp::weapon);
                ImGui::Checkbox("ESP BOT TAG", &cfg::esp::bot_tag);
                ImGui::Checkbox("CROSSHAIR", &cfg::esp::crosshair);
                ImGui::Checkbox("FIRING INDICATOR", &cfg::esp::firing_ind);
                ImGui::Checkbox("LOOT ESP", &cfg::esp::loot);

                ImGui::Dummy(ImVec2(0, 6));
                const char* boxTypes[] = { "Corner", "Full", "Glow" };
                ImGui::SetNextItemWidth(250);
                ImGui::Combo("Box Style", &cfg::esp::box_type, boxTypes, 3);
                ImGui::SliderFloat("Max Range", &cfg::esp::max_range, 50.0f, 500.0f, "%.0f m");
            }
            // TAB 2: CONFIGURATION & SETTINGS
            else if (activeTab == 2) {
                ImGui::SeparatorText("Configuration & Targets");
                ImGui::Dummy(ImVec2(0, 5));

                if (ImGui::Checkbox("Floating Quick Icons", &showFloatingIcons)) {
                    floatAim = showFloatingIcons;
                    floatESP = showFloatingIcons;
                }

                ImGui::Dummy(ImVec2(0, 5));
                if (ImGui::SliderFloat("UI Opacity", &ui_opacity, 0.2f, 1.0f, "%.2f")) {
                    ImGui::GetStyle().Alpha = ui_opacity;
                }

                ImGui::Dummy(ImVec2(0, 10));
                ImGui::Text("System Status:");
                ImGui::TextColored(proc::lib ? ImVec4(0,1,0,1) : ImVec4(1,1,0,1),
                    proc::lib ? "Game Hooked (libil2cpp: 0x%lx)" : "Waiting for Game...",
                    (unsigned long)proc::lib);
                ImGui::Text("Target PID: %d", proc::pid);
                ImGui::Text("FPS: %.1f", io.Framerate);
            }
            // TAB 3: ABOUT & EXIT
            else if (activeTab == 3) {
                ImGui::SeparatorText("About & Controls");
                ImGui::Dummy(ImVec2(0, 5));

                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "ninmods External VIP Panel");
                ImGui::Text("Engine: Safe Kernel Memory / External DMA Read");
                ImGui::Text("Architecture: 100% Non-Invasive (No In-game Lib Injected)");
                ImGui::Text("Developer: @Zelewin");

                ImGui::Dummy(ImVec2(0, 20)); 
                if (ImGui::Button(" EXIT PANEL ", ImVec2(ImGui::GetContentRegionAvail().x, 45))) {
                    main_thread_flag = false;
                    Touch_Close();
                    data::thread_stop();
                    shutdown();
                    exit(0);
                }
            }
        }
        ImGui::End();

        // Brutal-style Floating Quick Toggle Windows
        if (showFloatingIcons)
        {
            const ImVec2 winSize = ImVec2(200, 105); 

            auto DrawHeader = [&](const char* title, bool* visible) {
                ImGui::Text("%s", title);
                ImGui::SameLine(ImGui::GetWindowWidth() - 25);
                if (ImGui::Selectable("x", false, 0, ImVec2(14, 14))) {
                    *visible = false;
                }
                ImGui::Separator();
            };

            if (floatAim) {
                ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
                ImGui::SetNextWindowPos(ImVec2(60, 240), ImGuiCond_FirstUseEver);
                ImGui::Begin("##FloatAim", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                DrawHeader(" AIM LOCK ", &floatAim);
                ToggleSwitch("##aimFloat", &cfg::aim::enable);
                ImGui::End();
            }
            
            if (floatESP) {
                ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
                ImGui::SetNextWindowPos(ImVec2(60, 360), ImGuiCond_FirstUseEver);
                ImGui::Begin("##FloatESP", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                DrawHeader(" ESP TRACER ", &floatESP);
                ToggleSwitch("##espFloat", &cfg::esp::tracer);
                ImGui::End();
            }
        }
    }
}
