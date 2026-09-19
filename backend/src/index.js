require('dotenv').config();
const { connectDB } = require('./db');
const { setupMQTT } = require('./mqttHandler');

// If deploying to Render/Koyeb, they often require binding to a PORT
// Even for worker apps, starting a dummy HTTP server can keep the deployment healthy.
const express = require('express');
const app = express();
const port = process.env.PORT || 3000;

const { getAggregatedPastData, getPastWeatherData } = require('./db');
const { generateHistoricalSummary } = require('./aiAgent');

app.get('/health', (req, res) => res.send('OK'));

app.get('/api/summary', async (req, res) => {
  try {
    const hours = parseInt(req.query.hours) || 6; // Default to 6 hours
    
    // Fetch aggregated data
    const aggregatedData = await getAggregatedPastData(hours);
    const pastWeather = await getPastWeatherData(hours);
    
    if (!aggregatedData || aggregatedData.length === 0) {
      return res.status(404).json({ error: "No historical data found for the specified period." });
    }

    // Generate AI Summary
    const summary = await generateHistoricalSummary(aggregatedData, pastWeather);
    
    if (summary) {
      res.json(summary);
    } else {
      res.status(500).json({ error: "Failed to generate AI summary." });
    }
  } catch (error) {
    console.error("Error in /api/summary:", error);
    res.status(500).json({ error: "Internal server error" });
  }
});

const startApp = async () => {
  console.log('Starting GreenPulse Backend...');
  
  await connectDB();
  setupMQTT();
  
  app.listen(port, () => {
    console.log(`Health check server listening on port ${port}`);
  });
};

startApp();
