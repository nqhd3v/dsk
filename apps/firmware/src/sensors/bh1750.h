// dsk-guard — BH1750 ambient light sensor driver
// I2C @ 0x23, SDA GPIO 8, SCL GPIO 9
// Returns lux (0–65535 range, typical indoor 200–400)

#pragma once

#include <Arduino.h>
#include <BH1750.h>

struct LuxReading {
    float    lux;       // lux value
    bool     ok;        // true if read succeeded
    uint32_t at_ms;     // millis() when read
};

class Bh1750Sensor {
public:
    /// Initialize sensor. Call after Wire.begin().
    /// @return true if sensor responded on I2C
    bool begin();

    /// Read current lux value.
    LuxReading read();

    /// Is sensor present on bus?
    bool isReady() const { return _ready; }

private:
    BH1750 _sensor{0x23};
    bool   _ready   = false;
    float  _lastLux = 0;
};
