// dsk-guard — HomeScreen v2 (card dashboard)
// Dark theme, 3×2 sensor cards, FreeSans fonts

#include "HomeScreen.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

// FreeFonts — reference Adafruit GFX font structs directly
// (avoids Free_Fonts.h include path issues in PlatformIO)
extern const GFXfont FreeSans9pt7b;
extern const GFXfont FreeSansBold18pt7b;
#define FSS9  &FreeSans9pt7b
#define FSB18 &FreeSansBold18pt7b

#ifndef DG_FW_VERSION
#define DG_FW_VERSION "0.0.0"
#endif

// ---- Color palette (RGB565) ----
static constexpr uint16_t COL_BG       = HomeScreen::rgb565(13, 17, 23);    // #0D1117
static constexpr uint16_t COL_CARD     = HomeScreen::rgb565(22, 27, 34);    // #161B22
static constexpr uint16_t COL_HDR_LINE = HomeScreen::rgb565(33, 38, 45);    // #21262D
static constexpr uint16_t COL_LABEL    = HomeScreen::rgb565(125, 133, 144); // #7D8590
static constexpr uint16_t COL_UNIT     = HomeScreen::rgb565(72, 79, 88);    // #484F58
static constexpr uint16_t COL_TEXT     = HomeScreen::rgb565(230, 237, 243); // #E6EDF3
static constexpr uint16_t COL_GREEN    = HomeScreen::rgb565(107, 203, 119); // #6BCB77
static constexpr uint16_t COL_RED      = HomeScreen::rgb565(255, 107, 107); // #FF6B6B
static constexpr uint16_t COL_YELLOW   = HomeScreen::rgb565(255, 217, 61);  // #FFD93D
static constexpr uint16_t COL_CYAN     = HomeScreen::rgb565(78, 205, 196);  // #4ECDC4
static constexpr uint16_t COL_BLUE     = HomeScreen::rgb565(77, 150, 255);  // #4D96FF
static constexpr uint16_t COL_PURPLE   = HomeScreen::rgb565(155, 89, 182);  // #9B59B6
static constexpr uint16_t COL_ORANGE   = HomeScreen::rgb565(255, 159, 67);  // #FF9F43
static constexpr uint16_t COL_DIM_GRN  = HomeScreen::rgb565(63, 185, 80);  // #3FB950

static constexpr uint32_t STATS_INTERVAL_MS = 1000;

// ---- Card position helper ----

HomeScreen::CardPos HomeScreen::cardPos(uint8_t col, uint8_t row) {
    return {
        static_cast<int16_t>(MARGIN + col * (CARD_W + GAP)),
        static_cast<int16_t>(CARD_Y0 + row * (CARD_H + GAP))
    };
}

// ---- Public ----

void HomeScreen::begin(TFT_eSPI &tft) {
    tft.fillScreen(COL_BG);
    drawHeader(tft);
    drawAllCards(tft);
    drawFooter(tft, millis(), true);
    _dirty = false;
    _lastStatsUpdate = 0;
}

void HomeScreen::update(TFT_eSPI &tft, uint32_t now) {
    if (consumeDirty()) {
        tft.fillScreen(COL_BG);
        drawHeader(tft);
        drawAllCards(tft);
        drawFooter(tft, now, true);
        return;
    }

    if (_sensorDirty) {
        _sensorDirty = false;
        drawAllCards(tft);
    }

    if (now - _lastStatsUpdate >= STATS_INTERVAL_MS) {
        drawFooter(tft, now, false);
    }
}

void HomeScreen::setSensorData(const LuxReading &lux, const EnvReading &env,
                                const RadarReading &radar, const Co2Reading &co2) {
    _lux = lux;
    _env = env;
    _radar = radar;
    _co2 = co2;
    _sensorDirty = true;
}

// ---- Private: Header ----

