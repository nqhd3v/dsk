// dsk-guard — ActiveScreen implementation
// Left: HH:MM clock + countdown hint + [Active] pill + actionable suggestion
// Right: 2×2 mini cards (Temp, Humidity, CO2, Air)
// Footer: raw env | online dot

#include "ActiveScreen.h"
#include <Arduino.h>
#include <time.h>

// ── Quality helpers ─────────────────────────────────────────────

const char *ActiveScreen::co2Quality(uint16_t ppm, uint16_t &color) {
    if (ppm < 600)       { color = Theme::GREEN;  return "Fresh"; }
    if (ppm < 800)       { color = Theme::GREEN;  return "Good"; }
    if (ppm < 1000)      { color = Theme::YELLOW; return "Fair"; }
    if (ppm < 1500)      { color = Theme::ORANGE; return "Poor"; }
    color = Theme::RED; return "Bad!";
}

const char *ActiveScreen::airQuality(uint32_t gas_ohm, uint16_t &color) {
    if (gas_ohm > 300000)  { color = Theme::GREEN;  return "Clean"; }
    if (gas_ohm > 150000)  { color = Theme::GREEN;  return "Good"; }
    if (gas_ohm > 75000)   { color = Theme::YELLOW; return "Fair"; }
    if (gas_ohm > 30000)   { color = Theme::ORANGE; return "Poor"; }
    color = Theme::RED; return "Bad!";
}

// ── Actionable suggestions ──────────────────────────────────────

const char *ActiveScreen::suggestion(uint16_t &color) const {
    // Priority: worst condition → actionable advice
    if (_s.co2.ok && !_s.co2.preheating) {
        if (_s.co2.co2_ppm >= 1500) { color = Theme::RED;    return "Open the window!"; }
        if (_s.co2.co2_ppm >= 1000) { color = Theme::ORANGE; return "Open a window"; }
    }

    if (_s.env.ok && _s.env.gas_ohm < 30000) {
        color = Theme::RED; return "Ventilate the room";
    }

    if (_s.lux.ok) {
        if (_s.lux.lux < 100) { color = Theme::ORANGE; return "Turn on a light"; }
        if (_s.lux.lux > 600) { color = Theme::YELLOW; return "Reduce brightness"; }
    }

    if (_s.env.ok) {
        if (_s.env.temp_c > 32) { color = Theme::RED;    return "Turn on the fan"; }
        if (_s.env.temp_c > 28) { color = Theme::YELLOW; return "Consider a fan"; }
        if (_s.env.temp_c < 18) { color = Theme::CYAN;   return "Grab a jacket"; }
        if (_s.env.humidity > 70) { color = Theme::ORANGE; return "Use dehumidifier"; }
        if (_s.env.humidity < 30) { color = Theme::YELLOW; return "Use humidifier"; }
    }

    if (_s.co2.ok && !_s.co2.preheating && _s.co2.co2_ppm >= 800) {
        color = Theme::YELLOW; return "Getting stuffy";
    }

    color = Theme::GREEN; return "Looking good";
}

// ── begin() — full draw ─────────────────────────────────────────

void ActiveScreen::begin(TFT_eSPI &tft) {
    tft.fillScreen(Theme::BG);

    // Vertical divider
    tft.drawFastVLine(DIVIDER_X, 0, Theme::H - FOOTER_H, Theme::LINE);

    drawClock(tft, true);
    drawCountdownHint(tft);
    drawStatusPill(tft);
    drawSuggestion(tft);
    drawEnvCards(tft);
    drawFooter(tft);
    _dirty = false;
    _lastClockMin = 0xFFFF;
}

// ── update() — dirty-region redraws ─────────────────────────────

void ActiveScreen::update(TFT_eSPI &tft, uint32_t now) {
    if (consumeDirty()) {
        begin(tft);
        return;
    }

    // Clock update (every minute)
    struct tm ti;
    if (getLocalTime(&ti, 0)) {
        uint32_t curMin = ti.tm_hour * 60 + ti.tm_min;
        if (curMin != _lastClockMin) {
            _lastClockMin = curMin;
            drawClock(tft, false);
        }
    }

    if (_countdownDirty) {
        _countdownDirty = false;
        drawCountdownHint(tft);
    }

    if (_statusDirty || _netDirty) {
        _statusDirty = false;
        _netDirty = false;
        drawStatusPill(tft);
        drawFooter(tft);
    }

    if (_sensorDirty) {
        _sensorDirty = false;
        drawEnvCards(tft);
        drawSuggestion(tft);
        drawFooter(tft);
    }
}

// ── Public setters ──────────────────────────────────────────────

void ActiveScreen::setSensors(const SensorBundle &s) {
    _s = s;
    _sensorDirty = true;
}

