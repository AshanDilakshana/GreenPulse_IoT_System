#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>

class ActuatorManager {
private:
    uint8_t _redPin;
    uint8_t _greenPin;
    uint8_t _bluePin;
    uint8_t _pumpPin;
    uint8_t _lampPin;

    bool _isPumpActive;
    unsigned long _lastBlinkTime;
    unsigned long _pumpStartTime;
    bool _blinkState;
    String _lastColorHex;

public:
    ActuatorManager(uint8_t redPin, uint8_t greenPin, uint8_t bluePin, uint8_t pumpPin, uint8_t lampPin)
        : _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin), _pumpPin(pumpPin), _lampPin(lampPin),
          _isPumpActive(false), _lastBlinkTime(0), _pumpStartTime(0), _blinkState(false), _lastColorHex("#000000") {}

    void begin() {
        pinMode(_redPin, OUTPUT);
        pinMode(_greenPin, OUTPUT);
        pinMode(_bluePin, OUTPUT);
        pinMode(_pumpPin, OUTPUT);
        pinMode(_lampPin, OUTPUT);
        turnOff();
    }

    void setColor(uint8_t red, uint8_t green, uint8_t blue) {
        analogWrite(_redPin, red);
        analogWrite(_greenPin, green);
        analogWrite(_bluePin, blue);
    }

    void setColorHex(const char* hexColor, bool saveState = true) {
        if (hexColor == nullptr) return;

        String colorStr = String(hexColor);
        colorStr.trim();
        if (colorStr.startsWith("#")) {
            colorStr = colorStr.substring(1);
        }

        if (saveState) {
            _lastColorHex = "#" + colorStr;
        }

        if (colorStr.length() == 6) {
            long number = strtol(colorStr.c_str(), NULL, 16);
            uint8_t r = (number >> 16) & 0xFF;
            uint8_t g = (number >> 8) & 0xFF;
            uint8_t b = number & 0xFF;
            setColor(r, g, b);
        }
    }

    void setIndicatorColor(const char* colorName) {
        if (colorName == nullptr) return;
        String colorStr = String(colorName);
        colorStr.toUpperCase();
        
        String targetHex = "#0000FF"; // Default blue
        if (colorStr == "RED") {
            targetHex = "#FF0000";
        } else if (colorStr == "YELLOW") {
            targetHex = "#FFFF00";
        } else if (colorStr == "GREEN") {
            targetHex = "#00FF00";
        }
        
        // Save the intended state
        _lastColorHex = targetHex;

        // Apply immediately only if pump is not overriding it with blinking
        if (!_isPumpActive) {
            setColorHex(targetHex.c_str(), false);
            Serial.printf("[Actuator] Indicator set to %s (%s)\n", colorStr.c_str(), targetHex.c_str());
        }
    }

    void setPump(bool state) {
        if (_isPumpActive != state) {
            _isPumpActive = state;
            digitalWrite(_pumpPin, state ? HIGH : LOW);
            Serial.printf("[Actuator] Pump is now %s\n", state ? "ON" : "OFF");
            
            if (state) {
                // Start blinking red immediately and record start time
                _blinkState = true;
                _lastBlinkTime = millis();
                _pumpStartTime = millis();
                setColorHex("#FF0000", false);
            } else {
                // Pump turned off, restore the last known system color
                setColorHex(_lastColorHex.c_str(), false);
            }
        }
    }

    void setLamp(bool state) {
        digitalWrite(_lampPin, state ? HIGH : LOW);
        Serial.printf("[Actuator] Smart Lamp is now %s\n", state ? "ON" : "OFF");
    }

    void loop() {
        if (_isPumpActive) {
            unsigned long now = millis();
            
            // Pulse Watering Safety Timeout (Max 15 seconds)
            if (now - _pumpStartTime >= 15000) {
                Serial.println("[Actuator] 15s hardware timeout reached. Auto-stopping pump (Pulse watering).");
                setPump(false);
                return; // Exit loop after turning off
            }

            // Blinking logic
            if (now - _lastBlinkTime >= 500) { // Toggle every 500ms
                _lastBlinkTime = now;
                _blinkState = !_blinkState;
                if (_blinkState) {
                    setColorHex("#FF0000", false); // Red ON
                } else {
                    setColorHex("#000000", false); // OFF
                }
            }
        }
    }

    void turnOff() {
        setColor(0, 0, 0);
        digitalWrite(_pumpPin, LOW);
        digitalWrite(_lampPin, LOW);
        _isPumpActive = false;
        _lastColorHex = "#000000";
    }
};

#endif
