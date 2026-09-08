#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>

class ActuatorManager {
private:
    uint8_t _redPin;
    uint8_t _greenPin;
    uint8_t _bluePin;

public:
    ActuatorManager(uint8_t redPin, uint8_t greenPin, uint8_t bluePin)
        : _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin) {}

    void begin() {
        pinMode(_redPin, OUTPUT);
        pinMode(_greenPin, OUTPUT);
        pinMode(_bluePin, OUTPUT);
        turnOff();
    }

    void setColor(uint8_t red, uint8_t green, uint8_t blue) {
        analogWrite(_redPin, red);
        analogWrite(_greenPin, green);
        analogWrite(_bluePin, blue);
    }

    void setColorHex(const char* hexColor) {
        if (hexColor == nullptr) return;

        String colorStr = String(hexColor);
        colorStr.trim();
        if (colorStr.startsWith("#")) {
            colorStr = colorStr.substring(1);
        }

        if (colorStr.length() == 6) {
            long number = strtol(colorStr.c_str(), NULL, 16);
            uint8_t r = (number >> 16) & 0xFF;
            uint8_t g = (number >> 8) & 0xFF;
            uint8_t b = number & 0xFF;
            setColor(r, g, b);
            Serial.printf("[Actuator] RGB updated to: #%s (R:%d, G:%d, B:%d)\n", colorStr.c_str(), r, g, b);
        }
    }

    void turnOff() {
        setColor(0, 0, 0);
    }
};

#endif
