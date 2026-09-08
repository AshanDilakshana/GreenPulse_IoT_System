# Node-RED Dashboard Setup for GreenPulse

This guide will walk you through setting up a Node-RED dashboard to visualize the GreenPulse sensor data and AI responses.

## Prerequisites
1. Ensure Node-RED is installed and running.
2. Install the Dashboard nodes. In Node-RED, go to **Manage Palette** -> **Install** and search for `node-red-dashboard`. Install it.

## Step-by-Step Configuration

### 1. Configure the MQTT Broker Node
1. Drag an **mqtt in** node to the flow.
2. Double-click it and click the pencil icon next to "Server" to add a new MQTT broker.
3. **Server**: `broker.hivemq.com` (or your chosen broker URL).
4. **Port**: `8883`.
5. Check **Enable secure (SSL/TLS) connection**. You can create a default TLS configuration.
6. (Optional) In the Security tab, add your username and password if your broker requires it.

### 2. Visualize Sensor Data (`greenpulse/sensors`)
1. Create another **mqtt in** node.
2. Set the topic to: `greenpulse/sensors`
3. Output: `a parsed JSON object`.
4. Connect the output to three separate **function** nodes to extract values:
   - **Temp Function**: `msg.payload = msg.payload.temperature; return msg;`
   - **Humidity Function**: `msg.payload = msg.payload.humidity; return msg;`
   - **Soil Function**: `msg.payload = msg.payload.soilMoisture; return msg;`
5. From the dashboard palette, drag three **gauge** nodes to the flow.
6. Connect the outputs of the function nodes to the respective gauge nodes.
7. Double click each gauge to configure its label (Temperature °C, Humidity %, Soil Moisture %), Range (e.g., 0-100), and assign them to a Dashboard Group (e.g., "Sensor Data").

### 3. Visualize AI Alerts & Quotes (`greenpulse/alerts`)
1. Drag another **mqtt in** node.
2. Set the topic to: `greenpulse/alerts`
3. Output: `a parsed JSON object`.
4. Connect the output to two **function** nodes:
   - **Alert Function**: `msg.payload = msg.payload.alert; return msg;`
   - **Quote Function**: `msg.payload = msg.payload.quote; return msg;`
5. From the dashboard palette, drag two **text** (ui_text) nodes to the flow.
6. Connect the outputs of the function nodes to the respective text nodes.
7. Configure the text nodes (Label: "AI Alert" and "Plant Quote") and assign them to a Dashboard Group (e.g., "AI Insights").

### 4. Deploy and View
1. Click the red **Deploy** button in the top right corner.
2. Navigate to your dashboard UI, typically at `http://localhost:1880/ui` (or your server's IP address).

You will now see live updating gauges when the ESP32 publishes data, and dynamic quotes/alerts when the backend AI processes it!
