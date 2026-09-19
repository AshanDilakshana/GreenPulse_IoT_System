require('dotenv').config();
const mongoose = require('mongoose');

const sensorDataSchema = new mongoose.Schema({
  temperature: Number,
  humidity: Number,
  soilMoisture: Number,
  co2: Number,
  light: Number,
  timestamp: { type: Date, default: Date.now }
});
const SensorData = mongoose.models.SensorData || mongoose.model('SensorData', sensorDataSchema);

const test = async () => {
  await mongoose.connect(process.env.MONGODB_URI);
  console.log("Connected to DB for testing");

  // Create dummy data from 1 hour ago
  const oneHourAgo = new Date(Date.now() - 60 * 60 * 1000);
  const dummy1 = new SensorData({
    temperature: 28, humidity: 60, soilMoisture: 40, co2: 400, light: 1000, timestamp: oneHourAgo
  });
  await dummy1.save();

  console.log("Inserted dummy data. Curling the API...");
  
  const { execSync } = require('child_process');
  try {
    const res = execSync('curl -s "http://localhost:3001/api/summary?hours=6"').toString();
    console.log("API Response:");
    console.log(res);
  } catch (e) {
    console.error("Curl failed", e);
  }

  // Cleanup
  await SensorData.deleteOne({ _id: dummy1._id });
  console.log("Cleaned up dummy data");
  process.exit(0);
};

test();
