// dsk-guard — Shared radar types
// Used by both LD2410C and LD2410S drivers.
// ScreenFsm and UI code depend on these types, not on a specific sensor.

#pragma once

#include <Arduino.h>

enum class PresenceState : uint8_t {
    ABSENT,              // No target detected (or distance > threshold)
    PRESENT              // Person detected within range
};

struct RadarReading {
    PresenceState state;
    uint16_t distance_cm;            // smoothed detection distance
    uint16_t raw_distance_cm;        // unsmoothed raw distance
    uint8_t  raw_state;              // raw sensor state byte (debug)
    bool     ot2;                    // OT2 digital pin state (if wired)
    bool     ok;
    uint32_t at_ms;
};
