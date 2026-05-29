#include "mqtt_manager.h"

// Static instance for PubSubClient callback routing
MqttManager *MqttManager::_instance = nullptr;

void MqttManager::config(const char *host, uint16_t port,
                          const char *user, const char *password,
                          const char *nodeId) {
    _host     = host;
    _port     = port;
    _user     = user;
    _password = password;
    _nodeId   = nodeId;
}

bool MqttManager::begin() {
    _instance = this;

    _mqtt.setClient(_wifiClient);
    _mqtt.setServer(_host, _port);
    _mqtt.setBufferSize(BUFFER_SIZE);
    _mqtt.setCallback(staticCallback);
    _mqtt.setKeepAlive(30);

    Serial.printf("[MQTT] Broker %s:%u user=%s node=%s\n", _host, _port, _user, _nodeId);
    return reconnect();
}

void MqttManager::loop() {
    if (_mqtt.connected()) {
        _mqtt.loop();
        if (!_wasConnected) {
            _wasConnected = true;
            Serial.println("[MQTT] Reconnected.");
            subscribeTopics();
        }
        return;
    }

    // Disconnected
    if (_wasConnected) {
        _wasConnected = false;
        Serial.println("[MQTT] Connection lost.");
    }

    uint32_t now = millis();
    if (now - _lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
        _lastReconnectAttempt = now;
        reconnect();
    }
}

bool MqttManager::reconnect() {
    // Client ID: "dg-<nodeId>"
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "dg-%s", _nodeId);

    Serial.printf("[MQTT] Connecting as \"%s\"...\n", clientId);

    if (_mqtt.connect(clientId, _user, _password)) {
        _wasConnected = true;
        Serial.println("[MQTT] Connected!");
        subscribeTopics();
        return true;
    }

    Serial.printf("[MQTT] Failed, rc=%d\n", _mqtt.state());
    return false;
}

void MqttManager::subscribeTopics() {
    // Subscribe to cmd topics: dg/<node>/cmd/#
    char topic[64];
    snprintf(topic, sizeof(topic), "dg/%s/cmd/#", _nodeId);
    _mqtt.subscribe(topic);
    Serial.printf("[MQTT] Subscribed: %s\n", topic);
}

bool MqttManager::publish(const char *topic, const char *json, bool retained) {
    if (!_mqtt.connected()) return false;

    bool ok = _mqtt.publish(topic, json, retained);
    if (!ok) {
        Serial.printf("[MQTT] Publish FAILED topic=%s len=%u\n", topic, strlen(json));
    }
    return ok;
}

bool MqttManager::isConnected() {
    return _mqtt.connected();
}

void MqttManager::staticCallback(char *topic, uint8_t *payload, unsigned int len) {
    Serial.printf("[MQTT] Received: %s (%u bytes)\n", topic, len);
    if (_instance && _instance->_callback) {
        _instance->_callback(topic, payload, len);
    }
}
