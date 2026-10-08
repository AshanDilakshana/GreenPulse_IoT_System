#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "SensorManager.h"
#include <time.h>

class HistoryManager {
private:
    const char* _filename = "/history.jsonl";

    String getISOTime() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return ""; // No time synced yet
        }
        char timeStringBuff[50];
        strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
        return String(timeStringBuff);
    }

public:
    HistoryManager() {}

    void begin() {
        if (!LittleFS.begin(true)) {
            Serial.println("[History] LittleFS Mount Failed");
            return;
        }
        Serial.println("[History] LittleFS Mounted successfully.");
    }

    void saveOfflineData(const SensorData& data) {
        String timestamp = getISOTime();
        if (timestamp == "") {
            Serial.println("[History] Time not set, cannot save historical data.");
            return; // Wait until NTP syncs before storing history
        }

        File file = LittleFS.open(_filename, FILE_APPEND);
        if (!file) {
            Serial.println("[History] Failed to open file for appending");
            return;
        }

        StaticJsonDocument<256> doc;
        doc["timestamp"] = timestamp;
        doc["temperature"] = round(data.temperature * 10.0) / 10.0;
        doc["humidity"] = round(data.humidity * 10.0) / 10.0;
        doc["soilMoisture"] = data.soilMoisture;
        doc["co2"] = data.co2;
        doc["light"] = data.light;

        String line;
        serializeJson(doc, line);
        
        file.println(line);
        file.close();
        
        Serial.println("[History] Offline data saved to LittleFS.");
    }

    bool hasData() {
        if (!LittleFS.exists(_filename)) return false;
        File file = LittleFS.open(_filename, FILE_READ);
        bool hasContent = file.size() > 0;
        file.close();
        return hasContent;
    }

    String getAndClearHistoryAsJsonArray() {
        if (!hasData()) return "[]";

        File file = LittleFS.open(_filename, FILE_READ);
        if (!file) return "[]";

        String jsonArray = "[";
        bool first = true;
        int recordCount = 0;
        
        while (file.available() && recordCount < 30) { // Limit to 30 records (~3KB) to prevent MQTT buffer overflow
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                if (!first) jsonArray += ",";
                jsonArray += line;
                first = false;
                recordCount++;
            }
        }
        jsonArray += "]";
        file.close();

        // Clear the file after reading
        LittleFS.remove(_filename);
        
        return jsonArray;
    }
};

#endif
