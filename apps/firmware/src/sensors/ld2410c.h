// dsk-guard — LD2410C mmWave radar presence sensor driver
//
// *** NOT IN USE — hardware wiring issue (zero UART data from module). ***
// *** Kept for future use when LD2410C is re-wired or replaced.        ***
// *** Active driver: ld2410s.h (LD2410S, 3.3V, 115200 baud).          ***
//
// UART2 @ 256000 baud, ESP TX=GPIO 15, RX=GPIO 16
// 24 GHz FMCW, ~60° cone, ~6 m range
// Detects: moving target, stationary target, distance, energy levels
// Library: ncmreynolds/ld2410

#pragma once

#include "radar_types.h"
#include <ld2410.h>

class Ld2410cSensor {
public:
    /// Initialize radar on UART2.
    /// @param serial  HardwareSerial reference (Serial2)
    /// @param rxPin   ESP RX pin (radar TX → ESP)
    /// @param txPin   ESP TX pin (ESP → radar RX)
    bool begin(HardwareSerial &serial, uint8_t rxPin = 16, uint8_t txPin = 15);

    /// Call frequently from loop() — feeds serial data to parser.
    /// Non-blocking.
    void poll();

    /// Get latest parsed reading.
    RadarReading read();

    bool isReady() const { return _ready; }

    /// Return bytes available on UART (debug: is data flowing?)
    int uartAvailable() { return _serial ? _serial->available() : -1; }

    /// Has any data been received from radar since begin()?
    bool hasData() const { return _dataReceived; }

    /// Number of successfully parsed frames
    uint32_t frameCount() const { return _frameCount; }

    // Raw library accessors for debugging (before our swap correction)
    uint16_t rawMovingDist()       { return _radar.movingTargetDistance(); }
    uint16_t rawStationaryDist()   { return _radar.stationaryTargetDistance(); }
    uint8_t  rawMovingEnergy()     { return _radar.movingTargetEnergy(); }
    uint8_t  rawStationaryEnergy() { return _radar.stationaryTargetEnergy(); }
    bool     rawPresence()         { return _radar.presenceDetected(); }

private:
    ld2410 _radar;
    HardwareSerial *_serial = nullptr;
    bool _ready = false;
    bool _dataReceived = false;
    uint32_t _frameCount = 0;
    uint32_t _lastDiagMs = 0;
    uint8_t _rxPin = 0;
    uint8_t _txPin = 0;
};
