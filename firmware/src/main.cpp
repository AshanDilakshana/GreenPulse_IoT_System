#include "secrets.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Pin Definitions
#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN 34
#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27

// LCD configuration: I2C address 0x27, 16 columns, 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

unsigned long lastPublishTime = 0;
const long publishInterval = 60000; // Publish every 60 seconds

void setupWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connecting.");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi!");
  lcd.clear();
  lcd.print("WiFi Connected!");
}

void setLEDColor(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("JSON Parse failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Expecting a payload like: {"color": "#FF0000", "quote": "...", "alert":
  // "..."} We only care about the color for the RGB LED on the ESP32
  const char *colorHex = doc["color"];
  if (colorHex != nullptr) {
    // Basic hex parsing (e.g. "#FF0000")
    String colorStr = String(colorHex);
    if (colorStr.startsWith("#") && colorStr.length() == 7) {
      long number = strtol(&colorStr[1], NULL, 16);
      int r = number >> 16;
      int g = number >> 8 & 0xFF;
      int b = number & 0xFF;
      setLEDColor(r, g, b);
    }
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    lcd.setCursor(0, 1);
    lcd.print("MQTT: Wait...   ");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("Connected!");
      lcd.setCursor(0, 1);
      lcd.print("MQTT: Connected ");
      mqttClient.subscribe("greenpulse/alerts");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setLEDColor(0, 0, 0); // Off initially

  lcd.init();
  lcd.backlight();
  dht.begin();

  setupWiFi();

  // Configure Secure Client
  secureClient.setCACert(ROOT_CA);
  secureClient.setCertificate(CERTIFICATE);
  secureClient.setPrivateKey(PRIVATE_KEY);

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    lastPublishTime = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);

    // Map soil moisture reading (e.g. 0-4095 for ESP32) to percentage (0-100)
    int soilMoisturePercent = map(soilMoistureRaw, 4095, 0, 0, 100);
    soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Display real-time data on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(t, 1);
    lcd.print("C H:");
    lcd.print(h, 1);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("Soil:");
    lcd.print(soilMoisturePercent);
    lcd.print("% MQ:OK");

    // Create JSON Payload
    StaticJsonDocument<200> doc;
    doc["temperature"] = t;
    doc["humidity"] = h;
    doc["soilMoisture"] = soilMoisturePercent;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    Serial.print("Publishing message: ");
    Serial.println(jsonBuffer);
    mqttClient.publish("greenpulse/sensors", jsonBuffer);
  }
}
