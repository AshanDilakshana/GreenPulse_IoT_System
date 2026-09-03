require('dotenv').config();
const { connectDB } = require('./db');
const { setupMQTT } = require('./mqttHandler');

// If deploying to Render/Koyeb, they often require binding to a PORT
// Even for worker apps, starting a dummy HTTP server can keep the deployment healthy.
const express = require('express');
const app = express();
const port = process.env.PORT || 3000;

app.get('/health', (req, res) => res.send('OK'));

const startApp = async () => {
  console.log('Starting GreenPulse Backend...');
  
  await connectDB();
  setupMQTT();
  
  app.listen(port, () => {
    console.log(`Health check server listening on port ${port}`);
  });
};

startApp();
