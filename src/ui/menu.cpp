#include "menu.hpp"
#include "imgui.h"
#include "Android_draw/draw.h"
#include "Android_touch/Touch.hpp"
#include "func/esp.hpp"
#include "cfg.hpp"
#include "game/data.hpp"
#include "encrypt.h"
#include <string>
#include <cmath>
#include <algorithm>

using namespace ImGui;

extern bool main_thread_flag;
static int activeTab = 0;
static float ui_opacity = 1.0f;
static bool menu_open = true;

// ═══════════════════════════════════════════════════════════════════════════════
// DESIGN 08: ELECTRIC ORCHID GLASS — EXACT COLOR DEFINITIONS
// ═══════════════════════════════════════════════════════════════════════════════
namespace theme_orchid {
    constexpr ImU32 ColPrimary       = IM_COL32(225, 0, 152, 255);       // Electric Orchid #E10098
    constexpr ImU32 ColPrimaryLight  = IM_COL32(255, 90, 205, 255);      // Soft Orchid Glow #FF5ACD
    constexpr ImU32 ColPrimaryMuted  = IM_COL32(225, 0, 152, 60);        // Translucent Orchid
    constexpr ImU32 ColBgDark        = IM_COL32(14, 12, 20, 250);        // Deep Glass Navy #0E0C14
    constexpr ImU32 ColHeaderBg      = IM_COL32(21, 16, 31, 255);        // Header #15101F
    constexpr ImU32 ColSidebarBg     = IM_COL32(17, 14, 24, 255);        // Sidebar #110E18
    constexpr ImU32 ColCardBg        = IM_COL32(22, 17, 33, 230);        // Card Panel #161121
    constexpr ImU32 ColBorder        = IM_COL32(225, 0, 152, 180);       // Window Border Glow
    constexpr ImU32 ColCardBorder    = IM_COL32(50, 36, 72, 255);        // Card Subtle Border #322448
    constexpr ImU32 ColTextMuted     = IM_COL32(155, 140, 180, 255);     // Muted Text #9B8CB4
    constexpr ImU32 ColWhite         = IM_COL32(252, 248, 255, 255);     // Clean White
}

// ═══════════════════════════════════════════════════════════════════════════════
// VECTOR ICONS (PRECISE GEOMETRY & CLEAN ARCS)
// ═══════════════════════════════════════════════════════════════════════════════
enum EIconType {
    ICON_EYE = 0,
    ICON_CROSSHAIR,
    ICON_SETTINGS,
    ICON_POWER
};

static void DrawLogoFlame(ImDrawList* dl, ImVec2 center, float size, ImU32 col) {
    float hs = size * 0.5f;
    ImVec2 pts[6] = {
        ImVec2(center.x, center.y - hs),
        ImVec2(center.x + hs * 0.90f, center.y - hs * 0.30f),
        ImVec2(center.x + hs * 0.70f, center.y + hs * 0.60f),
        ImVec2(center.x, center.y + hs),
        ImVec2(center.x - hs * 0.70f, center.y + hs * 0.60f),
        ImVec2(center.x - hs * 0.90f, center.y - hs * 0.30f)
    };
    dl->AddConvexPolyFilled(pts, 6, col);
    dl->AddPolyline(pts, 6, IM_COL32(255, 255, 255, 220), ImDrawFlags_Closed, 1.2f);
}

