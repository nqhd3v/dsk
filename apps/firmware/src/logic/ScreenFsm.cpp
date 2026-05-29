// dsk-guard — Screen FSM implementation
// Simple 2-state presence: PRESENT / ABSENT
// Countdown accumulates while present, resets after 10s away.

#include "ScreenFsm.h"

void ScreenFsm::begin(const ScreenFsmConfig &cfg) {
    _cfg = cfg;
    _mode = ScreenMode::ACTIVE;
    _modeChanged = true;
    _sitAccumMs = 0;
    _wasPresentLastTick = false;
    _absent = false;
    Serial.println(F("[FSM] Init: ACTIVE"));
    Serial.printf("[FSM] Config: sit=%lus reset=%lus summary=%lus sleep=%lus\n",
                  (unsigned long)(_cfg.sitThresholdMs / 1000),
                  (unsigned long)(_cfg.resetDelayMs / 1000),
                  (unsigned long)(_cfg.summaryDelayMs / 1000),
                  (unsigned long)(_cfg.sleepDelayMs / 1000));
}

void ScreenFsm::setMode(ScreenMode m) {
    if (_mode != m) {
        const char *names[] = {"ACTIVE", "ALERT", "SUMMARY", "SLEEP"};
        Serial.printf("[FSM] %s -> %s\n", names[(uint8_t)_mode], names[(uint8_t)m]);
        _mode = m;
        _modeChanged = true;
    }
}

void ScreenFsm::resetCountdown() {
    _sitAccumMs = 0;
    _wasPresentLastTick = false;
}

bool ScreenFsm::update(PresenceState presence, uint32_t now) {
    _modeChanged = false;

    bool wasPresent = _present;
    _present = (presence == PresenceState::PRESENT);

    // ---- Accumulate sitting time while present ----
    if (_present) {
        if (_wasPresentLastTick) {
            // Accumulate delta since last tick
            uint32_t delta = now - _lastPresentMs;
            _sitAccumMs += delta;
        }
        _lastPresentMs = now;
        _wasPresentLastTick = true;

        // Clear absent timer
        _absent = false;
    } else {
        _wasPresentLastTick = false;

        // Start/continue absent timer
        if (!_absent) {
            _absent = true;
            _absentStartMs = now;
        }
    }

    uint32_t absentMs = _absent ? (now - _absentStartMs) : 0;

    // ---- State transitions ----
    switch (_mode) {
        case ScreenMode::ACTIVE:
            if (_present) {
                // Check countdown expired → ALERT
                if (_sitAccumMs >= _cfg.sitThresholdMs) {
                    _alertStartMs = now;
                    setMode(ScreenMode::ALERT);
                }
            } else {
                // Away — check thresholds (order: sleep > summary > reset)
                if (absentMs >= _cfg.sleepDelayMs) {
                    resetCountdown();
                    setMode(ScreenMode::SLEEP);
                } else if (absentMs >= _cfg.summaryDelayMs) {
                    resetCountdown();
                    setMode(ScreenMode::SUMMARY);
                } else if (absentMs >= _cfg.resetDelayMs) {
                    // Reset countdown but stay ACTIVE
                    resetCountdown();
                }
            }
            break;

        case ScreenMode::ALERT:
            // Person returns → dismiss, restart countdown
            if (_present) {
                resetCountdown();
                setMode(ScreenMode::ACTIVE);
            }
            // Auto-dismiss after timeout
            else if ((now - _alertStartMs) >= _cfg.alertAutoDismissMs) {
                resetCountdown();
                setMode(ScreenMode::ACTIVE);
            }
            break;

        case ScreenMode::SUMMARY:
            if (_present) {
                // Person returns → ACTIVE (countdown already reset)
                setMode(ScreenMode::ACTIVE);
            } else if (absentMs >= _cfg.sleepDelayMs) {
                setMode(ScreenMode::SLEEP);
            }
            break;

        case ScreenMode::SLEEP:
            if (_present) {
                setMode(ScreenMode::ACTIVE);
            }
            break;
    }

    return _modeChanged;
}

uint32_t ScreenFsm::countdownSec() const {
    if (_sitAccumMs >= _cfg.sitThresholdMs) return 0;
    return (_cfg.sitThresholdMs - _sitAccumMs) / 1000;
}

uint32_t ScreenFsm::sittingSec() const {
    return _sitAccumMs / 1000;
}
