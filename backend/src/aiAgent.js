const axios = require('axios');
const { ChatGoogleGenerativeAI } = require('@langchain/google-genai');
const { SystemMessage, HumanMessage } = require('@langchain/core/messages');
const { sendAlertEmail } = require('./mailer');
const { saveWeatherData } = require('./db');

let criticalConsecutiveCount = 0;

const getWeatherData = async () => {
  try {
    if (!process.env.WEATHER_API_KEY || !process.env.LOCATION) {
      return "Weather data unavailable (API key or location missing).";
    }
    // Fetch forecast for rain probability
    const url = `https://api.openweathermap.org/data/2.5/forecast?q=${process.env.LOCATION}&appid=${process.env.WEATHER_API_KEY}&units=metric`;
    const response = await axios.get(url);
    const data = response.data;
    
    // Calculate rain probability for the next 12 hours (next 4 forecast slots, each is 3 hours)
    let maxPop = 0;
    if (data.list && data.list.length >= 4) {
      for (let i = 0; i < 4; i++) {
        if (data.list[i].pop > maxPop) {
          maxPop = data.list[i].pop;
        }
      }
    }
    const rainProbability = (maxPop * 100).toFixed(0);
    return `Rain probability for the next 12 hours: ${rainProbability}%`;
  } catch (error) {
    console.error("Error fetching weather forecast:", error.message);
    return "Weather forecast unavailable.";
  }
};

const analyzePlantData = async (sensorData) => {
  if (!process.env.AI_API_KEY) {
    console.log("AI API Key not configured. Skipping analysis.");
    return null;
  }

  try {
    const weatherForecast = await getWeatherData();
    await saveWeatherData(weatherForecast); // Save weather history for historical agent
    
    // Get Current Time in 24-hour format
    const now = new Date();
    const currentTime = now.toLocaleTimeString('en-US', { hour12: false, hour: '2-digit', minute: '2-digit' });
    
    // Update critical count
    if (sensorData.soilMoisture !== undefined && sensorData.soilMoisture < 20) {
      criticalConsecutiveCount++;
    } else {
      criticalConsecutiveCount = 0;
    }
    
    // Initialize Gemini Model
    const model = new ChatGoogleGenerativeAI({
      model: "gemini-3.1-flash-lite",
      maxOutputTokens: 2048,
      temperature: 0.2, // Lower temperature for more consistent JSON structure
      apiKey: process.env.AI_API_KEY,
    });

    const systemPrompt = `You are the Agentic AI Brain of "GreenPulse", an advanced smart indoor plant-care IoT system. Your primary goal is to ensure the plant's health by analyzing real-time sensor data, weather forecasts, and the current time of day. Based on this analysis, you will autonomously control hardware actuators (water pump, smart lamp), send notifications, and generate creative literature-style care messages.

You will receive input data in JSON format containing:
- Sensor Data: Soil Moisture (%), Temperature (°C), Humidity (%), CO2 levels (ppm), and Light/Lux levels.
- Context: Current Time (24-hour format), Weather Forecast (Rain probability for the next 12 hours), and Consecutive Critical Count (${criticalConsecutiveCount} times the soil has been critically dry).

Based on the input, you MUST apply the following logic and output a JSON response with your decisions:

1. SMART WATERING LOGIC:
   - Check the Weather Forecast. If Rain is predicted, strictly DELAY watering (output pump_status as "OFF") regardless of soil moisture, and inform the user.
   - If NO Rain is predicted:
       - If Soil Moisture is between 20% and 40% (Warning Level): Output indicator_color as "YELLOW" and pump_status as "OFF". Generate an email warning the user to water the plant soon.
       - If Soil Moisture is BELOW 20% (Critical Level): 
            - If Consecutive Critical Count is 1: This is the first warning. Output indicator_color as "RED" and pump_status as "OFF". Generate an URGENT email warning.
            - If Consecutive Critical Count is > 1: This means the user ignored the RED warning. Output indicator_color as "RED" and pump_status as "ON" to save the plant. IMPORTANT: When pump_status is "ON", you MUST set "target_moisture" to a healthy optimal percentage (e.g. 60 or 70) so the hardware can auto-stop the pump when reached.
       - If Soil Moisture is above 40%: Output indicator_color as "GREEN" and pump_status as "OFF". Set "target_moisture" to 0.

2. TIME-BASED LIGHTING LOGIC:
   - If Light Level is LOW:
       - Check the Current Time.
       - If it is DAYTIME (06:00 to 18:00): Output smart_lamp_status as "ON" to support photosynthesis.
       - If it is NIGHTTIME (after 18:00 to 05:59): Output smart_lamp_status as "OFF" to respect the plant's natural dark/resting period.
   - If Light Level is ADEQUATE or HIGH (sufficient natural light): Output smart_lamp_status as "OFF".

3. AIR QUALITY & TEMPERATURE LOGIC:
   - If CO2 levels are HIGH or Temperature is detrimental to the plant, generate a practical action item (e.g., "Please open a window for better air circulation").

4. CARE QUOTE GENERATION:
   - Generate a short, beautiful, literature-style quote (1-2 sentences) reflecting the plant's current state (e.g., its thirst, the warmth of the room, or the air quality).

5. INDICATOR STATUS:
   - Determine the Care Urgency and output an indicator_color: "GREEN" (Good), "YELLOW" (Warning/Action Needed soon), or "RED" (Critical/Pump Activated).

OUTPUT FORMAT:
Return ONLY a valid JSON object with the following keys: 
{
  "pump_status": "ON" | "OFF" | "DELAYED",
  "target_moisture": number,
  "smart_lamp_status": "ON" | "OFF",
  "indicator_color": "GREEN" | "YELLOW" | "RED",
  "email_alert_body": "string",
  "dashboard_care_quote": "string"
}`;

    const userPrompt = `Input Data:
{
  "Sensor Data": {
    "Soil Moisture (%)": ${sensorData.soilMoisture !== undefined ? sensorData.soilMoisture : "N/A"},
    "Temperature (°C)": ${sensorData.temperature !== undefined ? sensorData.temperature : "N/A"},
    "Humidity (%)": ${sensorData.humidity !== undefined ? sensorData.humidity : "N/A"},
    "CO2 levels (ppm)": ${sensorData.co2 !== undefined ? sensorData.co2 : "N/A"},
    "Light/Lux levels": ${sensorData.light !== undefined ? sensorData.light : "N/A"}
  },
  "Context": {
    "Current Time": "${currentTime}",
    "Weather Forecast": "${weatherForecast}"
  }
}`;

    const res = await model.invoke([
      new SystemMessage(systemPrompt),
      new HumanMessage(userPrompt)
    ]);
    
    let resultText = res.content.trim();
    // Forcefully extract JSON object using regex to ignore any surrounding conversational text
    const jsonMatch = resultText.match(/\{[\s\S]*\}/);
    if (jsonMatch) {
      resultText = jsonMatch[0];
    }
    
    const jsonResult = JSON.parse(resultText);

    // If there is an email alert body that isn't just empty or placeholder, send it
    if (jsonResult.email_alert_body && jsonResult.email_alert_body.length > 5 && jsonResult.email_alert_body.toLowerCase() !== "none") {
      await sendAlertEmail(
        "GreenPulse: Plant Care Notification", 
        `${jsonResult.email_alert_body}\n\nQuote: ${jsonResult.dashboard_care_quote}\nSensor Data: ${JSON.stringify(sensorData, null, 2)}`
      );
    }

    return jsonResult;

  } catch (error) {
    console.error("AI Agent error:", error);
    return null;
  }
};

