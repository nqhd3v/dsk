// dsk-guard — SummaryScreen implementation
// 3×2 env cards centered on dark background.
// No countdown, no presence — just ambient data.

#include "SummaryScreen.h"
#include <Arduino.h>

void SummaryScreen::begin(TFT_eSPI &tft) {
    tft.fillScreen(Theme::BG);
    drawHeader(tft);
    drawCards(tft);
    _dirty = false;
}

void SummaryScreen::update(TFT_eSPI &tft, uint32_t now) {
    if (consumeDirty()) {
        begin(tft);
        return;
    }
    if (_sensorDirty) {
        _sensorDirty = false;
        drawCards(tft);
    }
}

void SummaryScreen::setSensors(const SensorBundle &s) {
    _s = s;
    _sensorDirty = true;
}

// ---- Header ----

void SummaryScreen::drawHeader(TFT_eSPI &tft) {
    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::DIM, Theme::BG);
    tft.drawString("Desk Guardian  —  Away", Theme::W / 2, HDR_H / 2);

    tft.drawFastHLine(MARGIN, HDR_H - 1, Theme::W - 2 * MARGIN, Theme::LINE);
}

// ---- Cards ----

void SummaryScreen::drawCards(TFT_eSPI &tft) {
    char buf[24];

    // Row 0: Light, Temp, Humidity
    // Light
    uint16_t luxColor = Theme::GREEN;
    if (_s.lux.ok) {
        if (_s.lux.lux < 200) luxColor = Theme::ORANGE;
        else if (_s.lux.lux > 400) luxColor = Theme::YELLOW;
        snprintf(buf, sizeof(buf), "%.0f", _s.lux.lux);
    } else {
        snprintf(buf, sizeof(buf), "--");
        luxColor = Theme::RED;
    }
    drawCard(tft, 0, 0, "Light", buf, "lux", luxColor);

    // Temp
    if (_s.env.ok)
        snprintf(buf, sizeof(buf), "%.1f", _s.env.temp_c);
    else
        snprintf(buf, sizeof(buf), "--");
    drawCard(tft, 1, 0, "Temp", buf, "C", _s.env.ok ? Theme::TEXT : Theme::RED);

    // Humidity
    if (_s.env.ok)
        snprintf(buf, sizeof(buf), "%.0f", _s.env.humidity);
    else
        snprintf(buf, sizeof(buf), "--");
    drawCard(tft, 2, 0, "Humidity", buf, "%", _s.env.ok ? Theme::CYAN : Theme::RED);

    // Row 1: CO2, Air, Pressure
    // CO2
    uint16_t co2Color = Theme::GREEN;
    if (_s.co2.ok) {
        if (_s.co2.preheating) co2Color = Theme::DIM;
        else if (_s.co2.co2_ppm > 1000) co2Color = Theme::RED;
        else if (_s.co2.co2_ppm > 800) co2Color = Theme::YELLOW;
        snprintf(buf, sizeof(buf), "%u", _s.co2.co2_ppm);
    } else {
        snprintf(buf, sizeof(buf), "--");
        co2Color = Theme::RED;
    }
    drawCard(tft, 0, 1, "CO2", buf, "ppm", co2Color);

    // Air (VOC)
    if (_s.env.ok) {
        if (_s.env.gas_ohm > 1000000)
            snprintf(buf, sizeof(buf), "%.1fM", _s.env.gas_ohm / 1e6f);
        else if (_s.env.gas_ohm > 1000)
            snprintf(buf, sizeof(buf), "%.0fk", _s.env.gas_ohm / 1e3f);
        else
            snprintf(buf, sizeof(buf), "%lu", (unsigned long)_s.env.gas_ohm);
    } else {
        snprintf(buf, sizeof(buf), "--");
    }
    drawCard(tft, 1, 1, "Air", buf, "ohm", _s.env.ok ? Theme::PURPLE : Theme::RED);

    // Pressure
    if (_s.env.ok)
        snprintf(buf, sizeof(buf), "%.0f", _s.env.pressure);
    else
        snprintf(buf, sizeof(buf), "--");
    drawCard(tft, 2, 1, "Pressure", buf, "hPa", _s.env.ok ? Theme::BLUE : Theme::RED);
}

void SummaryScreen::drawCard(TFT_eSPI &tft, uint8_t col, uint8_t row,
                              const char *label, const char *value,
                              const char *unit, uint16_t valColor) {
    int16_t x = MARGIN + col * (CARD_W + GAP);
    int16_t y = CARD_Y0 + row * (CARD_H + GAP);

    // Card background
    tft.fillRoundRect(x, y, CARD_W, CARD_H, CARD_R, Theme::CARD);

    int16_t cx = x + CARD_W / 2;

    // Label (top)
    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(Theme::LABEL, Theme::CARD);
    tft.drawString(label, cx, y + 12);

    // Value (center, large)
    tft.setFreeFont(FONT_LG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(valColor, Theme::CARD);
    tft.drawString(value, cx, y + CARD_H / 2 + 4);

    // Unit (bottom)
    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(Theme::DIM, Theme::CARD);
    tft.drawString(unit, cx, y + CARD_H - 8);
}
