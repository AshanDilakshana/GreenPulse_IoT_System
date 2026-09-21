#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "SensorManager.h"

/**
 * DisplayManager handles the 1604A (16x4 Character LCD) with HW-61 (PCF8574 I2C Backpack).
 * 
 * Note on 1604A DDRAM Addressing:
 * Standard 16x4 LCDs have row memory offsets at:
 *   Row 0: 0x00, Row 1: 0x40, Row 2: 0x10, Row 3: 0x50
 * (Unlike 20x4 LCDs which use 0x14 and 0x54).
 * We provide a tailored setCursor() so Row 2 and Row 3 are not indented by 4 characters.
 */
class DisplayManager {
private:
    LiquidCrystal_I2C _lcd;
    uint8_t _cols;
    uint8_t _rows;

public:
    DisplayManager(uint8_t lcdAddr = 0x27, uint8_t cols = 16, uint8_t rows = 4)
        : _lcd(lcdAddr, cols, rows), _cols(cols), _rows(rows) {}

    void begin() {
        _lcd.init();
        _lcd.backlight();
        _lcd.clear();
    }

    /**
     * Position cursor specifically mapped for 1604A (16 columns x 4 rows)
     */
    void setCursor(uint8_t col, uint8_t row) {
        if (_rows == 4 && _cols == 16) {
            const uint8_t row_offsets[] = { 0x00, 0x40, 0x10, 0x50 };
            if (row >= 4) row = 3;
            _lcd.command(0x80 | (col + row_offsets[row]));
        } else {
            _lcd.setCursor(col, row);
        }
    }

    /**
     * Prints a fixed 16-character line padded with spaces to avoid clear() screen flicker
     */
    void printLine(uint8_t row, const char* text) {
        setCursor(0, row);
        char buffer[17];
        snprintf(buffer, sizeof(buffer), "%-16.16s", text);
        _lcd.print(buffer);
    }

    /**
     * Display status messages across the 4 rows
     */
    void showStatus(const char* line1, const char* line2 = "", const char* line3 = "", const char* line4 = "") {
        printLine(0, line1);
        printLine(1, line2);
        printLine(2, line3);
        printLine(3, line4);
    }

    /**
     * Screen 1: Sensors Part 1 (Temp, Hum, Soil)
     */
    void showSensorsPart1(const SensorData& data) {
        char lineBuf[17];
        printLine(0, "--- SENSORS ---");

        // Line 1: Temperature
        if (isnan(data.temperature)) {
            snprintf(lineBuf, sizeof(lineBuf), "Temp : NAN C");
        } else {
            snprintf(lineBuf, sizeof(lineBuf), "Temp : %.1f C", data.temperature);
        }
        printLine(1, lineBuf);

        // Line 2: Humidity
        if (isnan(data.humidity)) {
            snprintf(lineBuf, sizeof(lineBuf), "Hum  : NAN %%");
        } else {
            snprintf(lineBuf, sizeof(lineBuf), "Hum  : %.1f %%", data.humidity);
        }
        printLine(2, lineBuf);

        // Line 3: Soil Moisture
        if (data.soilMoisture == -1) {
            snprintf(lineBuf, sizeof(lineBuf), "Soil : NAN %%");
        } else {
            snprintf(lineBuf, sizeof(lineBuf), "Soil : %d %%", data.soilMoisture);
        }
        printLine(3, lineBuf);
    }

    /**
     * Screen 2: Sensors Part 2 (CO2, Light)
     */
    void showSensorsPart2(const SensorData& data) {
        char lineBuf[17];
        printLine(0, "--- SENSORS ---");

        // Line 1: CO2
        if (data.co2 == -1) {
            snprintf(lineBuf, sizeof(lineBuf), "CO2  : NAN ppm");
        } else {
            snprintf(lineBuf, sizeof(lineBuf), "CO2  : %d ppm", data.co2);
        }
        printLine(1, lineBuf);

        // Line 2: Light
        if (data.light == -1) {
            snprintf(lineBuf, sizeof(lineBuf), "Light: NAN lx");
        } else {
            snprintf(lineBuf, sizeof(lineBuf), "Light: %d lx", data.light);
        }
        printLine(2, lineBuf);

        printLine(3, "                "); // Clear line 4
    }

    /**
     * Screen 2: Displays Wi-Fi and AWS IoT connection status
     */
    void showNetworkStatus(bool wifiConnected, bool mqttConnected) {
        char lineBuf[17];

        printLine(0, "--- NETWORK ---");
        
        snprintf(lineBuf, sizeof(lineBuf), "WiFi: %s", wifiConnected ? "CONNECTED" : "FAILED");
        printLine(1, lineBuf);

        snprintf(lineBuf, sizeof(lineBuf), "AWS : %s", mqttConnected ? "CONNECTED" : "FAILED");
        printLine(2, lineBuf);
        
        printLine(3, "                "); // Clear 4th line
    }
};

#endif