void ActiveScreen::setCountdown(uint32_t remainSec, uint32_t thresholdSec, uint32_t sittingSec) {
    _remainSec    = remainSec;
    _thresholdSec = thresholdSec;
    _sittingSec   = sittingSec;
    uint32_t remainMin = _remainSec / 60;
    if (remainMin != _lastRemainMin) {
        _lastRemainMin = remainMin;
        _countdownDirty = true;
    }
}

void ActiveScreen::setStatus(const char *text, uint16_t color) {
    if (strcmp(text, _statusText) != 0 || color != _statusColor) {
        strncpy(_statusText, text, sizeof(_statusText) - 1);
        _statusText[sizeof(_statusText) - 1] = '\0';
        _statusColor = color;
        _statusDirty = true;
    }
}

void ActiveScreen::setNetStatus(const char *text) {
    if (strcmp(text, _netStatus) != 0) {
        strncpy(_netStatus, text, sizeof(_netStatus) - 1);
        _netStatus[sizeof(_netStatus) - 1] = '\0';
        _netDirty = true;
    }
}

// ── Clock (left panel, primary) ─────────────────────────────────

void ActiveScreen::drawClock(TFT_eSPI &tft, bool force) {
    int16_t panelW = DIVIDER_X - 1;
    int16_t cx = panelW / 2;

    if (force) {
        tft.fillRect(0, 0, panelW, 100, Theme::BG);
    }

    // HH:MM — big centered
    struct tm ti;
    char clockBuf[8] = "--:--";
    if (getLocalTime(&ti, 0)) {
        snprintf(clockBuf, sizeof(clockBuf), "%02d:%02d", ti.tm_hour, ti.tm_min);
    }

    tft.fillRect(10, 20, panelW - 20, 65, Theme::BG);
    tft.setFreeFont(FONT_XL);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::TEXT, Theme::BG);
    tft.drawString(clockBuf, cx, 55);
}

// ── Countdown hint (below clock) ────────────────────────────────

void ActiveScreen::drawCountdownHint(TFT_eSPI &tft) {
    int16_t panelW = DIVIDER_X - 1;
    int16_t cx = panelW / 2;
    int16_t y = 95;

    tft.fillRect(0, y - 5, panelW, 25, Theme::BG);

    uint32_t remainMin = _remainSec / 60;

    // Color by urgency
    uint16_t hintColor = Theme::DIM;
    if (remainMin <= 2)       hintColor = Theme::RED;
    else if (remainMin <= 10) hintColor = Theme::YELLOW;

    char hint[32];
    if (_remainSec == 0) {
        snprintf(hint, sizeof(hint), "Time to stand up!");
        hintColor = Theme::RED;
    } else {
        snprintf(hint, sizeof(hint), "Stand up in %lu min", (unsigned long)remainMin);
    }

    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(hintColor, Theme::BG);
    tft.drawString(hint, cx, y);
}

// ── Status pill (left, below hint) ──────────────────────────────

void ActiveScreen::drawStatusPill(TFT_eSPI &tft) {
    int16_t cx = (DIVIDER_X - 1) / 2;
    int16_t py = 130;

    tft.fillRect(0, py - 2, DIVIDER_X - 1, 34, Theme::BG);

    int16_t pw = 110;
    int16_t ph = 28;
    tft.fillRoundRect(cx - pw / 2, py, pw, ph, ph / 2, Theme::CARD);
    tft.fillCircle(cx - pw / 2 + 13, py + ph / 2, 4, _statusColor);
    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(_statusColor, Theme::CARD);
    tft.drawString(_statusText, cx - pw / 2 + 22, py + ph / 2);
}

// ── Actionable suggestion (left, bottom area) ───────────────────

void ActiveScreen::drawSuggestion(TFT_eSPI &tft) {
    int16_t cx = (DIVIDER_X - 1) / 2;
    int16_t y = 190;

    tft.fillRect(0, y - 10, DIVIDER_X - 1, 50, Theme::BG);

    uint16_t sugColor;
    const char *sug = suggestion(sugColor);

    tft.setFreeFont(FONT_MD_B);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(sugColor, Theme::BG);
    tft.drawString(sug, cx, y + 10);
}

// ── 2×2 env mini cards (right panel) ────────────────────────────

