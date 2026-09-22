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
  uint8_t _pirPin;
  uint8_t _ldrPin;
  DHT _dht;
  unsigned long _lastReadTime;
  unsigned long _readInterval;

public:
  SensorManager(uint8_t dhtPin, uint8_t soilPin, uint8_t co2Pin, uint8_t pirPin, uint8_t ldrPin, uint8_t dhtType = DHT22,
                unsigned long readIntervalMs = 60000)
      : _dhtPin(dhtPin), _soilPin(soilPin), _co2Pin(co2Pin), _pirPin(pirPin), _ldrPin(ldrPin), _dht(dhtPin, dhtType),
        _lastReadTime(0), _readInterval(readIntervalMs) {}

  void begin() {
    _dht.begin();
    pinMode(_soilPin, INPUT);
    pinMode(_co2Pin, INPUT);
    pinMode(_pirPin, INPUT);
    pinMode(_ldrPin, INPUT);
  }

  bool isReady() {
    unsigned long currentMillis = millis();
    if (currentMillis - _lastReadTime >= _readInterval || _lastReadTime == 0) {
      _lastReadTime = currentMillis;
      return true;
    }
    return false;
  }

  // Returns true if PIR sensor detects motion
  bool hasMotion() {
    return digitalRead(_pirPin) == HIGH;
  }

  SensorData read() {
    SensorData data;
    data.humidity = _dht.readHumidity();
    data.temperature = _dht.readTemperature();

    int rawSoil = analogRead(_soilPin);
    if (rawSoil < 10) {
      data.soilMoisture = -1; // -1 indicates disconnected or error
    } else {
      // Calibrated values: Dry in air = ~2606, Wet in water = ~1200
      int soilPercent = map(rawSoil, 2606, 1200, 0, 100);
      data.soilMoisture = constrain(soilPercent, 0, 100);
    }

    int rawCo2 = analogRead(_co2Pin);
    if (rawCo2 < 10) {
      data.co2 = -1; // -1 indicates disconnected or error
    } else {
      // Rough map for analog gas sensor (0-4095 to 400-5000 ppm)
      data.co2 = map(rawCo2, 0, 4095, 400, 5000);
    }
    
    // Light level from LDR
    int rawLight = analogRead(_ldrPin);
    if (rawLight < 10) {
      data.light = -1; // -1 indicates disconnected or error
    } else {
      // Map LDR analog value (0-4095) to roughly lux or percentage (0-1000)
      data.light = map(rawLight, 0, 4095, 0, 1000);
    }
    
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
