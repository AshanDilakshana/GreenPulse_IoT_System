#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "SensorManager.h"
#include "ActuatorManager.h"

class MqttManager {
private:
    const char* _broker;
    int _port;
    const char* _clientId;
    const char* _pubTopic;
    const char* _subTopic;

    WiFiClientSecure _secureClient;
    PubSubClient _mqttClient;
    ActuatorManager* _actuator;
    unsigned long _lastReconnectAttempt;

    void reconnect();

public:
    void onMessage(char* topic, byte* payload, unsigned int length);

    MqttManager(const char* broker, int port, const char* clientId,
                const char* rootCA, const char* cert, const char* privateKey,
                const char* pubTopic = "greenpulse/sensors",
                const char* subTopic = "greenpulse/alerts");

    void begin(ActuatorManager* actuator);
    void loop();
    bool isConnected();
    bool publishSensors(const SensorData& data);
};

#endif