static void DrawVectorIcon(ImDrawList* dl, EIconType type, ImVec2 center, float size, ImU32 col) {
    float hs = size * 0.5f;
    float cx = center.x;
    float cy = center.y;

    switch (type) {
        case ICON_EYE: {
            // ESP Eye contour
            dl->AddBezierQuadratic(ImVec2(cx - hs, cy), ImVec2(cx, cy - hs * 0.85f), ImVec2(cx + hs, cy), col, 2.0f);
            dl->AddBezierQuadratic(ImVec2(cx - hs, cy), ImVec2(cx, cy + hs * 0.85f), ImVec2(cx + hs, cy), col, 2.0f);
            dl->AddCircleFilled(center, size * 0.22f, col);
            dl->AddCircleFilled(center, size * 0.08f, IM_COL32(14, 12, 20, 255));
            break;
        }
        case ICON_CROSSHAIR: {
            // Combat Crosshair
            dl->AddCircle(center, size * 0.38f, col, 16, 1.8f);
            dl->AddCircleFilled(center, 1.8f, col);
            dl->AddLine(ImVec2(cx, cy - hs), ImVec2(cx, cy - size * 0.22f), col, 1.8f);
            dl->AddLine(ImVec2(cx, cy + size * 0.22f), ImVec2(cx, cy + hs), col, 1.8f);
            dl->AddLine(ImVec2(cx - hs, cy), ImVec2(cx - size * 0.22f, cy), col, 1.8f);
            dl->AddLine(ImVec2(cx + size * 0.22f, cy), ImVec2(cx + hs, cy), col, 1.8f);
            break;
        }
        case ICON_SETTINGS: {
            // Gear icon with radial teeth
            dl->AddCircle(center, size * 0.30f, col, 16, 1.8f);
            dl->AddCircleFilled(center, size * 0.12f, col);
            for (int i = 0; i < 8; ++i) {
                float angle = i * (3.14159265f / 4.0f);
                float x1 = cx + cosf(angle) * (hs * 0.65f);
                float y1 = cy + sinf(angle) * (hs * 0.65f);
                float x2 = cx + cosf(angle) * hs;
                float y2 = cy + sinf(angle) * hs;
                dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), col, 2.0f);
            }
            break;
        }
        case ICON_POWER: {
            // Authentic Power Arc with gap at the top (no clear rect needed!)
            float r = size * 0.38f;
            dl->PathClear();
            dl->PathArcTo(center, r, -2.10f, 5.24f, 20); // ~ -120 deg to ~ 300 deg
            dl->PathStroke(col, 0, 2.0f);
            dl->AddLine(ImVec2(cx, cy - hs), ImVec2(cx, cy), col, 2.0f);
            break;
        }
    }
}

static bool DrawSidebarIconButton(const char* str_id, EIconType icon_type, bool is_active, ImVec2 size) {
    bool pressed = ImGui::InvisibleButton(str_id, size);
    bool hovered = ImGui::IsItemHovered();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);

    if (is_active) {
        // Active button background & subtle border
        dl->AddRectFilled(bMin, bMax, IM_COL32(225, 0, 152, 45), 8.0f);
        dl->AddRect(bMin, bMax, IM_COL32(225, 0, 152, 130), 8.0f, 0, 1.0f);

        // Left vertical indicator pill (crisp 3.5px width)
        dl->AddRectFilled(ImVec2(bMin.x + 2.0f, bMin.y + 7.0f), ImVec2(bMin.x + 5.5f, bMax.y - 7.0f), theme_orchid::ColPrimary, 2.0f);

        // Glowing white icon
        DrawVectorIcon(dl, icon_type, center, 20.0f, theme_orchid::ColWhite);
    } else {
        if (hovered) {
            dl->AddRectFilled(bMin, bMax, IM_COL32(255, 255, 255, 15), 8.0f);
        }
        ImU32 ic_col = hovered ? theme_orchid::ColPrimaryLight : theme_orchid::ColTextMuted;
        DrawVectorIcon(dl, icon_type, center, 18.0f, ic_col);
    }

    return pressed;
}

namespace ui::menu {

