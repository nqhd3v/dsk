// dsk-guard — AlertScreen implementation
// Full-screen pulsing "Stand up!" reminder.
// Pulses between RED and ORANGE every 800 ms.
// Auto-dismissed by FSM on movement or timeout.

#include "AlertScreen.h"
#include <Arduino.h>

void AlertScreen::begin(TFT_eSPI &tft) {
    tft.fillScreen(Theme::BG);
    _pulseHigh = true;
    _lastPulse = millis();
    drawContent(tft, Theme::RED);
    _dirty = false;
}

void AlertScreen::update(TFT_eSPI &tft, uint32_t now) {
    if (consumeDirty()) {
        begin(tft);
        return;
    }

    // Pulse animation
    if (now - _lastPulse >= PULSE_MS) {
        _lastPulse = now;
        _pulseHigh = !_pulseHigh;
        drawContent(tft, _pulseHigh ? Theme::RED : Theme::ORANGE);
    }
}

void AlertScreen::drawContent(TFT_eSPI &tft, uint16_t accentColor) {
    int16_t cx = Theme::W / 2;

    // Big icon area — exclamation in circle
    int16_t circleY = 100;
    tft.fillCircle(cx, circleY, 40, accentColor);
    tft.setFreeFont(FONT_XL);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::BG, accentColor);
    tft.drawString("!", cx, circleY);

    // "Stand up!" text
    tft.fillRect(0, 155, Theme::W, 50, Theme::BG);
    tft.setFreeFont(FONT_XL);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(accentColor, Theme::BG);
    tft.drawString("Stand up!", cx, 160);

    // Subtitle
    tft.fillRect(0, 210, Theme::W, 30, Theme::BG);
    tft.setFreeFont(FONT_MD);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(Theme::LABEL, Theme::BG);
    char buf[32];
    snprintf(buf, sizeof(buf), "You've been sitting %lu min", (unsigned long)_sittingMin);
    tft.drawString(buf, cx, 215);

    // Hint
    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(Theme::DIM, Theme::BG);
    tft.drawString("Move to dismiss", cx, 260);
}
