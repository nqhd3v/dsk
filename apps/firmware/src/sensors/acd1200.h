// dsk-guard — ACD1200 NDIR CO2 sensor driver
// UART1 @ 1200 baud, ESP TX=GPIO 17, RX=GPIO 18 (via BSS138)
// Aosong custom protocol (NOT Modbus)
// Range: 400–5000 ppm, data refresh every 2 s, preheat 120 s

#pragma once

#include <Arduino.h>

struct Co2Reading {
    uint16_t co2_ppm;   // CO2 concentration in ppm
    bool     ok;        // true if read + checksum valid
    uint32_t at_ms;     // millis() when read
    bool     preheating; // true if <120 s since power-on
};

class Acd1200Sensor {
public:
    /// Initialize sensor on a HardwareSerial port.
    /// @param serial  HardwareSerial reference (Serial1)
    /// @param rxPin   ESP RX pin (sensor TX → ESP, via BSS138)
    /// @param txPin   ESP TX pin (ESP → sensor RX)
    bool begin(HardwareSerial &serial, uint8_t rxPin = 18, uint8_t txPin = 17);

    /// Send read command and parse response. Blocking (~200 ms timeout).
    /// Call no more than once per 2 s (sensor refresh rate).
    Co2Reading read();

    bool isReady() const { return _ready; }

private:
    HardwareSerial *_serial = nullptr;
    bool     _ready    = false;
    uint32_t _bootTime = 0;

    static constexpr uint32_t PREHEAT_MS    = 120000;  // 120 s preheat
    static constexpr uint32_t READ_TIMEOUT_MS = 300;   // response timeout
    static constexpr uint8_t  REPLY_LEN     = 9;       // expected reply bytes

    // UART protocol constants
    static constexpr uint8_t FRAME_HEAD  = 0xFE;
    static constexpr uint8_t FIXED_CODE  = 0xA6;

    // Read CO2 command: FE A6 00 01 A7
    static constexpr uint8_t CMD_READ_CO2[] = {0xFE, 0xA6, 0x00, 0x01, 0xA7};

    /// Compute checksum: sum of all bytes after FRAME_HEAD, masked to 8 bits.
    static uint8_t checksum(const uint8_t *data, uint8_t len);
};
