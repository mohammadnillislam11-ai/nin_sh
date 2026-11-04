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
            float tabWidth = totalWidth / 3.0f; 

            ImGui::SetCursorPosX(padding);
            if (ImGui::Button("VISUALS / ESP", ImVec2(tabWidth - 4.0f, 40))) activeTab = 0;
            ImGui::SameLine();
            if (ImGui::Button("SETTINGS", ImVec2(tabWidth - 4.0f, 40))) activeTab = 1;
            ImGui::SameLine();
            if (ImGui::Button("ABOUT", ImVec2(tabWidth - 4.0f, 40))) activeTab = 2;

            ImGui::PopStyleVar(2);
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 8));

            // TAB 0: VISUALS / ESP
            if (activeTab == 0) {
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
            // TAB 1: CONFIGURATION & SETTINGS
            else if (activeTab == 1) {
                ImGui::SeparatorText("Configuration & Targets");
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
            // TAB 2: ABOUT & EXIT
            else if (activeTab == 2) {
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
    }
}
