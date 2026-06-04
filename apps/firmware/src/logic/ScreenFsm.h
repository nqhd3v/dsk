// dsk-guard — Screen state machine
// 2-state presence: PRESENT or ABSENT (from radar driver)
//
// Screen modes:
//   ACTIVE  — person at desk, countdown to sit reminder
//   ALERT   — countdown expired, stand-up reminder
//   SUMMARY — away >60s, show env cards
//   SLEEP   — away >5min, backlight off
//
// Transitions:
//   ACTIVE  → ALERT    countdown reaches 0
//   ALERT   → ACTIVE   person leaves desk (absent) — stays until then
//   ACTIVE  → ACTIVE   away >10s resets countdown
//   ACTIVE  → SUMMARY  away >60s
//   SUMMARY → SLEEP    away >5min
//   SUMMARY → ACTIVE   person returns
//   SLEEP   → ACTIVE   person returns

#pragma once

#include <Arduino.h>
#include "../sensors/radar_types.h"

enum class ScreenMode : uint8_t {
    ACTIVE,
    ALERT,
    SUMMARY,
    SLEEP
};

struct ScreenFsmConfig {
    uint32_t sitThresholdMs      = 45UL * 60 * 1000;  // 45 min → alert
    uint32_t resetDelayMs        = 10UL * 1000;        // 10 s away → reset countdown
    uint32_t summaryDelayMs      = 60UL * 1000;        // 60 s away → summary
    uint32_t sleepDelayMs        = 5UL  * 60 * 1000;   // 5 min away → sleep
    uint32_t alertAutoDismissMs  = 30UL * 1000;        // auto-dismiss alert
};

class ScreenFsm {
public:
    void begin(const ScreenFsmConfig &cfg = {});

    /// Call every loop iteration. Returns true if screen mode changed.
    bool update(PresenceState presence, uint32_t now);

    ScreenMode mode() const { return _mode; }

    /// Seconds remaining on sit countdown (0 when not active or expired).
    uint32_t countdownSec() const;

    /// Seconds spent sitting continuously.
    uint32_t sittingSec() const;

    /// Is person present?
    bool isPresent() const { return _present; }

    void forceMode(ScreenMode m) { _mode = m; _modeChanged = true; }
    bool modeChanged() const { return _modeChanged; }

private:
    ScreenFsmConfig _cfg;
    ScreenMode _mode = ScreenMode::ACTIVE;
    bool _modeChanged = false;
    bool _present = false;

    // Sitting countdown — runs while present
    uint32_t _sitAccumMs = 0;     // total ms spent present (sitting)
    uint32_t _lastPresentMs = 0;  // last time we saw presence (for delta)
    bool     _wasPresentLastTick = false;

    // Absent timer
    uint32_t _absentStartMs = 0;
    bool     _absent = false;

    // Alert
    uint32_t _alertStartMs = 0;

    void setMode(ScreenMode m);
    void resetCountdown();
};
