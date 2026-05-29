// dsk-guard — AlertScreen
// Full-screen "Stand up!" reminder with pulsing animation.
// Shown when sitting threshold exceeded (ALERT mode).

#pragma once

#include "Screen.h"
#include "Theme.h"

class AlertScreen : public Screen {
public:
    void begin(TFT_eSPI &tft) override;
    void update(TFT_eSPI &tft, uint32_t now) override;

    /// Set how long user has been sitting.
    void setSittingMin(uint32_t minutes) { _sittingMin = minutes; }

private:
    uint32_t _sittingMin = 0;
    uint32_t _lastPulse  = 0;
    bool     _pulseHigh  = true;

    static constexpr uint32_t PULSE_MS = 800;  // toggle interval

    void drawContent(TFT_eSPI &tft, uint16_t accentColor);
};
