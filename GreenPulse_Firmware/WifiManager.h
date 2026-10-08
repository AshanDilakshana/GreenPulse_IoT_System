#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WifiManager {
private:
    const char* _ssid;
    const char* _password;

public:
    WifiManager(const char* ssid, const char* password)
        : _ssid(ssid), _password(password) {}

    void connect() {
        Serial.printf("[WiFi] Connecting to %s", _ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[WiFi] Connected successfully!");
            Serial.print("[WiFi] IP Address: ");
            Serial.println(WiFi.localIP());

            // Initialize NTP Time
            Serial.println("[WiFi] Syncing time via NTP...");
            configTime(5 * 3600 + 1800, 0, "pool.ntp.org", "time.nist.gov"); // IST (UTC+5:30)
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 10000)) {
                Serial.println("[WiFi] Time synced!");
            } else {
                Serial.println("[WiFi] Time sync failed");
            }
        } else {
            Serial.println("\n[WiFi] Connection timeout. Will retry in loop.");
        }
    }

    bool isConnected() {
        return (WiFi.status() == WL_CONNECTED);
    }

    void maintain() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Lost connection. Reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
};

#endif
