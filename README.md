# GreenPulse IoT System 🌱

GreenPulse is an advanced, Agentic AI-powered Smart Indoor Plant Care system. It autonomously monitors plant health, controls physical actuators (water pump, smart lamp), and generates human-like insights using real-time sensor data, weather forecasts, and historical trends.

## 🌟 Key Features

*   **Agentic AI Brain:** Uses Google's Gemini AI (via LangChain) to make intelligent, context-aware decisions rather than relying on simple `if/else` rules.
*   **Weather-Aware Smart Watering:** Checks the OpenWeather API before watering. If the soil is dry but rain is predicted in the next 12 hours, it delays watering and alerts the user to prevent overwatering.
*   **Time-Based Smart Lighting:** Automatically controls a smart lamp based on ambient light levels and the current time of day, ensuring the plant gets its natural dark/resting period at night.
*   **Historical Trend Analysis:** Aggregates past sensor data (via MongoDB) to analyze hourly trends and predict future plant needs without exhausting API token limits.
*   **Hardware Safety Controls:** Implements "Pulse Watering" at the firmware level. The water pump automatically shuts off after 15 seconds to prevent accidental flooding, even if the AI or backend fails.
*   **Real-time Dashboard:** Designed to integrate with Node-RED for real-time visualization of sensor data and AI-generated literary quotes about the plant's mood.

## 🏗️ System Architecture

The system consists of three main layers:

1.  **Hardware Layer (ESP32 Firmware):** 
    Written in C++, it reads data from Soil Moisture, Temperature, Humidity, Light, and CO2 sensors every 2 seconds. It communicates with the backend via secure MQTT (AWS IoT Core) and controls the Water Pump, Smart Lamp, and an RGB Status LED.
2.  **Agentic Backend (Node.js):** 
    A unified JavaScript backend that handles MQTT messaging, MongoDB connections, and Express APIs. It throttles AI requests to once every 20 seconds to optimize API costs while maintaining real-time sensor logging.
3.  **Data & Intelligence Layer:** 
    Uses MongoDB Atlas to store real-time and aggregated historical data. Gemini AI (LangChain-JS) processes this data alongside OpenWeather API forecasts to make autonomous decisions.

## 🛠️ Technology Stack

*   **Firmware:** C++, Arduino IDE, PubSubClient, ArduinoJson
*   **Backend:** Node.js, Express.js
*   **AI Integration:** `@langchain/google-genai`, Google Gemini Pro/Flash
*   **Database:** MongoDB Atlas, Mongoose (with Aggregation Pipelines)
*   **Messaging:** MQTT via AWS IoT Core (mTLS)
*   **External APIs:** OpenWeatherMap API

## 🚀 Why JavaScript (Node.js) for AI?

While Python is traditionally used for AI, this project utilizes **Node.js** for the entire backend stack. This decision allows for incredibly fast asynchronous handling of real-time MQTT streams while maintaining a single, unified technology stack. The `@langchain/google-genai` library brings full Agentic capabilities (prompting, memory, structuring) directly into the JavaScript ecosystem, eliminating the need for a separate Python microservice.

## 📂 Project Structure

*   `/GreenPulse_Firmware` - ESP32 C++ source code and libraries.
*   `/backend` - Node.js server, AI Agent logic, database schemas, and API routes.

## ⚙️ Setup & Installation

1.  **Backend Setup:**
    *   Navigate to the `/backend` directory.
    *   Run `npm install`.
    *   Configure the `.env` file with your `MONGODB_URI`, `AI_API_KEY`, `WEATHER_API_KEY`, and AWS IoT certificates.
    *   Start the server: `npm start`
2.  **Firmware Setup:**
    *   Open `/GreenPulse_Firmware/GreenPulse_Firmware.ino` in Arduino IDE.
    *   Add your WiFi credentials and AWS endpoints in `secrets.h`.
    *   Upload to your ESP32 board.