const generateHistoricalSummary = async (aggregatedSensorData, pastWeatherData) => {
  if (!process.env.AI_API_KEY) {
    console.log("AI API Key not configured. Skipping historical analysis.");
    return null;
  }

  try {
    const model = new ChatGoogleGenerativeAI({
      model: "gemini-3.5-flash",
      maxOutputTokens: 1024,
      temperature: 0.3,
      apiKey: process.env.AI_API_KEY,
    });

    const systemPrompt = `You are the Expert Agricultural Analyst AI for "GreenPulse", an advanced smart indoor plant-care IoT system.
Your task is to analyze historical plant data (hourly averages for the last 6 hours) along with recent weather forecasts, and provide a comprehensive summary, trend analysis, and prediction.

You must return a valid JSON object matching this exact format:
{
  "summary": "A brief overview of the plant's condition over the past 6 hours.",
  "trend_analysis": "Meaningful insights about how temperature, moisture, light, or CO2 have fluctuated.",
  "plant_status": "Overall health status string (e.g., 'Excellent', 'Needs Attention', 'Thirsty', 'Overheated').",
  "prediction": "A prediction for the next 12-24 hours (e.g., when watering will be required next, or if temperature needs adjusting).",
  "action_items": [
    "Actionable tip 1",
    "Actionable tip 2"
  ]
}`;

    const userPrompt = `Here is the historical data for the last 6 hours:

Aggregated Sensor Data (Hourly Averages):
${JSON.stringify(aggregatedSensorData, null, 2)}

Recent Weather Forecasts:
${JSON.stringify(pastWeatherData, null, 2)}

Please analyze this and provide the required JSON output.`;

    const res = await model.invoke([
      new SystemMessage(systemPrompt),
      new HumanMessage(userPrompt)
    ]);

    let resultText = res.content.trim();
    if(resultText.startsWith("\`\`\`json")) {
        resultText = resultText.replace(/\`\`\`json/g, '').replace(/\`\`\`/g, '').trim();
    }

    return JSON.parse(resultText);

  } catch (error) {
    console.error("Historical AI Agent error:", error);
    return { error: "Failed to generate historical summary." };
  }
};

module.exports = { analyzePlantData, generateHistoricalSummary };
