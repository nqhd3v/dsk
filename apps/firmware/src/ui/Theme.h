// dsk-guard — UI Theme (colors, layout constants, font helpers)
// Shared across all screen implementations.

#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// FreeFonts — extern declarations (compiled into TFT_eSPI via LOAD_GFXFF)
extern const GFXfont FreeSans9pt7b;
extern const GFXfont FreeSans12pt7b;
extern const GFXfont FreeSansBold12pt7b;
extern const GFXfont FreeSansBold18pt7b;
extern const GFXfont FreeSansBold24pt7b;

#define FONT_SM    &FreeSans9pt7b         // labels, units
#define FONT_MD    &FreeSans12pt7b        // env values
#define FONT_MD_B  &FreeSansBold12pt7b    // env values bold
#define FONT_LG    &FreeSansBold18pt7b    // medium numbers
#define FONT_XL    &FreeSansBold24pt7b    // countdown digits

// RGB565 helper
static constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ---- Color palette ----
namespace Theme {
    static constexpr uint16_t BG        = rgb565(13, 17, 23);     // #0D1117
    static constexpr uint16_t CARD      = rgb565(22, 27, 34);     // #161B22
    static constexpr uint16_t LINE      = rgb565(33, 38, 45);     // #21262D
    static constexpr uint16_t LABEL     = rgb565(125, 133, 144);  // #7D8590
    static constexpr uint16_t DIM       = rgb565(72, 79, 88);     // #484F58
    static constexpr uint16_t TEXT      = rgb565(230, 237, 243);  // #E6EDF3
    static constexpr uint16_t GREEN     = rgb565(107, 203, 119);  // #6BCB77
    static constexpr uint16_t RED       = rgb565(255, 107, 107);  // #FF6B6B
    static constexpr uint16_t YELLOW    = rgb565(255, 217, 61);   // #FFD93D
    static constexpr uint16_t CYAN      = rgb565(78, 205, 196);   // #4ECDC4
    static constexpr uint16_t BLUE      = rgb565(77, 150, 255);   // #4D96FF
    static constexpr uint16_t PURPLE    = rgb565(155, 89, 182);   // #9B59B6
    static constexpr uint16_t ORANGE    = rgb565(255, 159, 67);   // #FF9F43
    static constexpr uint16_t DIM_GREEN = rgb565(63, 185, 80);    // #3FB950

    // Screen dimensions (landscape)
    static constexpr int16_t W = 480;
    static constexpr int16_t H = 320;
}
