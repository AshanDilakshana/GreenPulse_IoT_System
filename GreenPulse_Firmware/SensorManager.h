#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

struct SensorData {
    float temperature;
    float humidity;
    int soilMoisture;
    bool isValid;
};

class SensorManager {
private:
    uint8_t _dhtPin;
    uint8_t _soilPin;
    DHT _dht;
    unsigned long _lastReadTime;
    unsigned long _readInterval;

public:
    SensorManager(uint8_t dhtPin, uint8_t soilPin, uint8_t dhtType = DHT22, unsigned long readIntervalMs = 60000)
        : _dhtPin(dhtPin), _soilPin(soilPin), _dht(dhtPin, dhtType), _lastReadTime(0), _readInterval(readIntervalMs) {}

    void begin() {
        _dht.begin();
        pinMode(_soilPin, INPUT);
    }

    bool isReady() {
        unsigned long currentMillis = millis();
        if (currentMillis - _lastReadTime >= _readInterval || _lastReadTime == 0) {
            _lastReadTime = currentMillis;
            return true;
        }
        return false;
    }

    SensorData read() {
        SensorData data;
        data.humidity = _dht.readHumidity();
        data.temperature = _dht.readTemperature();

        int rawSoil = analogRead(_soilPin);
        int soilPercent = map(rawSoil, 4095, 1500, 0, 100);
        data.soilMoisture = constrain(soilPercent, 0, 100);

        if (isnan(data.temperature) || isnan(data.humidity)) {
            Serial.println("[SensorManager] Warning: Failed to read from DHT sensor!");
            data.isValid = false;
        } else {
            data.isValid = true;
        }

        return data;
    }
};

#endif
