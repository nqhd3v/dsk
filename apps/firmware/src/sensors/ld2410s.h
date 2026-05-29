// dsk-guard — LD2410S mmWave radar presence sensor driver
// UART2 @ 115200 baud, ESP TX=GPIO 15, RX=GPIO 16
// 24 GHz FMCW, ~60° horizontal, ~8 m moving / ~4 m stationary
//
// *** VCC = 3.3V (3.0–3.6V) — DO NOT connect to 5V! ***
//
// Simple 2-state model:
//   PRESENT = sensor says someone + smoothed distance ≤ maxRangeCm
//   ABSENT  = no one OR distance > maxRangeCm
//
// OT2 pin (optional): digital presence (HIGH=someone). Faster than UART state
// which has built-in unmanned delay. Used as presence ground truth when wired.
//
// Protocol: HLK-LD2410S Serial Communication Protocol V1.00, §2.1
//   Minimal frame (default): 6E | state(1) | dist(2, LE) | 62   (5 bytes)
//   Standard frame (cmd 0x007A): F4 F3 F2 F1 | ... | F8 F7 F6 F5
//   state: 0/1 = no one, 2/3 = someone

#pragma once

#include "radar_types.h"

struct Ld2410sConfig {
    uint16_t maxRangeCm   = 70;   // beyond this = ABSENT (desk range)
    uint8_t  smoothWindow = 5;     // moving average window for distance
};

class Ld2410sSensor {
public:
    /// Initialize radar on UART2.
    /// @param serial  HardwareSerial reference (Serial2)
    /// @param rxPin   ESP RX pin (radar TX → ESP)
    /// @param txPin   ESP TX pin (ESP → radar RX)
    /// @param ot2Pin  OT2 digital presence pin (HIGH=someone, LOW=no one), 0xFF=unused
    bool begin(HardwareSerial &serial, uint8_t rxPin = 16, uint8_t txPin = 15,
               uint8_t ot2Pin = 0xFF);

    /// Call frequently from loop() — reads UART bytes and parses frames.
    void poll();

    /// Get latest reading: PRESENT or ABSENT.
    RadarReading read();

    /// Configure parameters.
    void configure(const Ld2410sConfig &cfg) { _cfg = cfg; }

    bool isReady() const { return _ready; }
    bool hasData() const { return _dataReceived; }
    uint32_t frameCount() const { return _frameCount; }
    int uartAvailable() { return _serial ? _serial->available() : -1; }

    /// Raw values from last parsed frame (for debug)
    uint8_t  rawTargetState() const { return _targetState; }
    uint16_t rawDistance()     const { return _distance; }
    uint16_t smoothDistance()  const { return _smoothDist; }
    bool     ot2Present()     const { return _ot2Present; }
    bool     hasOt2()         const { return _ot2Pin != 0xFF; }

private:
    HardwareSerial *_serial = nullptr;
    bool _ready = false;
    bool _dataReceived = false;
    uint8_t _rxPin = 0;
    uint8_t _txPin = 0;
    uint8_t _ot2Pin = 0xFF;
    bool    _ot2Present = false;

    // Minimal frame parser: 6E [state] [dist_lo] [dist_hi] 62
    enum ParseState : uint8_t { WAIT_HEAD, IN_BODY };
    ParseState _parseState = WAIT_HEAD;
    uint8_t _frameBuf[5];
    uint8_t _framePos = 0;

    // Latest parsed values
    uint8_t  _targetState = 0;
    uint16_t _distance = 0;

    // Diagnostics
    uint32_t _frameCount = 0;
    uint32_t _lastDiagMs = 0;

    // Distance smoothing
    static constexpr uint8_t MAX_SMOOTH = 8;
    uint16_t _distBuf[MAX_SMOOTH] = {};
    uint8_t  _distBufIdx = 0;
    uint8_t  _distBufCount = 0;
    uint16_t _smoothDist = 0;

    Ld2410sConfig _cfg;

    void updateSmooth(uint16_t dist);
    bool parseFrame();
    void feedByte(uint8_t b);
};