void HomeScreen::drawHeader(TFT_eSPI &tft) {
    // Green dot
    tft.fillCircle(MARGIN + 6, HDR_H / 2 + 1, 4, COL_GREEN);

    // Title
    tft.setFreeFont(FSS9);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.drawString("Desk Guardian", MARGIN + 16, HDR_H / 2 + 1);

    // FW version
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(COL_UNIT, COL_BG);
    tft.drawString("fw " DG_FW_VERSION, SCREEN_W - MARGIN, HDR_H / 2 + 1);

    // Divider line
    tft.drawFastHLine(MARGIN, HDR_H + 1, SCREEN_W - 2 * MARGIN, COL_HDR_LINE);
}

// ---- Private: Cards ----

void HomeScreen::drawCard(TFT_eSPI &tft, uint8_t col, uint8_t row,
                           uint16_t accent, const char *label,
                           const char *value, const char *unit,
                           uint16_t valueColor) {
    CardPos p = cardPos(col, row);

    // Card background
    tft.fillRoundRect(p.x, p.y, CARD_W, CARD_H, CARD_R, COL_CARD);

    // Accent bar (left edge)
    tft.fillRect(p.x, p.y + CARD_R, ACCENT_W, CARD_H - 2 * CARD_R, accent);
    // Fill accent into corners
    tft.fillRect(p.x, p.y + 2, ACCENT_W, CARD_R, accent);
    tft.fillRect(p.x, p.y + CARD_H - CARD_R - 2, ACCENT_W, CARD_R, accent);

    int16_t cx = p.x + ACCENT_W + 10;  // content x start

    // Label (small, grey)
    tft.setFreeFont(FSS9);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COL_LABEL, COL_CARD);
    tft.drawString(label, cx, p.y + 10);

    // Value (large, colored)
    tft.setFreeFont(FSB18);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(valueColor, COL_CARD);
    tft.drawString(value, cx, p.y + 30);

    // Unit (small, dim)
    tft.setFreeFont(FSS9);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COL_UNIT, COL_CARD);
    tft.drawString(unit, cx, p.y + CARD_H - 24);
}

