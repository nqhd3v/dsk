// dsk-guard — LD2450 mmWave motion-target tracking radar driver
// UART2 @ 256000 baud, ESP TX=GPIO 15, RX=GPIO 16
// 24 GHz FMCW, 1T2R, azimuth ±60° / pitch ±35°, range 6 m, 10 Hz refresh
//
// *** VCC = 5V (supply >200mA, avg 120mA). IO level = 3.3V. ***
//     5V power, but TX/RX signals are 3.3V → direct wire to ESP OK, no shifter.
//
// Unlike LD2410S, LD2450 tracks up to THREE moving targets and reports each
// target's X/Y position (mm), speed (cm/s) and distance gate resolution (mm).
// There is NO OT2 digital presence pin on this module (only 5V/GND/TX/RX).
//
// Drop-in compatible with RadarReading (PRESENT/ABSENT + distance_cm) so it can
// replace LD2410S in ScreenFsm/UI without changes. Extra per-target data is
// exposed via accessors for future use (zone logic, multi-person, approach/leave).
//
// Protocol: HLK-LD2450 Instruction Manual V1.00, §6 Communication protocols
//   Frame (30 bytes, 10 fps):
//     AA FF 03 00 | target1(8) | target2(8) | target3(8) | 55 CC
//   Per target (8 bytes, little-endian):
//     X(int16) Y(int16) speed(int16) distRes(uint16)
//   Sign-magnitude encoding for X/Y/speed:
//     bit15 = 1 → positive value = (raw & 0x7FFF)
//     bit15 = 0 → negative value = -(raw)
//   distRes is plain uint16 (mm). An all-zero 8-byte block = no target.
//
// Presence model (simple 2-state, same as LD2410S):
//   PRESENT = ≥1 valid target AND nearest target distance ≤ maxRangeCm
//   ABSENT  = no target OR nearest distance > maxRangeCm

#pragma once

#include "radar_types.h"

struct Ld2450Target {
    int16_t  x_mm     = 0;   // +right / -left of radar normal
    int16_t  y_mm     = 0;   // distance forward from radar plane
    int16_t  speed_cms = 0;  // + approaching, - receding
    uint16_t res_mm   = 0;   // distance gate resolution
    uint16_t dist_cm  = 0;   // sqrt(x²+y²) in cm
    bool     valid    = false;
};

struct Ld2450Config {
    uint16_t maxRangeCm   = 150;    // beyond this = ABSENT (desk presence range)
    uint8_t  smoothWindow = 5;      // moving average window for nearest distance
    uint32_t holdMs       = 30000;  // keep PRESENT this long after last in-range
                                    // detection — LD2450 drops dead-still targets,
                                    // so linger to avoid false "Away" while sitting
};

class Ld2450Sensor {
public:
    /// Initialize radar on UART2.
    /// @param serial  HardwareSerial reference (Serial2)
    /// @param rxPin   ESP RX pin (radar TX → ESP)
    /// @param txPin   ESP TX pin (ESP → radar RX)
    /// NOTE: LD2450 has no OT2 pin — signature kept 3-arg unlike LD2410S.
    bool begin(HardwareSerial &serial, uint8_t rxPin = 16, uint8_t txPin = 15);

    /// Call frequently from loop() — reads UART bytes and parses frames.
    void poll();

    /// Get latest reading: PRESENT or ABSENT (nearest in-range target).
    RadarReading read();

    /// Configure parameters.
    void configure(const Ld2450Config &cfg) { _cfg = cfg; }

    bool isReady() const { return _ready; }
    bool hasData() const { return _dataReceived; }
    uint32_t frameCount() const { return _frameCount; }
    int uartAvailable() { return _serial ? _serial->available() : -1; }

    // Per-target access (last parsed frame) — for future multi-target logic.
    const Ld2450Target &target(uint8_t i) const { return _targets[i < 3 ? i : 0]; }
    uint8_t targetCount() const { return _targetCount; }     // # of valid targets
    uint16_t nearestDistCm() const { return _nearestCm; }    // raw nearest, 0=none
    uint16_t smoothDistance() const { return _smoothDist; }  // smoothed nearest

private:
    HardwareSerial *_serial = nullptr;
    bool _ready = false;
    bool _dataReceived = false;
    uint8_t _rxPin = 0;
    uint8_t _txPin = 0;

    // Frame parser: AA FF 03 00 [24B body] 55 CC  (30 bytes fixed)
    static constexpr uint8_t FRAME_LEN = 30;
    static constexpr uint8_t HEADER[4] = {0xAA, 0xFF, 0x03, 0x00};
    static constexpr uint8_t TAIL[2]   = {0x55, 0xCC};
    uint8_t _frameBuf[FRAME_LEN];
    uint8_t _framePos = 0;
    uint8_t _headMatch = 0;  // how many header bytes matched so far

    // Latest parsed targets
    Ld2450Target _targets[3];
    uint8_t      _targetCount = 0;
    uint16_t     _nearestCm = 0;

    // Presence hold (linger) — last time a target was seen in range
    uint32_t _lastInRangeMs = 0;
    bool     _everInRange = false;

    // Diagnostics
    uint32_t _frameCount = 0;
    uint32_t _lastDiagMs = 0;

    // Distance smoothing (nearest target)
    static constexpr uint8_t MAX_SMOOTH = 8;
    uint16_t _distBuf[MAX_SMOOTH] = {};
    uint8_t  _distBufIdx = 0;
    uint8_t  _distBufCount = 0;
    uint16_t _smoothDist = 0;

    Ld2450Config _cfg;

    void feedByte(uint8_t b);
    void parseFrame();
    void updateSmooth(uint16_t distCm);
    static int16_t decodeSigned(uint16_t raw);  // sign-magnitude → int16
};
