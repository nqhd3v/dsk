#include "wifi_manager.h"
#include <WiFi.h>

void WifiManager::begin(const char *ssid, const char *password) {
    _ssid     = ssid;
    _password = password;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_ssid, _password);
    _lastAttempt = millis();
}

void WifiManager::loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!_wasConnected) {
            _wasConnected   = true;
            _everConnected  = true;
            _connectedAt    = millis();
            _failCount      = 0;
            Serial.printf("[WiFi] Connected to DG-CENTER. IP=%s RSSI=%d\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());
        }
        return;
    }

    // Disconnected
    if (_wasConnected) {
        _wasConnected = false;
        Serial.println("[WiFi] DG-CENTER lost.");
    }

    uint32_t now = millis();
    uint32_t interval = (_failCount < MAX_FAST_RETRIES) ? FAST_RETRY_MS : RETRY_INTERVAL_MS;

    if (now - _lastAttempt >= interval) {
        _lastAttempt = now;
        _failCount++;
        WiFi.disconnect();
        WiFi.begin(_ssid, _password);
    }
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getIP() const {
    return WiFi.localIP().toString();
}

String WifiManager::getMAC() const {
    return WiFi.macAddress();
}

String WifiManager::statusText() const {
    if (WiFi.status() == WL_CONNECTED) {
        return "Online";
    }
    return "";  // empty = don't show anything
}
