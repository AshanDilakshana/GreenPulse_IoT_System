/**
 * GreenPulse IoT System - ESP32 Firmware (Modular OOP Architecture)
 * Designed for Arduino IDE & PlatformIO
 * 
 * Hardware:
 *  - ESP32 Dev Module
 *  - DHT22 (Temperature & Humidity) on GPIO 4
 *  - Capacitive Soil Moisture Sensor on GPIO 34 (Analog ADC1)
 *  - RGB LED (Urgency / Alert Indicator) on GPIO 25 (Red), GPIO 26 (Green), GPIO 27 (Blue)
 *  - Water Pump Relay on GPIO 14
 *  - Smart Lamp Relay on GPIO 12
 *  - 16x2 I2C LCD Display (SDA: GPIO 21, SCL: GPIO 22, Address 0x27)
 * 
 * Cloud:
 *  - AWS IoT Core over mutual TLS (Port 8883)
 */

#include <Arduino.h>
#include "secrets.h"
#include "WifiManager.h"
#include "SensorManager.h"
#include "DisplayManager.h"
#include "ActuatorManager.h"
#include "MqttManager.h"

// Pin Definitions
#define PIN_DHT          4
#define PIN_SOIL         34
#define PIN_LED_RED      25
#define PIN_LED_GREEN    26
#define PIN_LED_BLUE     27
#define PIN_PUMP         14
#define PIN_LAMP         12
#define LCD_I2C_ADDR     0x27

// Sensor Sampling Interval (milliseconds)
#define SENSOR_INTERVAL_MS 2000 // Every 2 seconds

// Instantiate Component Managers (OOP Objects)
WifiManager     wifi(WIFI_SSID, WIFI_PASSWORD);
SensorManager   sensors(PIN_DHT, PIN_SOIL, DHT22, SENSOR_INTERVAL_MS);
DisplayManager  display(LCD_I2C_ADDR, 16, 2);
ActuatorManager rgbLed(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE, PIN_PUMP, PIN_LAMP);
MqttManager     mqtt(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID, ROOT_CA, CERTIFICATE, PRIVATE_KEY);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("   🌱 GreenPulse IoT System Starting    ");
    Serial.println("========================================");

    // 1. Initialize Display & Actuators
    display.begin();
    display.showStatus("GreenPulse IoT", "Starting...");
    rgbLed.begin();

    // 2. Initialize Sensors
    sensors.begin();

    // 3. Connect to Wi-Fi
    display.showStatus("Wi-Fi Connecting", WIFI_SSID);
    wifi.connect();

    if (wifi.isConnected()) {
        display.showStatus("Wi-Fi Connected!", "Connecting MQTT");
    } else {
        display.showStatus("Wi-Fi Failed!", "Retrying in loop");
    }

    // 4. Initialize Secure MQTT (AWS IoT Core mTLS)
    mqtt.begin(&rgbLed);

    if (mqtt.isConnected()) {
        display.showStatus("GreenPulse Ready", "AWS IoT: OK");
    }
    delay(1500);
}

void loop() {
    // Keep Wi-Fi and MQTT connections alive
    wifi.maintain();
    mqtt.loop();

    // Handle Actuator non-blocking tasks (like blinking LED)
    rgbLed.loop();

    // Read sensors at defined interval
    if (sensors.isReady()) {
        SensorData data = sensors.read();

        // 1. Update local I2C LCD Display (Sensor data & MQTT status only, NO LLM text)
        display.showSensorData(data, mqtt.isConnected());

        // 2. Publish sensor readings to AWS IoT Core
        if (data.isValid) {
            mqtt.publishSensors(data);
        }
    }
}
