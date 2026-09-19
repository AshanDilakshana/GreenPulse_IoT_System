#include "MqttManager.h"
#include <ArduinoJson.h>

// Static reference needed for PubSubClient C-style callback
static MqttManager* instance = nullptr;

static void globalMqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance != nullptr) {
        // Forward to member method
        // Using static reference
        String msg = "";
        for (unsigned int i = 0; i < length; i++) {
            msg += (char)payload[i];
        }

        Serial.printf("[MQTT] Message on [%s]: %s\n", topic, msg.c_str());

        StaticJsonDocument<1024> doc; // Increased size to handle larger response
        DeserializationError err = deserializeJson(doc, msg);
        if (err) {
            Serial.printf("[MQTT] JSON parse error: %s\n", err.c_str());
            return;
        }

        // Expected backend payload: {"pump_status": "ON", "smart_lamp_status": "ON", "indicator_color": "RED", ...}
        if (instance->_actuator != nullptr) {
            if (doc.containsKey("indicator_color")) {
                const char* colorStr = doc["indicator_color"];
                instance->_actuator->setIndicatorColor(colorStr);
            }
            if (doc.containsKey("pump_status")) {
                const char* pumpStatus = doc["pump_status"];
                instance->_actuator->setPump(String(pumpStatus) == "ON");
            }
            if (doc.containsKey("smart_lamp_status")) {
                const char* lampStatus = doc["smart_lamp_status"];
                instance->_actuator->setLamp(String(lampStatus) == "ON");
            }
            if (doc.containsKey("dashboard_care_quote")) {
                const char* quote = doc["dashboard_care_quote"];
                Serial.printf("[MQTT] Care Quote: %s\n", quote);
            }
        }
    }
}

MqttManager::MqttManager(const char* broker, int port, const char* clientId,
                         const char* rootCA, const char* cert, const char* privateKey,
                         const char* pubTopic, const char* subTopic)
    : _broker(broker), _port(port), _clientId(clientId),
      _pubTopic(pubTopic), _subTopic(subTopic),
      _mqttClient(_secureClient), _actuator(nullptr), _lastReconnectAttempt(0) {

    // Configure mutual TLS for AWS IoT Core
    _secureClient.setCACert(rootCA);
    _secureClient.setCertificate(cert);
    _secureClient.setPrivateKey(privateKey);
}

void MqttManager::begin(ActuatorManager* actuator) {
    _actuator = actuator;
    instance = this;

    _mqttClient.setServer(_broker, _port);
    _mqttClient.setCallback(globalMqttCallback);
    // Increase buffer size to handle JSON payloads comfortably
    _mqttClient.setBufferSize(1024);

    reconnect();
}

void MqttManager::reconnect() {
    Serial.printf("[MQTT] Connecting to AWS IoT Core (%s)...\n", _broker);
    
    // AWS IoT Core uses mTLS certificates - no username/password needed
    if (_mqttClient.connect(_clientId)) {
        Serial.println("[MQTT] Connected to AWS IoT Core!");
        _mqttClient.subscribe(_subTopic);
        Serial.printf("[MQTT] Subscribed to topic: %s\n", _subTopic);
    } else {
        Serial.printf("[MQTT] Connection failed, rc=%d. Will retry...\n", _mqttClient.state());
    }
}

void MqttManager::loop() {
    if (!_mqttClient.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 5000) {
            _lastReconnectAttempt = now;
            reconnect();
        }
    } else {
        _mqttClient.loop();
    }
}

bool MqttManager::isConnected() {
    return _mqttClient.connected();
}

bool MqttManager::publishSensors(const SensorData& data) {
    if (!_mqttClient.connected()) {
        return false;
    }

    StaticJsonDocument<256> doc;
    doc["temperature"] = round(data.temperature * 10.0) / 10.0;
    doc["humidity"] = round(data.humidity * 10.0) / 10.0;
    doc["soilMoisture"] = data.soilMoisture;
    doc["co2"] = data.co2;
    doc["light"] = data.light;

    char buffer[256];
    size_t len = serializeJson(doc, buffer);

    bool ok = _mqttClient.publish(_pubTopic, buffer);
    if (ok) {
        Serial.printf("[MQTT] Published to %s: %s\n", _pubTopic, buffer);
    } else {
        Serial.println("[MQTT] Failed to publish sensor data!");
    }
    return ok;
}
