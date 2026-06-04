// dsk-guard — LD2450 driver implementation
//
// Raw UART frame parser — no external library needed.
// Protocol: HLK-LD2450 Instruction Manual V1.00, §6.
//
// Frame (30 bytes @ 10 fps):
//   AA FF 03 00 | t1(8) | t2(8) | t3(8) | 55 CC
// Per target (8 bytes LE): X(int16) Y(int16) speed(int16) distRes(uint16)
// Sign-magnitude: bit15=1 → +(raw&0x7FFF), bit15=0 → -(raw). All-zero = no target.
//
// Datasheet worked example (target 1):
//   X = 0x030E → -782 mm,  Y = 0x86B1 → +1713 mm,
//   speed = 0x0010 → -16 cm/s, distRes = 0x0140 → 320 mm.

#include "ld2450.h"
#include <math.h>

constexpr uint8_t Ld2450Sensor::HEADER[4];
constexpr uint8_t Ld2450Sensor::TAIL[2];

bool Ld2450Sensor::begin(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin) {
    _serial = &serial;
    _rxPin = rxPin;
    _txPin = txPin;

    // LD2450 default UART: 256000 baud, 8 data, no parity, 1 stop.
    _serial->begin(256000, SERIAL_8N1, rxPin, txPin);

    delay(500);  // let radar boot

    int preBytes = _serial->available();
    Serial.printf("[LD2450] Pre-init: %d bytes in UART RX buffer (GPIO %d)\n", preBytes, rxPin);

    if (preBytes == 0) {
        Serial.println(F("[LD2450] WARNING: No data from radar. Check wiring:"));
        Serial.println(F("  - VCC = 5V (supply >200mA)"));
        Serial.printf("  - Radar TX → ESP GPIO %d (RX)\n", rxPin);
        Serial.printf("  - ESP GPIO %d (TX) → Radar RX\n", txPin);
        Serial.println(F("  - Common GND, baud 256000"));
    }

    _ready = true;
    _dataReceived = false;
    _framePos = 0;
    _headMatch = 0;
    _frameCount = 0;
    _targetCount = 0;
    _nearestCm = 0;
    _distBufIdx = 0;
    _distBufCount = 0;
    _smoothDist = 0;
    _lastDiagMs = millis();

    Serial.printf("[LD2450] Init OK. UART2 @ 256000, RX=%d TX=%d range=%ucm\n",
                  rxPin, txPin, _cfg.maxRangeCm);
    return true;
}

int16_t Ld2450Sensor::decodeSigned(uint16_t raw) {
    // bit15 set → positive magnitude; clear → negative.
    if (raw & 0x8000) return (int16_t)(raw & 0x7FFF);
    return (int16_t)(-(int32_t)raw);
}

void Ld2450Sensor::feedByte(uint8_t b) {
    // Sync on 4-byte header AA FF 03 00, then collect to 30 bytes, verify tail.
    if (_framePos < 4) {
        if (b == HEADER[_framePos]) {
            _frameBuf[_framePos++] = b;
        } else if (b == HEADER[0]) {
            // Possible restart of header
            _frameBuf[0] = b;
            _framePos = 1;
        } else {
            _framePos = 0;
        }
        return;
    }

    _frameBuf[_framePos++] = b;
    if (_framePos == FRAME_LEN) {
        if (_frameBuf[28] == TAIL[0] && _frameBuf[29] == TAIL[1]) {
            parseFrame();
        }
        _framePos = 0;
    }
}

void Ld2450Sensor::parseFrame() {
    _frameCount++;
    _targetCount = 0;
    uint16_t nearest = 0xFFFF;

    for (uint8_t i = 0; i < 3; i++) {
        const uint8_t *p = &_frameBuf[4 + i * 8];
        uint16_t rx  = p[0] | (p[1] << 8);
        uint16_t ry  = p[2] | (p[3] << 8);
        uint16_t rs  = p[4] | (p[5] << 8);
        uint16_t res = p[6] | (p[7] << 8);

        Ld2450Target &t = _targets[i];
        // All-zero block → no target in this slot.
        if (rx == 0 && ry == 0 && rs == 0 && res == 0) {
            t = Ld2450Target{};
            continue;
        }

        t.x_mm      = decodeSigned(rx);
        t.y_mm      = decodeSigned(ry);
        t.speed_cms = decodeSigned(rs);
        t.res_mm    = res;
        float d_mm  = sqrtf((float)t.x_mm * t.x_mm + (float)t.y_mm * t.y_mm);
        t.dist_cm   = (uint16_t)(d_mm / 10.0f + 0.5f);
        t.valid     = true;
        _targetCount++;

        if (t.dist_cm < nearest) nearest = t.dist_cm;
    }

    _nearestCm = (nearest == 0xFFFF) ? 0 : nearest;
    if (_targetCount > 0) updateSmooth(_nearestCm);

    if (!_dataReceived) {
        _dataReceived = true;
        Serial.printf("[LD2450] First frame! targets=%u nearest=%ucm\n",
                      _targetCount, _nearestCm);
    }
}

void Ld2450Sensor::updateSmooth(uint16_t distCm) {
    uint8_t win = min(_cfg.smoothWindow, MAX_SMOOTH);
    if (win == 0) win = 1;

    _distBuf[_distBufIdx] = distCm;
    _distBufIdx = (_distBufIdx + 1) % win;
    if (_distBufCount < win) _distBufCount++;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < _distBufCount; i++) sum += _distBuf[i];
    _smoothDist = (uint16_t)(sum / _distBufCount);
}

void Ld2450Sensor::poll() {
    if (!_ready || !_serial) return;

    while (_serial->available()) {
        feedByte(_serial->read());
    }

    uint32_t now = millis();
    if (now - _lastDiagMs >= 10000) {
        _lastDiagMs = now;
        if (!_dataReceived) {
            Serial.printf("[LD2450] DIAG: No data after %lus. RX=%d avail=%d\n",
                          (unsigned long)(now / 1000), _rxPin, _serial->available());
        } else {
            Serial.printf("[LD2450] DIAG: frames=%lu targets=%u nearest=%u smooth=%u\n",
                          (unsigned long)_frameCount, _targetCount, _nearestCm, _smoothDist);
        }
    }
}

RadarReading Ld2450Sensor::read() {
    RadarReading r = {};
    r.at_ms = millis();

    if (!_ready || !_dataReceived || _frameCount == 0) {
        r.ok = false;
        r.state = PresenceState::ABSENT;
        return r;
    }

    r.ok = true;
    r.distance_cm     = _smoothDist;
    r.raw_distance_cm = _nearestCm;
    r.raw_state       = _targetCount;  // # of tracked targets (debug)
    r.ot2             = false;          // no OT2 pin on LD2450

    // Presence = ≥1 target within desk range.
    if (_targetCount > 0 && _smoothDist > 0 && _smoothDist <= _cfg.maxRangeCm) {
        r.state = PresenceState::PRESENT;
    } else {
        r.state = PresenceState::ABSENT;
        // Clear smoothing buffer on absence.
        _distBufCount = 0;
        _distBufIdx = 0;
    }

    return r;
}
