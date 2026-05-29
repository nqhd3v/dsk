// dsk-guard — MQTT manager
// PubSubClient wrapper: connect, auto-reconnect, publish telemetry + hello,
// subscribe to cmd topics.

#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// Forward-declare callback type for incoming commands
using MqttCmdCallback = std::function<void(const char *topic, const uint8_t *payload, unsigned int len)>;

class MqttManager {
public:
    /// Configure broker. Call before begin().
    void config(const char *host, uint16_t port,
                const char *user, const char *password,
                const char *nodeId);

    /// Connect to broker. Returns true if connected.
    bool begin();

    /// Call in loop(). Handles reconnect + PubSubClient.loop().
    void loop();

    /// Publish JSON string to topic. Returns true if sent.
    bool publish(const char *topic, const char *json, bool retained = false);

    /// Register callback for incoming cmd messages.
    void onMessage(MqttCmdCallback cb) { _callback = cb; }

    bool isConnected();
    const char *nodeId() const { return _nodeId; }

private:
    WiFiClient _wifiClient;
    PubSubClient _mqtt;

    const char *_host     = nullptr;
    uint16_t    _port     = 1883;
    const char *_user     = nullptr;
    const char *_password = nullptr;
    const char *_nodeId   = nullptr;

    MqttCmdCallback _callback = nullptr;

    uint32_t _lastReconnectAttempt = 0;
    bool     _wasConnected = false;

    static constexpr uint32_t RECONNECT_INTERVAL_MS = 5000;
    static constexpr uint16_t BUFFER_SIZE = 512;

    bool reconnect();
    void subscribeTopics();

    static void staticCallback(char *topic, uint8_t *payload, unsigned int len);
    static MqttManager *_instance;  // for static callback routing
};
