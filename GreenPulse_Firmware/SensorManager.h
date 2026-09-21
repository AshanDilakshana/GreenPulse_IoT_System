#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

struct SensorData {
  float temperature;
  float humidity;
  int soilMoisture;
  int co2;
  int light;
  bool isValid;
};

class SensorManager {
private:
  uint8_t _dhtPin;
  uint8_t _soilPin;
  uint8_t _co2Pin;
  DHT _dht;
  unsigned long _lastReadTime;
  unsigned long _readInterval;

public:
  SensorManager(uint8_t dhtPin, uint8_t soilPin, uint8_t co2Pin, uint8_t dhtType = DHT22,
                unsigned long readIntervalMs = 60000)
      : _dhtPin(dhtPin), _soilPin(soilPin), _co2Pin(co2Pin), _dht(dhtPin, dhtType),
        _lastReadTime(0), _readInterval(readIntervalMs) {}

  void begin() {
    _dht.begin();
    pinMode(_soilPin, INPUT);
    pinMode(_co2Pin, INPUT);
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
    if (rawSoil < 100) {
      data.soilMoisture = -1; // -1 indicates disconnected or error
    } else {
      int soilPercent = map(rawSoil, 4095, 1500, 0, 100);
      data.soilMoisture = constrain(soilPercent, 0, 100);
    }

    int rawCo2 = analogRead(_co2Pin);
    if (rawCo2 < 100) {
      data.co2 = -1; // -1 indicates disconnected or error
    } else {
      // Rough map for analog gas sensor (0-4095 to 400-5000 ppm)
      data.co2 = map(rawCo2, 0, 4095, 400, 5000);
    }
    
    // Light level (Set to -1 to indicate no sensor is connected currently)
    data.light = -1; 
    if (isnan(data.temperature) || isnan(data.humidity)) {
      Serial.println(
          "[SensorManager] Warning: Failed to read from DHT sensor!");
      data.isValid = false;
    } else {
      data.isValid = true;
    }

    return data;
  }
};

#endif
