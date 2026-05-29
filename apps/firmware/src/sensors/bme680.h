// dsk-guard — BME680 environment sensor driver
// I2C @ 0x76, SDA GPIO 8, SCL GPIO 9
// Reads: temperature (C), relative humidity (%), pressure (hPa), gas resistance (ohms)
// Note: raw gas resistance only — BSEC2 IAQ index is optional future enhancement.
// Gas heater needs 5–20 min warmup for stable readings.

#pragma once

#include <Arduino.h>
#include <Adafruit_BME680.h>

struct EnvReading {
    float    temp_c;      // Celsius
    float    humidity;    // %RH
    float    pressure;   // hPa
    uint32_t gas_ohm;    // gas resistance in ohms (higher = cleaner air)
    bool     ok;
    uint32_t at_ms;
};

class Bme680Sensor {
public:
    /// Initialize sensor. Call after Wire.begin().
    bool begin();

    /// Trigger + read all channels. Blocking (~180 ms for gas heater).
    EnvReading read();

    bool isReady() const { return _ready; }

private:
    Adafruit_BME680 _bme;
    bool _ready = false;
};
