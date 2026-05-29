// dsk-guard — BME680 driver implementation

#include "bme680.h"

bool Bme680Sensor::begin() {
    _ready = _bme.begin(0x77);
    if (!_ready) {
        Serial.println(F("[BME680] Init FAILED — check wiring / I2C addr 0x77"));
        return false;
    }

    // Oversampling + filter config (Adafruit defaults are fine for indoor)
    _bme.setTemperatureOversampling(BME680_OS_8X);
    _bme.setHumidityOversampling(BME680_OS_2X);
    _bme.setPressureOversampling(BME680_OS_4X);
    _bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

    // Gas heater: 320C for 150 ms
    _bme.setGasHeater(320, 150);

    Serial.println(F("[BME680] Init OK @ 0x77. Gas heater needs 5-20 min warmup."));
    return true;
}

EnvReading Bme680Sensor::read() {
    EnvReading r;
    r.at_ms = millis();

    if (!_ready) {
        r.ok = false;
        return r;
    }

    // performReading() is blocking (~180 ms due to gas heater)
    if (!_bme.performReading()) {
        Serial.println(F("[BME680] Read failed"));
        r.ok = false;
        return r;
    }

    r.temp_c   = _bme.temperature;
    r.humidity  = _bme.humidity;
    r.pressure  = _bme.pressure / 100.0f;  // Pa → hPa
    r.gas_ohm   = _bme.gas_resistance;
    r.ok        = true;

    return r;
}
