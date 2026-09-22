const mqtt = require('mqtt');
const fs = require('fs');
const path = require('path');

const keysDir = path.join(__dirname, '../keys'); 
console.log('Keys dir:', keysDir);
const keyPath = path.join(keysDir, 'a00801eca39ba7913071729b2095789bf868ff285fe15a902d665175a37c04dc-private.pem.key');
const certPath = path.join(keysDir, 'a00801eca39ba7913071729b2095789bf868ff285fe15a902d665175a37c04dc-certificate.pem.crt');
const caPath = path.join(keysDir, 'AmazonRootCA1.pem');

console.log('Key exists:', fs.existsSync(keyPath));
console.log('Cert exists:', fs.existsSync(certPath));
console.log('CA exists:', fs.existsSync(caPath));

const options = {
  clientId: 'GreenPulse_Test_' + Math.random().toString(16).substr(2, 8),
  protocol: 'mqtts',
  key: fs.readFileSync(keyPath),
  cert: fs.readFileSync(certPath),
  ca: fs.readFileSync(caPath),
  rejectUnauthorized: true
};

console.log('Attempting connection...');
const client = mqtt.connect('mqtts://a1ftomq193a3f9-ats.iot.us-east-1.amazonaws.com:8883', options);

client.on('connect', () => {
  console.log('SUCCESS: Connected to MQTT broker');
  client.end();
});
client.on('error', (err) => {
  console.error('ERROR:', err);
  client.end();
});
