// dsk-guard — ActiveScreen
// Left: HH:MM clock + "Stand up in XX min" + [Active] pill + actionable suggestion
// Right: 2×2 mini cards (Temp °C, Humidity %, CO2 quality, Air quality)
// Footer: raw env data | online dot

#pragma once

#include "Screen.h"
#include "Theme.h"
#include "../sensors/bh1750.h"
#include "../sensors/bme680.h"
#include "../sensors/radar_types.h"
#include "../sensors/acd1200.h"

struct SensorBundle {
    LuxReading   lux;
    EnvReading   env;
    RadarReading radar;
    Co2Reading   co2;
};

class ActiveScreen : public Screen {
public:
    void begin(TFT_eSPI &tft) override;
    void update(TFT_eSPI &tft, uint32_t now) override;

    void setSensors(const SensorBundle &s);
    void setCountdown(uint32_t remainSec, uint32_t thresholdSec, uint32_t sittingSec);
    void setStatus(const char *text, uint16_t color);
    void setNetStatus(const char *text);

private:
    SensorBundle _s = {};
    bool _sensorDirty = false;

    uint32_t _remainSec    = 0;
    uint32_t _thresholdSec = 0;
    uint32_t _sittingSec   = 0;
    uint32_t _lastRemainMin = 0xFFFF;
    uint32_t _lastRemainSec = 0xFFFFFFFF;
    bool     _countdownDirty = false;

    char     _statusText[24] = "Active";
    uint16_t _statusColor = Theme::GREEN;
    bool     _statusDirty = false;

    char     _netStatus[32] = "";
    bool     _netDirty = false;

    uint32_t _lastClockMin = 0xFFFF;

    // Layout
    static constexpr int16_t DIVIDER_X = 240;   // left/right split
    static constexpr int16_t FOOTER_H  = 28;

    // Right panel: 2×2 cards
    static constexpr int16_t CARD_MARGIN = 6;
    static constexpr int16_t CARD_R     = 8;

    void drawClock(TFT_eSPI &tft, bool force);
    void drawCountdownHint(TFT_eSPI &tft);
    void drawStatusPill(TFT_eSPI &tft);
    void drawSuggestion(TFT_eSPI &tft);
    void drawEnvCards(TFT_eSPI &tft);
    void drawMiniCard(TFT_eSPI &tft, int16_t x, int16_t y, int16_t w, int16_t h,
                      const char *label, const char *value, const char *unit,
                      uint16_t valColor);
    void drawFooter(TFT_eSPI &tft);

    // Quality helpers
    static const char *co2Quality(uint16_t ppm, uint16_t &color);
    static const char *airQuality(uint32_t gas_ohm, uint16_t &color);
    const char *suggestion(uint16_t &color) const;
};
