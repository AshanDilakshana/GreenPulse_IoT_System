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
 *  - 16x4 I2C LCD Display (1604A v1.1 with HW-61 backpack, SDA: GPIO 21, SCL: GPIO 22, Address 0x27 or 0x3F)
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
#define PIN_CO2          32
#define PIN_LED_RED      13
#define PIN_LED_GREEN    12
#define PIN_LED_BLUE     14
#define PIN_PUMP         25 // Moved from 14 to avoid conflict with Blue
#define PIN_LAMP         26 // Moved from 12 to avoid conflict with Green
#define LCD_I2C_ADDR     0x27 // Typically 0x27 (PCF8574T) or 0x3F (PCF8574AT)

// Sensor Sampling Interval (milliseconds)
#define SENSOR_INTERVAL_MS 2000 // Every 2 seconds

// Instantiate Component Managers (OOP Objects)
WifiManager     wifi(WIFI_SSID, WIFI_PASSWORD);
SensorManager   sensors(PIN_DHT, PIN_SOIL, PIN_CO2, DHT22, SENSOR_INTERVAL_MS);
DisplayManager  display(LCD_I2C_ADDR, 16, 4); // 1604A (16 columns x 4 rows)
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
    display.showStatus("GreenPulse IoT", "1604A Display OK", "Initializing...", "");
    rgbLed.begin();

    // 2. Initialize Sensors
    sensors.begin();

    // 3. Connect to Wi-Fi
    display.showStatus("GreenPulse IoT", "Wi-Fi Connecting", WIFI_SSID, "Please wait...");
    wifi.connect();

    if (wifi.isConnected()) {
        display.showStatus("GreenPulse IoT", "Wi-Fi: Connected", "Connecting AWS..", "");
    } else {
        display.showStatus("GreenPulse IoT", "Wi-Fi: FAILED", "Retrying...", "");
    }

    // 4. Initialize Secure MQTT (AWS IoT Core mTLS)
    mqtt.begin(&rgbLed);

    if (mqtt.isConnected()) {
        display.showStatus("GreenPulse IoT", "Wi-Fi : OK", "AWS   : CONNECTED", "System Ready!");
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

        // 1. Alternate LCD Display every 4 seconds (3 screens total)
        // Since isReady() triggers every 2 seconds, we show each screen for 2 cycles (4 seconds)
        static int displayCounter = 0;
        if (displayCounter < 2) {
            display.showSensorsPart1(data);
        } else if (displayCounter < 4) {
            display.showSensorsPart2(data);
        } else {
            display.showNetworkStatus(wifi.isConnected(), mqtt.isConnected());
        }
        
        displayCounter++;
        if (displayCounter >= 6) {
            displayCounter = 0;
        }

        // 2. Publish sensor readings to AWS IoT Core
        if (data.isValid) {
            mqtt.publishSensors(data);
        }
    }
}
