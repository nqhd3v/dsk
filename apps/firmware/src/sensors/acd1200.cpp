// dsk-guard — ACD1200 driver implementation
//
// UART protocol (SET pin = GND):
//   Baud: 1200, 8N1
//   Read CO2 send:  FE A6 00 01 A7
//   Read CO2 reply: FE A6 04 01 D1 D2 D3 D4 CS
//     CO2 ppm = D1*256 + D2  (D3, D4 reserved)
//     CS = (A6 + 04 + 01 + D1 + D2 + D3 + D4) & 0xFF

#include "acd1200.h"

// Static member definition
constexpr uint8_t Acd1200Sensor::CMD_READ_CO2[];

bool Acd1200Sensor::begin(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin) {
    _serial = &serial;
    _bootTime = millis();

    _serial->begin(1200, SERIAL_8N1, rxPin, txPin);

    // Flush any stale data
    delay(100);
    while (_serial->available()) {
        _serial->read();
    }

    _ready = true;
    Serial.printf("[ACD1200] Init OK. UART1 @ 1200 baud, RX=%d TX=%d\n", rxPin, txPin);
    Serial.println(F("[ACD1200] Preheat: 120 s from power-on for accurate readings."));
    return true;
}

Co2Reading Acd1200Sensor::read() {
    Co2Reading r;
    r.at_ms = millis();
    r.co2_ppm = 0;
    r.ok = false;
    r.preheating = (millis() - _bootTime) < PREHEAT_MS;

    if (!_ready || !_serial) {
        return r;
    }

    // Flush RX buffer before sending command
    while (_serial->available()) {
        _serial->read();
    }

    // Send read CO2 command
    _serial->write(CMD_READ_CO2, sizeof(CMD_READ_CO2));
    _serial->flush();  // wait for TX to complete

    // Wait for response (9 bytes at 1200 baud ≈ 75 ms)
    uint8_t buf[REPLY_LEN];
    uint8_t idx = 0;
    uint32_t start = millis();

    while (idx < REPLY_LEN && (millis() - start) < READ_TIMEOUT_MS) {
        if (_serial->available()) {
            buf[idx++] = _serial->read();
        }
    }

    // Debug: print raw bytes
    Serial.printf("[ACD1200] RX %d bytes:", idx);
    for (uint8_t i = 0; i < idx; i++) {
        Serial.printf(" %02X", buf[i]);
    }
    Serial.println();

    // Validate response
    if (idx < REPLY_LEN) {
        Serial.printf("[ACD1200] Timeout — got %d/%d bytes\n", idx, REPLY_LEN);
        return r;
    }

    // Check frame header: FE A6
    if (buf[0] != FRAME_HEAD || buf[1] != FIXED_CODE) {
        Serial.printf("[ACD1200] Bad header: %02X %02X (expected FE A6)\n", buf[0], buf[1]);
        return r;
    }

    // Check length byte = 0x04 and command byte = 0x01
    if (buf[2] != 0x04 || buf[3] != 0x01) {
        Serial.printf("[ACD1200] Bad len/cmd: %02X %02X (expected 04 01)\n", buf[2], buf[3]);
        return r;
    }

    // Verify checksum: sum of bytes [1..7] & 0xFF should equal byte [8]
    uint8_t cs = checksum(&buf[1], 7);
    if (cs != buf[8]) {
        Serial.printf("[ACD1200] Checksum fail: calc=%02X got=%02X\n", cs, buf[8]);
        return r;
    }

    // Parse CO2: D1=buf[4], D2=buf[5]
    r.co2_ppm = ((uint16_t)buf[4] << 8) | buf[5];
    r.ok = true;

    return r;
}

uint8_t Acd1200Sensor::checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}