void HomeScreen::drawAllCards(TFT_eSPI &tft) {
    char buf[24];

    // ---- Card 0,0: Light ----
    uint16_t luxColor = COL_GREEN;
    if (_lux.ok) {
        if (_lux.lux < 200) luxColor = COL_ORANGE;
        else if (_lux.lux > 400) luxColor = COL_YELLOW;
        snprintf(buf, sizeof(buf), "%.0f", _lux.lux);
    } else {
        snprintf(buf, sizeof(buf), "--");
        luxColor = COL_RED;
    }
    drawCard(tft, 0, 0, COL_YELLOW, "LIGHT", buf, "lux", luxColor);

    // ---- Card 1,0: Temperature ----
    if (_env.ok) {
        snprintf(buf, sizeof(buf), "%.1f", _env.temp_c);
    } else {
        snprintf(buf, sizeof(buf), "--");
    }
    drawCard(tft, 1, 0, COL_RED, "TEMP", buf,
             "deg C",
             _env.ok ? COL_TEXT : COL_RED);

    // ---- Card 2,0: Humidity ----
    if (_env.ok) {
        snprintf(buf, sizeof(buf), "%.1f", _env.humidity);
    } else {
        snprintf(buf, sizeof(buf), "--");
    }
    drawCard(tft, 2, 0, COL_CYAN, "HUMIDITY", buf, "%",
             _env.ok ? COL_TEXT : COL_RED);

    // ---- Card 0,1: CO2 ----
    uint16_t co2Color = COL_GREEN;
    const char *co2Unit = "ppm";
    if (_co2.ok) {
        if (_co2.preheating) {
            co2Color = COL_UNIT;
            co2Unit = "ppm *";
        } else if (_co2.co2_ppm > 1000) {
            co2Color = COL_RED;
        } else if (_co2.co2_ppm > 800) {
            co2Color = COL_YELLOW;
        }
        snprintf(buf, sizeof(buf), "%u", _co2.co2_ppm);
    } else {
        snprintf(buf, sizeof(buf), "--");
        co2Color = COL_RED;
    }
    drawCard(tft, 0, 1, COL_GREEN, "CO2", buf, co2Unit, co2Color);

    // ---- Card 1,1: Presence ----
    const char *presVal = "--";
    const char *presSub = "";
    uint16_t presColor = COL_TEXT;
    if (_radar.ok) {
        if (_radar.state == PresenceState::PRESENT) {
            presVal = "Active";
            presColor = COL_GREEN;
            static char distBuf[16];
            snprintf(distBuf, sizeof(distBuf), "%u cm", _radar.distance_cm);
            presSub = distBuf;
        } else {
            presVal = "Away";
            presColor = COL_UNIT;
        }
    } else {
        presColor = COL_RED;
    }
    drawCard(tft, 1, 1, COL_BLUE, "PRESENCE", presVal, presSub, presColor);

    // ---- Card 2,1: Air quality (gas resistance) ----
    uint16_t airColor = COL_TEXT;
    const char *airUnit = "ohm";
    if (_env.ok) {
        if (_env.gas_ohm > 1000000) {
            snprintf(buf, sizeof(buf), "%.1f M", _env.gas_ohm / 1000000.0f);
        } else if (_env.gas_ohm > 1000) {
            snprintf(buf, sizeof(buf), "%.0f k", _env.gas_ohm / 1000.0f);
        } else {
            snprintf(buf, sizeof(buf), "%lu", (unsigned long)_env.gas_ohm);
        }
    } else {
        snprintf(buf, sizeof(buf), "--");
        airColor = COL_RED;
    }
    drawCard(tft, 2, 1, COL_PURPLE, "AIR", buf, airUnit, airColor);
}

// ---- Private: Footer ----

void HomeScreen::drawFooter(TFT_eSPI &tft, uint32_t now, bool force) {
    uint32_t heap  = ESP.getFreeHeap();
    uint32_t upSec = now / 1000;

    // Only redraw if changed (or forced)
    if (!force && heap == _lastHeap && upSec == _lastUptime) return;

    _lastStatsUpdate = now;
    _lastHeap   = heap;
    _lastUptime = upSec;

    int16_t ftrY = SCREEN_H - FTR_H;

    // Clear footer area
    tft.fillRect(0, ftrY, SCREEN_W, FTR_H, COL_BG);

    // Divider
    tft.drawFastHLine(MARGIN, ftrY, SCREEN_W - 2 * MARGIN, COL_HDR_LINE);

    // Use built-in small font for footer (more compact)
    tft.setTextFont(1);
    int16_t textY = ftrY + 7;

    // Pressure
    char buf[24];
    if (_env.ok) {
        snprintf(buf, sizeof(buf), "%.0f hPa", _env.pressure);
    } else {
        snprintf(buf, sizeof(buf), "-- hPa");
    }
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(COL_UNIT, COL_BG);
    tft.drawString(buf, MARGIN + 4, textY);

    // Heap
    snprintf(buf, sizeof(buf), "Heap: %lu KB", (unsigned long)(heap / 1024));
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, SCREEN_W / 3, textY);

    // PSRAM
    snprintf(buf, sizeof(buf), "PSRAM: %lu KB", (unsigned long)(ESP.getFreePsram() / 1024));
    tft.drawString(buf, SCREEN_W * 2 / 3, textY);

    // Uptime
    uint32_t m = upSec / 60;
    uint32_t s = upSec % 60;
    snprintf(buf, sizeof(buf), "Up: %lum %02lus", (unsigned long)m, (unsigned long)s);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(COL_DIM_GRN, COL_BG);
    tft.drawString(buf, SCREEN_W - MARGIN - 4, textY);
}