    void render() {
        // Always render ESP overlays on the background
        func::esp::draw();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::GetStyle().Alpha = ui_opacity;

        float realW = (float)displayInfo.width;
        float realH = (float)displayInfo.height;
        if (realW <= 0) realW = 1920.0f;
        if (realH <= 0) realH = 1080.0f;

        // ══════════════════════════════════════════════════════════════════════
        // FLOATING RE-OPEN PILL (SHOWN ONLY WHEN MENU IS MINIMIZED)
        // ══════════════════════════════════════════════════════════════════════
        if (!menu_open) {
            ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(136, 44));

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.082f, 0.063f, 0.122f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.882f, 0.000f, 0.596f, 0.90f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 22.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));

            if (ImGui::Begin("##NIN_MINI", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                if (ImGui::InvisibleButton("##ReOpenPill", avail)) {
                    menu_open = true;
                }
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 p0 = ImGui::GetItemRectMin();
                ImVec2 p1 = ImGui::GetItemRectMax();
                float cy = (p0.y + p1.y) * 0.5f;

                DrawLogoFlame(dl, ImVec2(p0.x + 20, cy), 16.0f, theme_orchid::ColPrimary);
                dl->AddText(ImVec2(p0.x + 36, cy - 8), theme_orchid::ColWhite, "NINMODS");
            }
            ImGui::End();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);
            return;
        }

        // ══════════════════════════════════════════════════════════════════════
        // MAIN MENU: DESIGN 08 (ELECTRIC ORCHID GLASS)
        // ══════════════════════════════════════════════════════════════════════
        const float winW = 680.0f;
        const float winH = 430.0f;
        ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(
            ImVec2(realW * 0.5f, realH * 0.5f),
            ImGuiCond_FirstUseEver,
            ImVec2(0.5f, 0.5f)
        );

        // Precise styling for Electric Orchid Glass
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.047f, 0.078f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.882f, 0.000f, 0.596f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.110f, 0.086f, 0.157f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.160f, 0.120f, 0.220f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.220f, 0.160f, 0.300f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.882f, 0.000f, 0.596f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.882f, 0.000f, 0.596f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.000f, 0.353f, 0.804f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.110f, 0.086f, 0.157f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.200f, 0.140f, 0.280f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.882f, 0.000f, 0.596f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.882f, 0.000f, 0.596f, 0.40f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.882f, 0.000f, 0.596f, 0.60f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.882f, 0.000f, 0.596f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.988f, 0.973f, 1.000f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.608f, 0.549f, 0.706f, 1.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("##NIN_MAIN_PANEL", nullptr, winFlags)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();

            // ──────────────────────────────────────────────────────────────────
            // 1. TOP HEADER (44px)
            // ──────────────────────────────────────────────────────────────────
            const float headerH = 44.0f;
            ImVec2 headerP0 = winPos;
            ImVec2 headerP1 = ImVec2(winPos.x + winSize.x, winPos.y + headerH);

            // Header Background & Underline
            dl->AddRectFilled(headerP0, headerP1, theme_orchid::ColHeaderBg, 12.0f, ImDrawFlags_RoundCornersTop);
            dl->AddLine(ImVec2(winPos.x, winPos.y + headerH), ImVec2(winPos.x + winSize.x, winPos.y + headerH), IM_COL32(54, 38, 76, 255), 1.0f);

            // Header BAM PASE: Logo + Name "NINMODS"
            float logoX = winPos.x + 22.0f;
            float headerCY = winPos.y + headerH * 0.5f;
            DrawLogoFlame(dl, ImVec2(logoX, headerCY), 18.0f, theme_orchid::ColPrimary);
            dl->AddText(ImVec2(logoX + 16.0f, headerCY - 9.0f), theme_orchid::ColWhite, "NINMODS");

            // Header DAN PASE: CLOSE ICON BUTTON (JUST BUTTON, NO BACKGROUND, NO BORDER!)
            const float closeBtnSize = 36.0f;
            ImVec2 closePos = ImVec2(winPos.x + winSize.x - closeBtnSize - 8.0f, winPos.y + (headerH - closeBtnSize) * 0.5f);
            ImGui::SetCursorScreenPos(closePos);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

            if (ImGui::Button("##CloseBtn", ImVec2(closeBtnSize, closeBtnSize))) {
                menu_open = false; // Pure close action
            }
            bool closeHovered = ImGui::IsItemHovered();

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(4);

            // Draw Clean Close Icon (X) centered in touch target
            ImVec2 closeCenter = ImVec2(closePos.x + closeBtnSize * 0.5f, closePos.y + closeBtnSize * 0.5f);
            ImU32 closeCol = closeHovered ? theme_orchid::ColPrimaryLight : IM_COL32(200, 195, 215, 230);
            const float xRadius = 6.0f;
            dl->AddLine(ImVec2(closeCenter.x - xRadius, closeCenter.y - xRadius), ImVec2(closeCenter.x + xRadius, closeCenter.y + xRadius), closeCol, 2.2f);
            dl->AddLine(ImVec2(closeCenter.x - xRadius, closeCenter.y + xRadius), ImVec2(closeCenter.x + xRadius, closeCenter.y - xRadius), closeCol, 2.2f);

            // ──────────────────────────────────────────────────────────────────
            // 2. MAIN BODY: LEFT SIDEBAR (ICON-ONLY) + RIGHT CONTENT AREA
            // ──────────────────────────────────────────────────────────────────
            const float bodyY = headerH;
            const float bodyH = winSize.y - headerH;
            const float sidebarW = 60.0f;

            // SIDEBAR CONTAINER
            ImGui::SetCursorPos(ImVec2(0, bodyY));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.067f, 0.055f, 0.094f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 10));

            if (ImGui::BeginChild("##Sidebar", ImVec2(sidebarW, bodyH), false, ImGuiWindowFlags_NoScrollbar)) {
                // Divider line between sidebar & content
                dl->AddLine(ImVec2(winPos.x + sidebarW, winPos.y + headerH), ImVec2(winPos.x + sidebarW, winPos.y + winSize.y), IM_COL32(40, 30, 56, 255), 1.0f);

                ImVec2 btnSize = ImVec2(sidebarW - 10.0f, 46.0f);

                // Tab 0: ESP (Eye Icon)
                if (DrawSidebarIconButton("##TabESP", ICON_EYE, (activeTab == 0), btnSize)) {
                    activeTab = 0;
                }
                ImGui::Dummy(ImVec2(0, 4));

                // Tab 1: Combat / Aim (Crosshair Icon)
                if (DrawSidebarIconButton("##TabCombat", ICON_CROSSHAIR, (activeTab == 1), btnSize)) {
                    activeTab = 1;
                }
                ImGui::Dummy(ImVec2(0, 4));

                // Tab 2: Settings (Gear Icon)
                if (DrawSidebarIconButton("##TabSettings", ICON_SETTINGS, (activeTab == 2), btnSize)) {
                    activeTab = 2;
                }

                // Bottom Safe Exit Icon (Anchored precisely to bottom edge)
                ImGui::SetCursorPos(ImVec2(5.0f, bodyH - 52.0f));
                if (DrawSidebarIconButton("##TabPower", ICON_POWER, false, btnSize)) {
                    main_thread_flag = false;
                    Touch_Close();
                    data::thread_stop();
                    shutdown();
                    exit(0);
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            // ──────────────────────────────────────────────────────────────────
            // 3. MAIN CONTENT AREA (EXACT DUAL-COLUMN GEOMETRY)
            // ──────────────────────────────────────────────────────────────────
            const float contentX = sidebarW + 12.0f;
            const float contentY = headerH + 10.0f;
            const float contentW = winSize.x - contentX - 12.0f;
            const float contentH = bodyH - 20.0f;
            const float colGap   = 12.0f;
            const float colW     = (contentW - colGap) * 0.5f;

            // Card background styling
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.086f, 0.067f, 0.130f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.196f, 0.141f, 0.282f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 9));

            // ══════════════════════════════════════════════════════════════════
            // TAB 0: ESP VISUALS (DUAL COLUMN: BAM PASE + DAN PASE)
            // ══════════════════════════════════════════════════════════════════
            if (activeTab == 0) {
                // ── LEFT COLUMN CARD (BAM PASE OPTION): ESP VISUALS ──
                ImGui::SetCursorPos(ImVec2(contentX, contentY));
                if (ImGui::BeginChild("##ColLeftESP", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "ESP VISUALS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 lpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(lpos.x, lpos.y - 4), ImVec2(lpos.x + 85, lpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("ESP Box (Chaturbhuj)", &cfg::esp::box);
                    ImGui::Checkbox("ESP Health Bar (Right)", &cfg::esp::health);
                    ImGui::Checkbox("ESP Laser Line", &cfg::esp::tracer);
                    ImGui::Checkbox("ESP Player Name", &cfg::esp::name);
                    ImGui::Checkbox("ESP Distance", &cfg::esp::distance);
                    ImGui::Checkbox("ESP Armor Bar", &cfg::esp::armor);
                    ImGui::Checkbox("ESP Weapon Name", &cfg::esp::weapon);
                    ImGui::Checkbox("Firing Indicator", &cfg::esp::firing_ind);
                }
                ImGui::EndChild();

                // ── RIGHT COLUMN CARD (DAN PASE OPTION): FILTERS & CONTROLS ──
                ImGui::SetCursorPos(ImVec2(contentX + colW + colGap, contentY));
                if (ImGui::BeginChild("##ColRightESP", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "SETTINGS & FILTERS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 rpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(rpos.x, rpos.y - 4), ImVec2(rpos.x + 120, rpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("Ignore Bots", &cfg::esp::bot_tag);
                    ImGui::Checkbox("Crosshair Center", &cfg::esp::crosshair);
                    ImGui::Checkbox("Loot ESP", &cfg::esp::loot);

                    ImGui::Dummy(ImVec2(0, 4));
                    const char* boxTypes[] = { "Corner", "Full Chaturbhuj", "Glow" };
                    ImGui::SetNextItemWidth(-1);
                    ImGui::Combo("##BoxStyle", &cfg::esp::box_type, boxTypes, 3);
                    ImGui::TextDisabled("Box Style Mode");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##MaxRange", &cfg::esp::max_range, 50.0f, 500.0f, "%.0f m");
                    ImGui::TextDisabled("Max Render Distance");

                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##LaserThickness", &cfg::esp::box_thickness, 1.0f, 5.0f, "%.1f px");
                    ImGui::TextDisabled("Laser Line Width");
                }
                ImGui::EndChild();
            }
            // ══════════════════════════════════════════════════════════════════
            // TAB 1: COMBAT & TARGET FILTERS (DUAL COLUMN)
            // ══════════════════════════════════════════════════════════════════
            else if (activeTab == 1) {
                // Left Column: Targeting options
                ImGui::SetCursorPos(ImVec2(contentX, contentY));
                if (ImGui::BeginChild("##ColLeftCombat", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "TARGET PRIORITY");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 lpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(lpos.x, lpos.y - 4), ImVec2(lpos.x + 110, lpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Checkbox("Auto Target Closest", &cfg::esp::firing_ind);
                    ImGui::Checkbox("Prioritize Low HP", &cfg::esp::health);
                    ImGui::Checkbox("Filter Invisible Enemies", &cfg::esp::box);

                    ImGui::Dummy(ImVec2(0, 10));
                    ImGui::TextDisabled("Tactical Notice:");
                    ImGui::TextWrapped("Direct touch injection is completely removed for maximum stability.");
                }
                ImGui::EndChild();

                // Right Column: Target info & details
                ImGui::SetCursorPos(ImVec2(contentX + colW + colGap, contentY));
                if (ImGui::BeginChild("##ColRightCombat", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "FOV & RANGES");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 rpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(rpos.x, rpos.y - 4), ImVec2(rpos.x + 95, rpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::SetNextItemWidth(-1);
                    ImGui::SliderFloat("##LootRange", &cfg::esp::loot_range, 30.0f, 300.0f, "%.0f m");
                    ImGui::TextDisabled("Loot Scan Radius");

                    ImGui::Dummy(ImVec2(0, 10));
                    ImGui::Text("Active Target: None");
                    ImGui::Text("Target Distance: -- m");
                    ImGui::Text("Target Health: --");
                }
                ImGui::EndChild();
            }
            // ══════════════════════════════════════════════════════════════════
            // TAB 2: SETTINGS & SYSTEM STATUS (DUAL COLUMN)
            // ══════════════════════════════════════════════════════════════════
            else if (activeTab == 2) {
                // Left Column: UI Customization
                ImGui::SetCursorPos(ImVec2(contentX, contentY));
                if (ImGui::BeginChild("##ColLeftSettings", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "UI APPEARANCE");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 lpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(lpos.x, lpos.y - 4), ImVec2(lpos.x + 105, lpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    if (ImGui::SliderFloat("##Opacity", &ui_opacity, 0.2f, 1.0f, "Opacity: %.2f")) {
                        ImGui::GetStyle().Alpha = ui_opacity;
                    }

                    ImGui::Dummy(ImVec2(0, 12));
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Engine Architecture:");
                    ImGui::Text("Kernel DMA / RPM Memory");
                    ImGui::Text("100% Non-Invasive");
                    ImGui::Text("Screen Record: Unblocked");
                    ImGui::Text("Screenshot: Allowed");
                }
                ImGui::EndChild();

                // Right Column: Live Process Diagnostics
                ImGui::SetCursorPos(ImVec2(contentX + colW + colGap, contentY));
                if (ImGui::BeginChild("##ColRightSettings", ImVec2(colW, contentH), true)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.80f, 1.0f), "LIVE DIAGNOSTICS");
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    ImVec2 rpos = ImGui::GetCursorScreenPos();
                    cdl->AddLine(ImVec2(rpos.x, rpos.y - 4), ImVec2(rpos.x + 115, rpos.y - 4), theme_orchid::ColPrimary, 2.0f);
                    ImGui::Dummy(ImVec2(0, 4));

                    ImGui::Text("Target PID: %d", proc::pid);
                    ImGui::TextColored(proc::lib ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0.8f, 0, 1),
                        proc::lib ? "libil2cpp: 0x%lx" : "Waiting for Game...",
                        (unsigned long)proc::lib);
                    ImGui::Text("Overlay FPS: %.1f", io.Framerate);
                    ImGui::Text("Screen: %.0f x %.0f", realW, realH);

                    ImGui::Dummy(ImVec2(0, 12));
                    if (ImGui::Button("SAFE EXIT APPLICATION", ImVec2(ImGui::GetContentRegionAvail().x, 38))) {
                        main_thread_flag = false;
                        Touch_Close();
                        data::thread_stop();
                        shutdown();
                        exit(0);
                    }
                }
                ImGui::EndChild();
            }

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
        }
        ImGui::End();

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(16);
    }
}
