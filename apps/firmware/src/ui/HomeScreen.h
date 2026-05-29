// dsk-guard — HomeScreen (v2: card-based dashboard)
// 480×320 ILI9488, dark theme, 3×2 card grid
// FreeSans fonts via TFT_eSPI GFXFF

#pragma once

#include "Screen.h"
#include "../sensors/bh1750.h"
#include "../sensors/bme680.h"
#include "../sensors/radar_types.h"
#include "../sensors/acd1200.h"

class HomeScreen : public Screen {
public:
    void begin(TFT_eSPI &tft) override;
    void update(TFT_eSPI &tft, uint32_t now) override;

    /// Feed latest sensor readings. Called from main loop.
    void setSensorData(const LuxReading &lux, const EnvReading &env,
                       const RadarReading &radar, const Co2Reading &co2);

private:
    // Cached sensor values
    LuxReading   _lux   = {};
    EnvReading   _env   = {};
    RadarReading _radar = {};
    Co2Reading   _co2   = {};
    bool _sensorDirty = false;

    // Cached footer stats
    uint32_t _lastStatsUpdate = 0;
    uint32_t _lastHeap    = 0;
    uint32_t _lastUptime  = 0;

    // --- Layout constants ---
    static constexpr int16_t SCREEN_W = 480;
    static constexpr int16_t SCREEN_H = 320;

    // Header
    static constexpr int16_t HDR_H    = 30;

    // Card grid: 3 cols × 2 rows
    static constexpr int16_t MARGIN   = 8;
    static constexpr int16_t GAP      = 5;
    static constexpr int16_t CARD_Y0  = 34;   // top of first card row
    static constexpr int16_t FTR_H    = 22;   // footer height
    static constexpr uint8_t COLS     = 3;
    static constexpr uint8_t ROWS     = 2;
    static constexpr int16_t ACCENT_W = 3;     // left accent bar width
    static constexpr int16_t CARD_R   = 6;     // corner radius

    // Computed card dimensions
    static constexpr int16_t CARD_W = (SCREEN_W - 2 * MARGIN - (COLS - 1) * GAP) / COLS;
    static constexpr int16_t CARD_AREA_H = SCREEN_H - CARD_Y0 - FTR_H - 4;
    static constexpr int16_t CARD_H = (CARD_AREA_H - (ROWS - 1) * GAP) / ROWS;

public:
    // RGB565 color helper — public so .cpp file-scope constants can use it
    static constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
private:

    // Card positions
    struct CardPos {
        int16_t x, y;
    };
    static CardPos cardPos(uint8_t col, uint8_t row);

    // Drawing
    void drawHeader(TFT_eSPI &tft);
    void drawAllCards(TFT_eSPI &tft);
    void drawCard(TFT_eSPI &tft, uint8_t col, uint8_t row,
                  uint16_t accent, const char *label,
                  const char *value, const char *unit,
                  uint16_t valueColor);
    void drawFooter(TFT_eSPI &tft, uint32_t now, bool force);
};
