const mongoose = require('mongoose');

const sensorDataSchema = new mongoose.Schema({
  temperature: Number,
  humidity: Number,
  soilMoisture: Number,
  co2: Number,
  light: Number,
  timestamp: { type: Date, default: Date.now }
});

const weatherHistorySchema = new mongoose.Schema({
  forecastString: String,
  timestamp: { type: Date, default: Date.now }
});

const SensorData = mongoose.model('SensorData', sensorDataSchema);
const WeatherHistory = mongoose.model('WeatherHistory', weatherHistorySchema);

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
    // Intentionally omitting console.log here to avoid spamming every 2s
  } catch (error) {
    console.error('Error saving data to MongoDB:', error);
  }
};

const saveWeatherData = async (weatherString) => {
  if (mongoose.connection.readyState !== 1) return;
  try {
    const newWeather = new WeatherHistory({ forecastString: weatherString });
    await newWeather.save();
  } catch (error) {
    console.error('Error saving weather data:', error);
  }
};

// Returns hourly averages for the past `hours` limit
const getAggregatedPastData = async (hours = 6) => {
  if (mongoose.connection.readyState !== 1) return [];
  try {
    const pastDate = new Date(Date.now() - hours * 60 * 60 * 1000);
    
    const aggregatedData = await SensorData.aggregate([
      { $match: { timestamp: { $gte: pastDate } } },
      {
        $group: {
          _id: {
            year: { $year: "$timestamp" },
            month: { $month: "$timestamp" },
            day: { $dayOfMonth: "$timestamp" },
            hour: { $hour: "$timestamp" }
          },
          avgTemp: { $avg: "$temperature" },
          avgHumidity: { $avg: "$humidity" },
          avgMoisture: { $avg: "$soilMoisture" },
          avgCo2: { $avg: "$co2" },
          avgLight: { $avg: "$light" }
        }
      },
      { $sort: { "_id.year": 1, "_id.month": 1, "_id.day": 1, "_id.hour": 1 } }
    ]);
    
    // Format output
    return aggregatedData.map(data => ({
      time: `${data._id.year}-${data._id.month}-${data._id.day} ${data._id.hour}:00`,
      temperature: data.avgTemp ? parseFloat(data.avgTemp.toFixed(1)) : null,
      humidity: data.avgHumidity ? parseFloat(data.avgHumidity.toFixed(1)) : null,
      soilMoisture: data.avgMoisture ? parseFloat(data.avgMoisture.toFixed(1)) : null,
      co2: data.avgCo2 ? parseFloat(data.avgCo2.toFixed(1)) : null,
      light: data.avgLight ? parseFloat(data.avgLight.toFixed(1)) : null
    }));
  } catch (error) {
    console.error("Error aggregating past data:", error);
    return [];
  }
};

const getPastWeatherData = async (hours = 6) => {
  if (mongoose.connection.readyState !== 1) return [];
  try {
    const pastDate = new Date(Date.now() - hours * 60 * 60 * 1000);
    // Get distinct or recent weather forecasts (e.g. 1 per hour)
    // To simplify, we'll just get the latest 5 forecasts within the timeframe
    const weatherData = await WeatherHistory.find({ timestamp: { $gte: pastDate } })
                                            .sort({ timestamp: -1 })
                                            .limit(5);
    return weatherData.map(w => ({ time: w.timestamp, forecast: w.forecastString }));
  } catch (error) {
    console.error("Error fetching past weather:", error);
    return [];
  }
};

module.exports = { connectDB, saveSensorData, saveWeatherData, getAggregatedPastData, getPastWeatherData };
