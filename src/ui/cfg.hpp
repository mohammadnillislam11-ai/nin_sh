#pragma once
#include "imgui.h"

namespace cfg {
namespace esp {
    // ── Master ──
    inline bool  box              = true;
    inline int   box_type         = 1; // 1 = Full Chaturbhuj (Rectangular) Box
    inline bool  name             = true;
    inline bool  health           = true;
    inline bool  armor            = true;
    inline bool  distance         = true;
    inline bool  weapon           = true;
    inline bool  bot_tag          = true;
    inline bool  ignore_bots      = false;
    inline bool  tracer           = true;
    inline bool  crosshair        = true;
    inline bool  firing_ind       = true;
    inline bool  loot             = false;

    // ── Setting ──
    inline float max_range        = 250.f;
    inline float loot_range       = 150.f;
    inline float box_rounding     = 3.f;
    inline float box_thickness    = 2.0f;
    inline float tracer_thickness = 1.2f;
    inline float bracket_frac     = 0.28f;

    // ── Warna ──
    inline ImVec4 box_col         = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 box_visible     = ImVec4(0.30f, 1.0f, 0.55f, 1.0f);
    inline ImVec4 name_col        = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col    = ImVec4(0.95f, 0.95f, 0.30f, 1.0f);
    inline ImVec4 weapon_col      = ImVec4(1.0f, 0.85f, 0.30f, 1.0f);
}

namespace combat {
    inline bool auto_target_closest = true;
    inline bool prioritize_low_hp   = false;
    inline bool filter_invisible    = true;
}

namespace aim {
    inline bool  enable           = true;
    inline bool  show_fov         = true;
    inline float fov_radius       = 180.0f;
    inline float smooth           = 6.0f;
    inline int   target_bone      = 0; // 0 = Head, 1 = Neck/Chest, 2 = Pelvis/Body
    inline int   trigger_mode     = 0; // 0 = When Firing, 1 = When Scoping, 2 = Always In FOV
    inline int   priority_mode    = 0; // 0 = Crosshair Distance, 1 = Low HP, 2 = 3D Distance
    inline bool  ignore_knocked   = true;
    inline bool  ignore_bots      = false;
    inline bool  visible_check    = true;
    inline float max_distance     = 200.0f;
    inline bool  prediction       = true;
}
}