void ActiveScreen::drawEnvCards(TFT_eSPI &tft) {
    int16_t panelX = DIVIDER_X + 1;
    int16_t panelW = Theme::W - panelX;
    int16_t panelH = Theme::H - FOOTER_H;

    int16_t cardW = (panelW - CARD_MARGIN * 3) / 2;
    int16_t cardH = (panelH - CARD_MARGIN * 3) / 2;

    int16_t x0 = panelX + CARD_MARGIN;
    int16_t x1 = x0 + cardW + CARD_MARGIN;
    int16_t y0 = CARD_MARGIN;
    int16_t y1 = y0 + cardH + CARD_MARGIN;

    char buf[16];
    uint16_t color;
    const char *quality;

    // Card [0,0]: Temperature (number)
    if (_s.env.ok) {
        snprintf(buf, sizeof(buf), "%.1f", _s.env.temp_c);
        color = Theme::TEXT;
        if (_s.env.temp_c > 32) color = Theme::RED;
        else if (_s.env.temp_c > 28) color = Theme::YELLOW;
        else if (_s.env.temp_c < 18) color = Theme::CYAN;
    } else {
        snprintf(buf, sizeof(buf), "--");
        color = Theme::RED;
    }
    drawMiniCard(tft, x0, y0, cardW, cardH, "TEMP", buf, "C", color);

    // Card [1,0]: Humidity (number)
    if (_s.env.ok) {
        snprintf(buf, sizeof(buf), "%.0f", _s.env.humidity);
        color = Theme::TEXT;
        if (_s.env.humidity > 70) color = Theme::ORANGE;
        else if (_s.env.humidity < 30) color = Theme::YELLOW;
    } else {
        snprintf(buf, sizeof(buf), "--");
        color = Theme::RED;
    }
    drawMiniCard(tft, x1, y0, cardW, cardH, "HUMIDITY", buf, "%", color);

    // Card [0,1]: CO2 (quality text)
    if (_s.co2.ok) {
        if (_s.co2.preheating) {
            color = Theme::DIM;
            quality = "--";
        } else {
            quality = co2Quality(_s.co2.co2_ppm, color);
        }
    } else {
        color = Theme::RED;
        quality = "--";
    }
    // Show ppm number as unit
    char co2Unit[16] = "";
    if (_s.co2.ok) snprintf(co2Unit, sizeof(co2Unit), "%u ppm", _s.co2.co2_ppm);
    drawMiniCard(tft, x0, y1, cardW, cardH, "CO2", quality, co2Unit, color);

    // Card [1,1]: Air (quality text)
    if (_s.env.ok) {
        quality = airQuality(_s.env.gas_ohm, color);
    } else {
        color = Theme::RED;
        quality = "--";
    }
    char airUnit[16] = "";
    if (_s.env.ok) {
        if (_s.env.gas_ohm > 1000)
            snprintf(airUnit, sizeof(airUnit), "%.0fk", _s.env.gas_ohm / 1e3f);
        else
            snprintf(airUnit, sizeof(airUnit), "%lu", (unsigned long)_s.env.gas_ohm);
    }
    drawMiniCard(tft, x1, y1, cardW, cardH, "AIR", quality, airUnit, color);
}

void ActiveScreen::drawMiniCard(TFT_eSPI &tft, int16_t x, int16_t y,
                                 int16_t w, int16_t h,
                                 const char *label, const char *value,
                                 const char *unit, uint16_t valColor) {
    tft.fillRoundRect(x, y, w, h, CARD_R, Theme::CARD);

    int16_t cx = x + w / 2;

    // Label (top, small, grey)
    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(Theme::LABEL, Theme::CARD);
    tft.drawString(label, cx, y + 10);

    // Value (center, large, colored)
    tft.setFreeFont(FONT_LG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(valColor, Theme::CARD);
    tft.drawString(value, cx, y + h / 2 + 2);

    // Unit (bottom, small, dim)
    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(Theme::DIM, Theme::CARD);
    tft.drawString(unit, cx, y + h - 6);
}

// ── Footer — raw env | online dot ───────────────────────────────

void ActiveScreen::drawFooter(TFT_eSPI &tft) {
    int16_t ftrY = Theme::H - FOOTER_H;

    tft.fillRect(0, ftrY, Theme::W, FOOTER_H, Theme::BG);
    tft.drawFastHLine(8, ftrY, Theme::W - 16, Theme::LINE);

    int16_t textY = ftrY + 16;
    tft.setTextFont(1);

    // Left: raw env
    char footer[80];
    char *p = footer;
    int rem = sizeof(footer);
    int n;

    if (_s.lux.ok) {
        n = snprintf(p, rem, "%.0f lx", _s.lux.lux);
    } else {
        n = snprintf(p, rem, "-- lx");
    }
    p += n; rem -= n;

    if (_s.co2.ok) {
        n = snprintf(p, rem, " | %u ppm", _s.co2.co2_ppm);
    } else {
        n = snprintf(p, rem, " | -- ppm");
    }
    p += n; rem -= n;

    if (_s.env.ok) {
        snprintf(p, rem, " | %.1fC %.0f%% | %.0f hPa",
                 _s.env.temp_c, _s.env.humidity, _s.env.pressure);
    }

    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(Theme::DIM, Theme::BG);
    tft.drawString(footer, 12, textY);

    // Right: online dot
    bool online = (_netStatus[0] != '\0' && strcmp(_netStatus, "Online") == 0);
    tft.fillCircle(Theme::W - 16, textY, 4, online ? Theme::GREEN : Theme::DIM);
}
