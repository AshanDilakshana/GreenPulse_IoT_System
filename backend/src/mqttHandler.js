const mqtt = require('mqtt');
const { saveSensorData } = require('./db');
const { analyzePlantData } = require('./aiAgent');

const fs = require('fs');
const path = require('path');

const setupMQTT = () => {
  if (!process.env.MQTT_BROKER_URL) {
    console.warn("MQTT_BROKER_URL not set in .env. Skipping MQTT setup.");
    return;
  }

  // Paths to AWS IoT Core Certificates
  const keysDir = path.join(__dirname, '../../keys'); 
  const keyPath = path.join(keysDir, 'a00801eca39ba7913071729b2095789bf868ff285fe15a902d665175a37c04dc-private.pem.key');
  const certPath = path.join(keysDir, 'a00801eca39ba7913071729b2095789bf868ff285fe15a902d665175a37c04dc-certificate.pem.crt');
  const caPath = path.join(keysDir, 'AmazonRootCA1.pem');

  const options = {
    clientId: 'GreenPulse_Backend_' + Math.random().toString(16).substr(2, 8),
    protocol: 'mqtts',
  };

  // If the AWS certs exist, add them to options for mTLS
  if (fs.existsSync(keyPath) && fs.existsSync(certPath) && fs.existsSync(caPath)) {
    console.log('[MQTT] Successfully loaded AWS mTLS certificates.');
    options.key = fs.readFileSync(keyPath);
    options.cert = fs.readFileSync(certPath);
    options.ca = fs.readFileSync(caPath);
    options.rejectUnauthorized = true;
  } else {
    console.log('[MQTT] Certificates not found. Falling back to username/password.');
    // Fallback to standard username/password MQTT (e.g. HiveMQ)
    options.username = process.env.MQTT_USER;
    options.password = process.env.MQTT_PASSWORD;
  }

  console.log('[MQTT] Connecting to:', process.env.MQTT_BROKER_URL);
  const client = mqtt.connect(process.env.MQTT_BROKER_URL, options);

  client.on('connect', () => {
    console.log('Connected to MQTT broker securely.');
    client.subscribe('greenpulse/sensors', (err) => {
      if (err) console.error('Subscription error:', err);
      else console.log('Subscribed to greenpulse/sensors');
    });
    client.subscribe('greenpulse/trigger_summary', (err) => {
      if (err) console.error('Subscription error for trigger_summary:', err);
      else console.log('Subscribed to greenpulse/trigger_summary');
    });
  });

  let lastAITime = 0;
  let lastDbSaveTime = 0;

  client.on('message', async (topic, message) => {
    if (topic === 'greenpulse/sensors') {
      try {
        console.log('Received sensor data:', message.toString());
        const sensorData = JSON.parse(message.toString());
        
        const now = Date.now();
        
        // 1. Log to Database (Every 10 minutes - 600000 ms)
        if (now - lastDbSaveTime >= 600000) {
          lastDbSaveTime = now;
          await saveSensorData(sensorData);
          console.log('[DB] Saved sensor data to MongoDB');
        }

        // Throttle AI Agent calls to once every 60 seconds (prevent rate limits)
        if (now - lastAITime >= 60000) {
          lastAITime = now;
          
          // 2. Analyze with AI Agent
          const aiResponse = await analyzePlantData(sensorData);

          // 3. Publish response back for ESP32 and Node-RED
          if (aiResponse) {
            client.publish('greenpulse/alerts', JSON.stringify(aiResponse));
            console.log('Published AI response to greenpulse/alerts');
          }
        }
        
      } catch (error) {
        console.error('Error processing MQTT message:', error);
      }
    } else if (topic === 'greenpulse/trigger_summary') {
      try {
        console.log('Manual AI Summary Trigger received via MQTT.');
        const { getAggregatedPastData, getPastWeatherData } = require('./db');
        const { generateHistoricalSummary } = require('./aiAgent');
        
        const aggregatedData = await getAggregatedPastData(6);
        const pastWeather = await getPastWeatherData(6);
        
        if (!aggregatedData || aggregatedData.length === 0) {
          console.log("No historical data found.");
          return;
        }

        const summary = await generateHistoricalSummary(aggregatedData, pastWeather);
        
        if (summary) {
          client.publish('greenpulse/summary', JSON.stringify(summary));
          console.log('Published historical summary to greenpulse/summary');
        }
      } catch (error) {
        console.error('Error processing trigger_summary:', error);
      }
    }
  });

  client.on('error', (err) => {
    console.error('MQTT connection error:', err);
  });
};

module.exports = { setupMQTT };
