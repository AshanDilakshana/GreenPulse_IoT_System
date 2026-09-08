#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "SensorManager.h"

class DisplayManager {
private:
    LiquidCrystal_I2C _lcd;

public:
    DisplayManager(uint8_t lcdAddr = 0x27, uint8_t cols = 16, uint8_t rows = 2)
        : _lcd(lcdAddr, cols, rows) {}

    void begin() {
        _lcd.init();
        _lcd.backlight();
        _lcd.clear();
    }

    void showStatus(const char* line1, const char* line2 = "") {
        _lcd.clear();
        _lcd.setCursor(0, 0);
        _lcd.print(line1);
        if (line2 != nullptr && strlen(line2) > 0) {
            _lcd.setCursor(0, 1);
            _lcd.print(line2);
        }
    }

    void showSensorData(const SensorData& data, bool mqttConnected) {
        _lcd.clear();
        _lcd.setCursor(0, 0);
        _lcd.print("T:");
        _lcd.print(data.temperature, 1);
        _lcd.print("C H:");
        _lcd.print((int)data.humidity);
        _lcd.print("%");

        _lcd.setCursor(0, 1);
        _lcd.print("Soil:");
        _lcd.print(data.soilMoisture);
        _lcd.print("% ");
        _lcd.print(mqttConnected ? "MQ:OK" : "MQ:ERR");
    }
};

#endif
