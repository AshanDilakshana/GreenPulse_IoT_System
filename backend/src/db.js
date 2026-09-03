const mongoose = require('mongoose');

const sensorDataSchema = new mongoose.Schema({
  temperature: Number,
  humidity: Number,
  soilMoisture: Number,
  timestamp: { type: Date, default: Date.now }
});

const SensorData = mongoose.model('SensorData', sensorDataSchema);

const connectDB = async () => {
  try {
    const uri = process.env.MONGODB_URI;
    if (!uri) {
      console.warn('MongoDB URI is not defined in .env. Skipping DB connection.');
      return;
    }
    await mongoose.connect(uri);
    console.log('Connected to MongoDB Atlas');
  } catch (error) {
    console.error('MongoDB connection error:', error);
  }
};

const saveSensorData = async (data) => {
  if (mongoose.connection.readyState !== 1) return; // Not connected
  try {
    const newReading = new SensorData(data);
    await newReading.save();
    console.log('Saved sensor data to MongoDB');
  } catch (error) {
    console.error('Error saving data to MongoDB:', error);
  }
};

module.exports = { connectDB, saveSensorData };
