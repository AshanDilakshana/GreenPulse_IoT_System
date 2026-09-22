/**
 * GreenPulse IoT System - ESP32 Firmware (Modular OOP Architecture)
 * Designed for Arduino IDE & PlatformIO
 *
 * Hardware:
 *  - ESP32 Dev Module
 *  - DHT22 (Temperature & Humidity) on GPIO 4
 *  - Capacitive Soil Moisture Sensor on GPIO 34 (Analog ADC1)
 *  - RGB LED (Urgency / Alert Indicator) on GPIO 25 (Red), GPIO 26 (Green),
 * GPIO 27 (Blue)
 *  - Water Pump Relay on GPIO 14
 *  - Smart Lamp Relay on GPIO 12
 *  - 16x4 I2C LCD Display (1604A v1.1 with HW-61 backpack, SDA: GPIO 21, SCL:
 * GPIO 22, Address 0x27 or 0x3F)
 *
 * Cloud:
 *  - AWS IoT Core over mutual TLS (Port 8883)
 */

#include "ActuatorManager.h"
#include "DisplayManager.h"
#include "MqttManager.h"
#include "SensorManager.h"
#include "WifiManager.h"
#include "secrets.h"
#include <Arduino.h>

// Pin Definitions
#define PIN_DHT 4
#define PIN_SOIL 34
#define PIN_CO2 32
#define PIN_LED_RED 13
#define PIN_LED_GREEN 12
#define PIN_LED_BLUE 14
#define PIN_PUMP 25 // Moved from 14 to avoid conflict with Blue
#define PIN_LAMP 26 // Moved from 12 to avoid conflict with Green
#define PIN_PIR 27
#define PIN_LDR 33
#define PIN_BUZZER 15
#define PIN_BUTTON 5
#define LCD_I2C_ADDR 0x27 // Typically 0x27 (PCF8574T) or 0x3F (PCF8574AT)

// Sensor Sampling Interval (milliseconds)
#define SENSOR_INTERVAL_MS 2000 // Every 2 seconds

// Instantiate Component Managers (OOP Objects)
WifiManager wifi(WIFI_SSID, WIFI_PASSWORD);
SensorManager sensors(PIN_DHT, PIN_SOIL, PIN_CO2, PIN_PIR, PIN_LDR, DHT22,
                      SENSOR_INTERVAL_MS);
DisplayManager display(LCD_I2C_ADDR, 16, 4); // 1604A (16 columns x 4 rows)
ActuatorManager rgbLed(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE, PIN_PUMP,
                       PIN_LAMP, PIN_BUZZER);
MqttManager mqtt(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID, ROOT_CA, CERTIFICATE,
                 PRIVATE_KEY);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("   🌱 GreenPulse IoT System Starting    ");
  Serial.println("========================================");

  // 1. Initialize Display & Actuators
  display.begin();
  display.showStatus("GreenPulse IoT", "1604A Display OK", "Initializing...",
                     "");
  rgbLed.begin();

  // Initialize Button (Active LOW: connect button between GPIO 5 and GND)
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // 2. Initialize Sensors
  sensors.begin();

  // 3. Connect to Wi-Fi
  display.showStatus("GreenPulse IoT", "Wi-Fi Connecting", WIFI_SSID,
                     "Please wait...");
  wifi.connect();

  if (wifi.isConnected()) {
    display.showStatus("GreenPulse IoT", "Wi-Fi: Connected", "Connecting AWS..",
                       "");
  } else {
    display.showStatus("GreenPulse IoT", "Wi-Fi: FAILED", "Retrying...", "");
  }

  // 4. Initialize Secure MQTT (AWS IoT Core mTLS)
  mqtt.begin(&rgbLed);

  if (mqtt.isConnected()) {
    display.showStatus("GreenPulse IoT", "Wi-Fi : OK", "AWS   : CONNECTED",
                       "System Ready!");
  }
  delay(1500);
}

void loop() {
  // Keep Wi-Fi and MQTT connections alive
  wifi.maintain();
  mqtt.loop();

  // Handle Actuator non-blocking tasks (like blinking LED)
  rgbLed.loop();

  // --- Edge Computing Auto-Off Check ---
  // Constantly check if pump needs to be turned off based on AI target moisture
  rgbLed.checkAutoOff(sensors.getLastData().soilMoisture);

  // --- PIR Motion Buzzer Logic ---
  // Buzzer plays a pattern ONLY if motion is detected AND AI says the plant needs care (Yellow or Red LED)
  rgbLed.setMotionState(sensors.hasMotion() && rgbLed.needsCare());

  static SensorData lastData; // Cache for immediate UI updates

  // --- Manual Display Toggle Logic ---
  static bool manualMode = false;
  static unsigned long lastButtonPressTime = 0;
  static int displayState = 0; // 0: Sensors1, 1: Sensors2, 2: Network
  static bool lastButtonState = HIGH;

  bool currentButtonState = digitalRead(PIN_BUTTON);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    manualMode = true;
    lastButtonPressTime = millis();
    displayState = (displayState + 1) % 3; // Cycle 0 -> 1 -> 2 -> 0

    // Force immediate display update using cached data (prevents slow DHT read)
    if (displayState == 0)
      display.showSensorsPart1(lastData);
    else if (displayState == 1)
      display.showSensorsPart2(lastData);
    else
      display.showNetworkStatus(wifi.isConnected(), mqtt.isConnected());

    delay(50); // Simple debounce
  }
  lastButtonState = currentButtonState;

  // Timeout logic: Revert to Auto Mode after 30 seconds of inactivity
  if (manualMode && (millis() - lastButtonPressTime > 30000)) {
    manualMode = false;
    Serial.println("[Display] Reverting to Auto Mode");
  }

  // Read sensors at defined interval
  if (sensors.isReady()) {
    lastData = sensors.read(); // Update the cache

    // If in Auto Mode, cycle state every 4 seconds.
    // If in Manual Mode, just refresh the current screen without changing
    // displayState.
    if (!manualMode) {
      static int autoCounter = 0;
      if (autoCounter < 2)
        displayState = 0;
      else if (autoCounter < 4)
        displayState = 1;
      else
        displayState = 2;

      autoCounter++;
      if (autoCounter >= 6)
        autoCounter = 0;
    }

    // Refresh LCD with the latest sensor data
    if (displayState == 0)
      display.showSensorsPart1(lastData);
    else if (displayState == 1)
      display.showSensorsPart2(lastData);
    else
      display.showNetworkStatus(wifi.isConnected(), mqtt.isConnected());

    // 2. Publish sensor readings to AWS IoT Core
    if (lastData.isValid) {
      mqtt.publishSensors(lastData);
    }
  }
}
