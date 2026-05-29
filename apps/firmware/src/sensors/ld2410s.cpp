// dsk-guard — LD2410S driver implementation
//
// Raw UART frame parser — no external library needed.
// Protocol: HLK-LD2410S Serial Communication Protocol V1.00
//
// LD2410S has two output modes (see §2.1):
//
//   MINIMAL (default):
//     6E  SS  DL DH  62        (5 bytes)
//     SS = target state: 0/1=no one, 2/3=someone
//     DL DH = distance in cm (little-endian)
//
//   STANDARD (requires cmd 0x007A to switch):
//     F4 F3 F2 F1  LL LL  01  SS  DL DH  RR RR  [64B energy]  F8 F7 F6 F5
//
// We parse the MINIMAL format since that's the factory default.
//
// Presence model (simple 2-state):
//   PRESENT = sensor says someone + smoothed distance ≤ maxRangeCm
//   ABSENT  = no one OR distance > maxRangeCm

#include "ld2410s.h"

bool Ld2410sSensor::begin(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin,
                          uint8_t ot2Pin) {
    _serial = &serial;
    _rxPin = rxPin;
    _txPin = txPin;
    _ot2Pin = ot2Pin;

    _serial->begin(115200, SERIAL_8N1, rxPin, txPin);

    // Setup OT2 digital presence pin if provided
    if (_ot2Pin != 0xFF) {
        pinMode(_ot2Pin, INPUT);
        _ot2Present = digitalRead(_ot2Pin) == HIGH;
        Serial.printf("[LD2410S] OT2 pin=%d initial=%s\n", _ot2Pin,
                      _ot2Present ? "PRESENT" : "ABSENT");
    }

    delay(500);  // Let radar boot

    int preBytes = _serial->available();
    Serial.printf("[LD2410S] Pre-init: %d bytes in UART RX buffer (GPIO %d)\n", preBytes, rxPin);

    if (preBytes == 0) {
        Serial.println(F("[LD2410S] WARNING: No data from radar. Check wiring:"));
        Serial.println(F("  - VCC = 3.3V (NOT 5V!)"));
        Serial.printf("  - Radar TX (OT1) → ESP GPIO %d (RX)\n", rxPin);
        Serial.printf("  - ESP GPIO %d (TX) → Radar RX\n", txPin);
        Serial.println(F("  - Common GND"));
    }

    _ready = true;
    _dataReceived = false;
    _parseState = WAIT_HEAD;
    _framePos = 0;
    _frameCount = 0;
    _distBufIdx = 0;
    _distBufCount = 0;
    _smoothDist = 0;
    _lastDiagMs = millis();

    Serial.printf("[LD2410S] Init OK. UART2 @ 115200, RX=%d TX=%d OT2=%d range=%ucm\n",
                  rxPin, txPin, ot2Pin, _cfg.maxRangeCm);
    return true;
}

void Ld2410sSensor::feedByte(uint8_t b) {
    // Minimal frame: 6E [state] [dist_lo] [dist_hi] 62  (5 bytes fixed)
    switch (_parseState) {
        case WAIT_HEAD:
            if (b == 0x6E) {
                _frameBuf[0] = b;
                _framePos = 1;
                _parseState = IN_BODY;
            }
            break;
        case IN_BODY:
            _frameBuf[_framePos++] = b;
            if (_framePos == 5) {
                if (_frameBuf[4] == 0x62) {
                    parseFrame();
                }
                _parseState = WAIT_HEAD;
                _framePos = 0;
            }
            break;
    }
}

bool Ld2410sSensor::parseFrame() {
    // Minimal frame: 6E [state] [dist_lo] [dist_hi] 62
    _targetState = _frameBuf[1];
    _distance = _frameBuf[2] | (_frameBuf[3] << 8);
    _frameCount++;

    // Feed distance into moving average
    updateSmooth(_distance);

    if (!_dataReceived) {
        _dataReceived = true;
        Serial.printf("[LD2410S] First frame! state=%u dist=%ucm ot2=%d\n",
                      _targetState, _distance, _ot2Present ? 1 : 0);
    }

    return true;
}

void Ld2410sSensor::updateSmooth(uint16_t dist) {
    uint8_t win = min(_cfg.smoothWindow, MAX_SMOOTH);
    if (win == 0) win = 1;

    _distBuf[_distBufIdx] = dist;
    _distBufIdx = (_distBufIdx + 1) % win;
    if (_distBufCount < win) _distBufCount++;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < _distBufCount; i++) sum += _distBuf[i];
    _smoothDist = (uint16_t)(sum / _distBufCount);
}

void Ld2410sSensor::poll() {
    if (!_ready || !_serial) return;

    // Read OT2 digital pin (instant presence, no delay)
    if (_ot2Pin != 0xFF) {
        _ot2Present = digitalRead(_ot2Pin) == HIGH;
    }

    // Read all available UART bytes
    while (_serial->available()) {
        feedByte(_serial->read());
    }

    // Periodic diagnostic
    uint32_t now = millis();
    if (now - _lastDiagMs >= 10000) {
        _lastDiagMs = now;
        if (!_dataReceived) {
            Serial.printf("[LD2410S] DIAG: No data after %lus. RX=%d avail=%d\n",
                          (unsigned long)(now / 1000), _rxPin, _serial->available());
        } else {
            Serial.printf("[LD2410S] DIAG: frames=%lu st=%u dist=%u smooth=%u ot2=%d\n",
                          (unsigned long)_frameCount, _targetState, _distance,
                          _smoothDist, _ot2Present ? 1 : 0);
        }
    }
}

RadarReading Ld2410sSensor::read() {
    RadarReading r = {};
    r.at_ms = millis();

    if (!_ready || !_dataReceived || _frameCount == 0) {
        r.ok = false;
        r.state = PresenceState::ABSENT;
        return r;
    }

    r.ok = true;
    r.distance_cm = _smoothDist;
    r.raw_distance_cm = _distance;
    r.raw_state = _targetState;
    r.ot2 = _ot2Present;

    // Presence = sensor says someone + within desk range
    bool sensorSaysSomeone;
    if (_ot2Pin != 0xFF) {
        sensorSaysSomeone = _ot2Present;
    } else {
        sensorSaysSomeone = (_targetState >= 2);
    }

    if (sensorSaysSomeone && _smoothDist <= _cfg.maxRangeCm) {
        r.state = PresenceState::PRESENT;
    } else {
        r.state = PresenceState::ABSENT;
        // Clear smoothing buffer on absence
        _distBufCount = 0;
        _distBufIdx = 0;
    }

    return r;
}
