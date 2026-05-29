// dsk-guard — BH1750 driver implementation

#include "bh1750.h"
#include <Wire.h>

bool Bh1750Sensor::begin() {
    // Wire.begin() must be called before this — done in main.cpp
    _ready = _sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    if (_ready) {
        Serial.println(F("[BH1750] Init OK @ 0x23"));
    } else {
        Serial.println(F("[BH1750] Init FAILED — check wiring / I2C addr"));
    }
    return _ready;
}

LuxReading Bh1750Sensor::read() {
    LuxReading r;
    r.at_ms = millis();

    if (!_ready) {
        r.ok  = false;
        r.lux = 0;
        return r;
    }

    if (_sensor.measurementReady()) {
        r.lux = _sensor.readLightLevel();
        r.ok  = (r.lux >= 0);  // negative = error
        if (!r.ok) {
            Serial.println(F("[BH1750] Read error (negative lux)"));
        }
    } else {
        // Measurement not ready yet — return stale-but-ok
        r.lux = _lastLux;
        r.ok  = true;
    }

    if (r.ok) _lastLux = r.lux;
    return r;
}
