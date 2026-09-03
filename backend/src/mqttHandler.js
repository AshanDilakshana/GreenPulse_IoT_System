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
  const keysDir = path.join(__dirname, '../../../keys'); 
  const keyPath = path.join(keysDir, 'a00801eca39ba7913071729b2095789bf868ff285fe15a902d665175a37c04dc-private.pem.key');
  const certPath = path.join(keysDir, 'a00801eca39ba7913071729b2095789bf868ff285fe15a902d665175a37c04dc-certificate.pem.crt');
  const caPath = path.join(keysDir, 'AmazonRootCA1.pem');

  const options = {
    clientId: 'GreenPulse_Backend_' + Math.random().toString(16).substr(2, 8),
    protocol: 'mqtts',
  };

  // If the AWS certs exist, add them to options for mTLS
  if (fs.existsSync(keyPath) && fs.existsSync(certPath) && fs.existsSync(caPath)) {
    options.key = fs.readFileSync(keyPath);
    options.cert = fs.readFileSync(certPath);
    options.ca = fs.readFileSync(caPath);
  } else {
    // Fallback to standard username/password MQTT (e.g. HiveMQ)
    options.username = process.env.MQTT_USER;
    options.password = process.env.MQTT_PASSWORD;
  }

  const client = mqtt.connect(process.env.MQTT_BROKER_URL, options);

  client.on('connect', () => {
    console.log('Connected to MQTT broker securely.');
    client.subscribe('greenpulse/sensors', (err) => {
      if (err) console.error('Subscription error:', err);
      else console.log('Subscribed to greenpulse/sensors');
    });
  });

  client.on('message', async (topic, message) => {
    if (topic === 'greenpulse/sensors') {
      try {
        console.log('Received sensor data:', message.toString());
        const sensorData = JSON.parse(message.toString());
        
        // 1. Log to Database
        await saveSensorData(sensorData);

        // 2. Analyze with AI Agent
        const aiResponse = await analyzePlantData(sensorData);

        // 3. Publish response back for ESP32 and Node-RED
        if (aiResponse) {
          client.publish('greenpulse/alerts', JSON.stringify(aiResponse));
          console.log('Published AI response to greenpulse/alerts');
        }
        
      } catch (error) {
        console.error('Error processing MQTT message:', error);
      }
    }
  });

  client.on('error', (err) => {
    console.error('MQTT connection error:', err);
  });
};

module.exports = { setupMQTT };
