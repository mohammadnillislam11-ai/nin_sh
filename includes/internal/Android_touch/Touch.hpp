#pragma once
#include <cstdint>

// ═══════════════════════════════════════════════════════
// Touch Driver Interface (Aligned with Brutal TouchHelperA)
// ═══════════════════════════════════════════════════════
bool Touch_Init(int w, int h, uint32_t orientation_, bool readOnly = true);
void UpdateScreenData(int w, int h, uint32_t orientation_);
void Touch_Close();
void Touch_Down(float x, float y);
void Touch_Move(float x, float y);
void Touch_Up();

namespace touch {
    inline bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        return Touch_Init(_screen_w, _screen_h, (uint32_t)_orientation, true);
    }
    inline void update(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        UpdateScreenData(_screen_w, _screen_h, (uint32_t)_orientation);
    }
    inline void updateOrientation(uint8_t _orientation) {
        // Handled via UpdateScreenData
    }
    inline void shutdown() {
        Touch_Close();
    }
}
