// dsk-guard — SummaryScreen
// Centered 3×2 env cards, no countdown clock.
// Shown when person is absent for inactiveDelay (SUMMARY mode).

#pragma once

#include "Screen.h"
#include "Theme.h"
#include "ActiveScreen.h"   // SensorBundle

class SummaryScreen : public Screen {
public:
    void begin(TFT_eSPI &tft) override;
    void update(TFT_eSPI &tft, uint32_t now) override;

    void setSensors(const SensorBundle &s);

private:
    SensorBundle _s = {};
    bool _sensorDirty = false;

    // Card layout: 3 cols × 2 rows, centered
    static constexpr int16_t MARGIN  = 12;
    static constexpr int16_t GAP     = 8;
    static constexpr int16_t HDR_H   = 36;
    static constexpr int16_t CARD_R  = 8;
    static constexpr uint8_t COLS    = 3;
    static constexpr uint8_t ROWS    = 2;
    static constexpr int16_t CARD_W  = (Theme::W - 2 * MARGIN - (COLS - 1) * GAP) / COLS;
    static constexpr int16_t CARD_H  = (Theme::H - HDR_H - 2 * MARGIN - (ROWS - 1) * GAP) / ROWS;
    static constexpr int16_t CARD_Y0 = HDR_H + MARGIN;

    void drawHeader(TFT_eSPI &tft);
    void drawCards(TFT_eSPI &tft);
    void drawCard(TFT_eSPI &tft, uint8_t col, uint8_t row,
                  const char *label, const char *value, const char *unit,
                  uint16_t valColor);
};
