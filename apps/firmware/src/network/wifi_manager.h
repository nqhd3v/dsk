// dsk-guard — WiFi STA manager
// Connects to AP from secrets.h, auto-reconnects in loop().

#pragma once

#include <Arduino.h>

class WifiManager {
public:
    /// Start WiFi STA. Non-blocking — kicks off connection, returns immediately.
    void begin(const char *ssid, const char *password);

    /// Call in loop(). Handles reconnect if dropped. Non-blocking.
    void loop();

    bool isConnected() const;
    bool wasEverConnected() const { return _everConnected; }
    String getIP() const;
    String getMAC() const;

    /// Human-readable status for UI footer: "WiFi OK 192.168.4.2" / "WiFi..." / "No DG-CENTER"
    String statusText() const;

private:
    const char *_ssid     = nullptr;
    const char *_password = nullptr;
    bool  _wasConnected   = false;
    bool  _everConnected  = false;
    uint32_t _connectedAt = 0;
    uint32_t _lastAttempt = 0;
    uint8_t  _failCount   = 0;

    static constexpr uint32_t RETRY_INTERVAL_MS = 10000;  // retry every 10s
    static constexpr uint8_t  MAX_FAST_RETRIES  = 3;      // first 3 retries faster
    static constexpr uint32_t FAST_RETRY_MS     = 3000;
};
