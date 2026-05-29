// dsk-guard — LD2410C driver implementation
//
// BUG in ncmreynolds/ld2410 v0.1.3: parse_data_frame_() swaps fields.
//
// Datasheet byte layout (basic target data frame, after header):
//   [8]  target_type: 0=none, 1=moving, 2=stationary, 3=both
//   [9-10]  moving distance (cm)       → lib stores as stationary_target_distance_  ← WRONG
//   [11]    moving energy (0-100)      → lib stores as moving_target_energy_        ← OK
//   [12-13] stationary distance (cm)   → lib DOES NOT READ THIS AT ALL             ← MISSING
//   [14]    stationary energy (0-100)  → lib stores as stationary_target_energy_    ← OK
//   [15-16] detection distance (cm)    → lib stores as moving_target_distance_      ← WRONG
//
// Critical consequence: stationaryTargetDetected() checks swapped distance field,
// returns FALSE when person sits still (target_type=0x02 but moving_dist=0).
//
// Fix: bypass library's movingTargetDetected()/stationaryTargetDetected().
// Use presenceDetected() (correct — checks target_type_ != 0) + energies (correct bytes).
// Determine moving/stationary from energy values, which are in the right byte positions.

#include "ld2410c.h"

bool Ld2410cSensor::begin(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin) {
    _serial = &serial;
    _rxPin = rxPin;
    _txPin = txPin;

    // Init UART2 at 256000 baud
    _serial->begin(256000, SERIAL_8N1, rxPin, txPin);

    // Give radar time to boot — LD2410C needs ~200ms after power-on
    delay(500);

    // Check if any bytes coming from radar BEFORE handshake
    int preBytes = _serial->available();
    Serial.printf("[LD2410C] Pre-handshake: %d bytes in UART RX buffer (GPIO %d)\n", preBytes, rxPin);

    if (preBytes == 0) {
        Serial.println(F("[LD2410C] WARNING: No data from radar. Check wiring:"));
        Serial.println(F("  - Is 5V reaching radar VCC?"));
        Serial.printf("  - Radar TX → ESP GPIO %d (RX)?\n", rxPin);
        Serial.printf("  - ESP GPIO %d (TX) → Radar RX?\n", txPin);
        Serial.println(F("  - Common GND?"));
        Serial.println(F("  - Not swapped TX↔RX?"));
    }

    // Try full handshake (sends firmware version request)
    _ready = _radar.begin(*_serial, true);
    if (_ready) {
        Serial.printf("[LD2410C] Init OK (handshake). UART2 @ 256000, RX=%d TX=%d\n", rxPin, txPin);
    } else {
        Serial.println(F("[LD2410C] Handshake failed — radar not responding to commands."));
        // Fall back to passive mode: just read data frames without commanding.
        // NOTE: begin(stream, false) always returns true — it doesn't actually verify data flow.
        _radar.begin(*_serial, false);
        _ready = true;  // We'll validate via _dataReceived flag in poll()
        _dataReceived = false;
        Serial.println(F("[LD2410C] Passive mode enabled. Waiting for data frames..."));
    }
    return _ready;
}

void Ld2410cSensor::poll() {
    if (!_ready) return;

    // Track raw bytes for diagnostics
    int avail = _serial->available();
    if (avail > 0 && !_dataReceived) {
        _dataReceived = true;
        Serial.printf("[LD2410C] First data received! %d bytes in buffer\n", avail);
    }

    bool parsed = _radar.read();
    if (parsed) {
        _frameCount++;
    }

    // Periodic diagnostic (every 10s from begin)
    uint32_t now = millis();
    if (now - _lastDiagMs >= 10000) {
        _lastDiagMs = now;
        if (!_dataReceived) {
            Serial.printf("[LD2410C] DIAG: Still no UART data after %lus. UART RX=%d avail=%d\n",
                          (unsigned long)(now / 1000), _rxPin, _serial->available());
        } else {
            Serial.printf("[LD2410C] DIAG: frames=%lu presence=%d type=0x%02X mov_e=%u stat_e=%u\n",
                          (unsigned long)_frameCount,
                          _radar.presenceDetected() ? 1 : 0,
                          0,  // can't access target_type_ directly
                          _radar.movingTargetEnergy(),
                          _radar.stationaryTargetEnergy());
        }
    }
}

RadarReading Ld2410cSensor::read() {
    RadarReading r;
    r.at_ms = millis();
    r.moving_distance_cm     = 0;
    r.moving_energy          = 0;
    r.stationary_distance_cm = 0;
    r.stationary_energy      = 0;
    r.detection_distance_cm  = 0;

    if (!_ready || !_dataReceived) {
        r.ok = false;
        r.state = PresenceState::ABSENT;
        return r;
    }

    // Only report ok=true after we've parsed at least one frame
    r.ok = (_frameCount > 0);

    // --- Bypass library's broken movingTargetDetected()/stationaryTargetDetected() ---
    // Those methods check swapped distance fields and fail for stationary-only targets.
    //
    // Instead: use presenceDetected() (correct: checks target_type_ != 0)
    // + energies (correct byte positions) to determine state.
    bool hasPresence = _radar.presenceDetected();

    // Energies ARE in correct byte positions in the library
    uint8_t movEnergy  = _radar.movingTargetEnergy();
    uint8_t statEnergy = _radar.stationaryTargetEnergy();

    // Determine presence state from energy values
    // Energy > 0 means that target type is detected
    bool moving     = hasPresence && (movEnergy > 0);
    bool stationary = hasPresence && (statEnergy > 0);

    if (moving && stationary) {
        r.state = PresenceState::PRESENT_BOTH;
    } else if (moving) {
        r.state = PresenceState::PRESENT_MOVING;
    } else if (stationary) {
        r.state = PresenceState::PRESENT_STATIONARY;
    } else if (hasPresence) {
        // target_type_ says present but both energies are 0 — treat as stationary
        // (radar sometimes reports type=2 with energy=0 briefly during transitions)
        r.state = PresenceState::PRESENT_STATIONARY;
    } else {
        r.state = PresenceState::ABSENT;
    }

    // Distance correction (swap back from library's wrong mapping)
    // lib.stationaryTargetDistance() = bytes [9-10]  = actual MOVING distance
    // lib.movingTargetDistance()     = bytes [15-16] = actual DETECTION distance
    // Actual stationary distance [12-13] not exposed by library
    r.moving_distance_cm     = _radar.stationaryTargetDistance();  // lib [9-10] = actual moving
    r.stationary_distance_cm = 0;  // not available from library
    r.detection_distance_cm  = _radar.movingTargetDistance();      // lib [15-16] = actual detection

    r.moving_energy    = movEnergy;
    r.stationary_energy = statEnergy;

    return r;
}
