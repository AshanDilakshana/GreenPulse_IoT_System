const axios = require('axios');
const { ChatGoogleGenerativeAI } = require('@langchain/google-genai');
const { SystemMessage, HumanMessage } = require('@langchain/core/messages');
const { sendAlertEmail } = require('./mailer');

const getWeatherData = async () => {
  try {
    if (!process.env.WEATHER_API_KEY || !process.env.LOCATION) {
      return "Weather data unavailable (API key or location missing).";
    }
    const url = `https://api.openweathermap.org/data/2.5/weather?q=${process.env.LOCATION}&appid=${process.env.WEATHER_API_KEY}&units=metric`;
    const response = await axios.get(url);
    const data = response.data;
    return `${data.weather[0].description}, Temp: ${data.main.temp}°C, Humidity: ${data.main.humidity}%`;
  } catch (error) {
    console.error("Error fetching weather:", error.message);
    return "Weather data unavailable.";
  }
};

const analyzePlantData = async (sensorData) => {
  if (!process.env.AI_API_KEY) {
    console.log("AI API Key not configured. Skipping analysis.");
    return null;
  }

  try {
    const weatherInfo = await getWeatherData();
    
    // Initialize Gemini Model
    const model = new ChatGoogleGenerativeAI({
      modelName: "gemini-1.5-pro",
      maxOutputTokens: 256,
      apiKey: process.env.AI_API_KEY,
    });

    const systemPrompt = `You are a poetic but precise AI plant care assistant. 
You will receive live sensor data (Soil Moisture, Temp, Humidity) and current outside weather.
Your job is to generate a JSON response with exactly three keys:
1. "color": A hex color code for an RGB LED representing priority. (#00FF00 for good/happy, #FF0000 for critical/needs water, #FFA500 for warning/soon).
2. "quote": A short, literature-style poetic quote about plant care or nature reflecting the plant's current state. (max 1 sentence)
3. "alert": A short, practical summary of the plant's status (e.g., "Soil is bone dry, water immediately!").

Return ONLY valid JSON. Do not include markdown formatting like \`\`\`json.`;

    const userPrompt = `Sensor Data: Soil Moisture: ${sensorData.soilMoisture}%, Temp: ${sensorData.temperature}°C, Humidity: ${sensorData.humidity}%
Outside Weather: ${weatherInfo}`;

    const res = await model.invoke([
      new SystemMessage(systemPrompt),
      new HumanMessage(userPrompt)
    ]);
    
    let resultText = res.content.trim();
    if(resultText.startsWith("\`\`\`json")) {
        resultText = resultText.replace(/\`\`\`json/g, '').replace(/\`\`\`/g, '').trim();
    }

    const jsonResult = JSON.parse(resultText);

    // If critical, send email
    if (jsonResult.color.toUpperCase() === '#FF0000' || sensorData.soilMoisture < 20) {
      await sendAlertEmail("CRITICAL: GreenPulse Plant Alert", `Alert: ${jsonResult.alert}\nQuote: ${jsonResult.quote}\nSensor Data: ${JSON.stringify(sensorData)}`);
    }

    return jsonResult;

  } catch (error) {
    console.error("AI Agent error:", error);
    return null;
  }
};

module.exports = { analyzePlantData };
