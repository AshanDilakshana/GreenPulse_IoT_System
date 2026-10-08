# GreenPulse IoT System 🌱

GreenPulse is an advanced, Agentic AI-powered Smart Indoor Plant Care system. It autonomously monitors plant health, controls physical actuators (water pump, smart lamp), and generates human-like insights using real-time sensor data, weather forecasts, and historical trends.

## 🌟 Key Features

*   **Agentic AI Brain:** Uses Google's Gemini AI (via LangChain) to make intelligent, context-aware decisions rather than relying on simple `if/else` rules.
*   **Weather-Aware Smart Watering & Temp Shock:** Checks the OpenWeather API before watering. If rain is predicted, it delays watering. If the indoor temperature severely mismatches the outdoor temperature (e.g. AC is on), it warns about "Temperature Shock".
*   **Time-Based Smart Lighting:** Automatically controls a smart UV lamp based on ambient light levels and the current time of day.
*   **Robust Offline Failsafe (Edge Computing):** Even if the WiFi drops, the ESP32 acts autonomously using its own internal clock (NTP synced) and LittleFS storage:
    *   **Offline Data Logging:** Saves sensor data locally every 10 minutes and syncs it back to the AWS Cloud (`greenpulse/history`) automatically when reconnected.
    *   **Failsafe Auto-Watering:** If soil moisture drops below a critical 15% while offline, the hardware auto-waters the plant up to a safe 40% target.
    *   **Failsafe Auto-Lighting:** Operates the UV lamp based on the internal RTC clock between 6 AM and 6 PM if light is too low.
*   **WhatsApp Bot Integration:** 2-Way conversational WhatsApp integration via `whatsapp-web.js`. Users get critical alerts on WhatsApp and can control the system directly by sending conversational messages like "Turn on the pump now".
*   **Historical Trend Analysis:** Aggregates past sensor data (via MongoDB) to analyze hourly trends and predict future plant needs.
*   **Real-time Node-RED Dashboard:** Live visualization of all sensor gauges, historical charts, AI-generated literary quotes, and local weather.

## 🏗️ System Architecture

1.  **Hardware Layer (ESP32 Firmware):** Written in C++, reads data from Soil Moisture, Temperature, Humidity, Light, and CO2 sensors. Uses Secure MQTT (AWS IoT Core mTLS) to communicate. Features built-in LittleFS offline storage.
2.  **Agentic Backend (Node.js):** Handles MQTT messaging, MongoDB connections, WhatsApp client initialization, and throttles AI requests to optimize API costs.
3.  **Data & Intelligence Layer:** Uses MongoDB Atlas for historical data. Gemini AI processes data alongside OpenWeather API forecasts.
4.  **Presentation Layer:** Node-RED hosted on EC2 for real-time visualization.

## 🛠️ Technology Stack

*   **Firmware:** C++, Arduino IDE, LittleFS
*   **Backend:** Node.js, Express.js, `whatsapp-web.js`
*   **AI Integration:** `@langchain/google-genai`, Google Gemini Pro/Flash
*   **Database:** MongoDB Atlas
*   **Cloud & Infrastructure:** AWS IoT Core (mTLS), AWS EC2, AWS CloudFormation
*   **External APIs:** OpenWeatherMap API

## 📚 Required Arduino Libraries

To upload the code to your ESP32, please install the following libraries via the Arduino IDE Library Manager:
1.  **PubSubClient** by Nick O'Leary
2.  **ArduinoJson** by Benoit Blanchon (Version 6.x or 7.x)
3.  **DHT sensor library** by Adafruit (Install dependencies when asked)
4.  **LiquidCrystal I2C** by Frank de Brabander

*(Note: `WiFi.h`, `WiFiClientSecure.h`, `LittleFS.h`, and `time.h` are built into the ESP32 core and do not need to be downloaded).*

## ⚙️ Setup & Installation

1.  **Cloud Infrastructure:**
    *   Deploy the provided `cloudformation.yaml` to AWS to automatically create your Ubuntu EC2 Server.
2.  **Backend Setup (on EC2):**
    *   Clone the repository to the EC2 instance.
    *   Run `npm install` in the `/backend` directory.
    *   Configure the `.env` file with `MONGODB_URI`, `AI_API_KEY`, `WEATHER_API_KEY`, and AWS IoT paths.
    *   Start using PM2: `pm2 start npm --name "greenpulse-backend" -- start`
    *   *Note: Check `pm2 logs` once to scan the WhatsApp QR Code.*
3.  **Node-RED Setup:**
    *   Install Node-RED on EC2 and import the provided `node-red-flow.json`.
    *   Install `node-red-dashboard` from the Palette Manager.
4.  **Firmware Setup:**
    *   Open `/GreenPulse_Firmware/GreenPulse_Firmware.ino`.
    *   Add your WiFi, AWS endpoints, and keys in `secrets.h`.
    *   Upload to your ESP32 board.