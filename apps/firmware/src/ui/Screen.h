// dsk-guard — Screen base class
// All screens inherit from this. ScreenManager (future) will switch between them.
//
// Contract:
//   begin()  — one-time init, draw static elements
//   update() — called every frame (~10 Hz), redraw only dirty regions
//   dirty()  — mark screen for full redraw (e.g. on screen switch)

#pragma once

#include <TFT_eSPI.h>

class Screen {
public:
    virtual ~Screen() = default;

    /// Called once when screen becomes active. Draw full layout.
    virtual void begin(TFT_eSPI &tft) = 0;

    /// Called every frame. Only redraw what changed.
    /// @param tft  display reference
    /// @param now  millis() timestamp for animations/timers
    virtual void update(TFT_eSPI &tft, uint32_t now) = 0;

    /// Mark entire screen dirty — next update() redraws everything.
    void dirty() { _dirty = true; }

    /// Check and consume dirty flag.
    bool consumeDirty() {
        bool d = _dirty;
        _dirty = false;
        return d;
    }

protected:
    bool _dirty = true;
};
