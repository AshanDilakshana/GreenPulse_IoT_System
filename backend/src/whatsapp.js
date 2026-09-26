const { Client, LocalAuth } = require('whatsapp-web.js');
const qrcode = require('qrcode-terminal');

let isClientReady = false;

// Initialize WhatsApp Web Client
const client = new Client({
  authStrategy: new LocalAuth(), // Saves the session so you don't have to scan every time
  puppeteer: {
    args: ['--no-sandbox', '--disable-setuid-sandbox']
  },
  webVersionCache: {
    type: 'remote',
    remotePath: 'https://raw.githubusercontent.com/wppconnect-team/wa-version/main/html/2.2412.54.html',
  }
});

client.on('qr', (qr) => {
  console.log('===========================================================');
  console.log('📱 SCAN THIS QR CODE IN WHATSAPP TO LINK THE BOT 📱');
  console.log('===========================================================');
  qrcode.generate(qr, { small: true });
});

client.on('authenticated', () => {
  console.log('⏳ WhatsApp Client is authenticated! Loading chats (this may take a few seconds)...');
});

client.on('ready', () => {
  console.log('✅ WhatsApp Client is fully ready and linked!');
  isClientReady = true;
});

client.on('disconnected', (reason) => {
  console.log('❌ WhatsApp Client was disconnected', reason);
  isClientReady = false;
});

// === INCOMING MESSAGE LISTENER (2-WAY CONVERSATIONAL AI) ===
client.on('message', async (msg) => {
  const { parseWhatsAppCommand } = require('./aiAgent');
  const { publishMQTT } = require('./mqttHandler');

  const myPhone = process.env.WHATSAPP_PHONE;
  if (!myPhone) return;
  
  let formattedNumber = myPhone.replace('+', '').trim();
  const rawNumber = formattedNumber.replace('@c.us', '');
  const ownerLid = process.env.WHATSAPP_OWNER_LID || ''; // The owner's Linked ID (LID)

  // Get real contact info (fixes @lid hidden numbers if possible)
  const contact = await msg.getContact();
  const senderNumber = (contact && contact.number) ? contact.number : msg.from;

  // STRICT SECURITY CHECK: Allow the authorized phone OR the owner's specific @lid
  if (!senderNumber.includes(rawNumber) && !senderNumber.includes(ownerLid)) {
    // Silently ignore all other messages
    return;
  }

  if (!msg.body || msg.body.trim() === '') {
    // Ignore empty sync messages to save AI quota
    return;
  }

  console.log(`[WhatsApp] Received authorized message: "${msg.body}"`);

  // Parse the message using Gemini AI
  const aiCommand = await parseWhatsAppCommand(msg.body);
  if (!aiCommand) return;

  console.log(`[WhatsApp AI Parser] Intent: ${aiCommand.intent}, Time: ${aiCommand.time}`);

  // Send the AI's reply back to the user
  if (aiCommand.replyMessage) {
    await client.sendMessage(msg.from, aiCommand.replyMessage);
  }

  // Hardware Command Logic
  if (aiCommand.intent === 'TURN_ON_PUMP' || aiCommand.intent === 'TURN_OFF_PUMP') {
    const pumpStatus = aiCommand.intent === 'TURN_ON_PUMP' ? 'ON' : 'OFF';
    const commandPayload = JSON.stringify({ pump_status: pumpStatus });

    if (aiCommand.time === 'NOW') {
      publishMQTT('greenpulse/commands', commandPayload);
    } else {
      // Very basic time-scheduling logic (e.g. HH:MM for today)
      // For a real production app, use node-schedule or agenda
      try {
        const [targetHour, targetMinute] = aiCommand.time.split(':').map(Number);
        const now = new Date();
        const targetTime = new Date();
        targetTime.setHours(targetHour, targetMinute, 0, 0);

        // If time has already passed today, assume tomorrow
        if (targetTime < now) {
          targetTime.setDate(targetTime.getDate() + 1);
        }

        const msDelay = targetTime.getTime() - now.getTime();
        console.log(`[WhatsApp] Scheduling pump ${pumpStatus} in ${msDelay}ms`);
        
        setTimeout(() => {
          publishMQTT('greenpulse/commands', commandPayload);
          client.sendMessage(msg.from, `🔔 (Scheduled Task) Water pump is now ${pumpStatus}!`);
        }, msDelay);

      } catch (e) {
        console.error("Error scheduling time:", e);
      }
    }
  }
});
// ============================================================

const fs = require('fs');
const path = require('path');

// Clean up orphan Puppeteer locks before starting
const sessionPath = path.join(__dirname, '..', '.wwebjs_auth', 'session');
const lockFile = path.join(sessionPath, 'SingletonLock');

try {
  if (fs.existsSync(lockFile)) fs.unlinkSync(lockFile);
  // DO NOT DELETE SingletonCookie, it contains the login session!
} catch (e) {
  console.log("Could not clear locks, ignoring...");
}

// Start the client
client.initialize();

const generateWhatsAppProgressBar = (value, min, max) => {
  const percentage = Math.max(0, Math.min(100, ((value - min) / (max - min)) * 100));
  const filledBlocks = Math.round((percentage / 100) * 10);
  const emptyBlocks = 10 - filledBlocks;
  
  // Use colored squares based on level
  let square = '🟩';
  if (percentage < 20 || percentage > 80) square = '🟥';
  else if (percentage < 40 || percentage > 60) square = '🟨';

  return `[${square.repeat(filledBlocks)}${'⬜'.repeat(emptyBlocks)}]`;
};

const sendWhatsAppAlert = async (aiText, sensorData) => {
  if (!isClientReady) {
    console.log("⚠️ WhatsApp Client is not ready yet. Please scan the QR code in the terminal.");
    return;
  }

  const phone = process.env.WHATSAPP_PHONE; // e.g. 9471XXXXXXX (without +)
  if (!phone) {
    console.log("⚠️ WHATSAPP_PHONE is not set in .env");
    return;
  }

  // Format the number for whatsapp-web.js (must end with @c.us)
  let formattedNumber = phone.replace('+', '').trim();
  if (!formattedNumber.endsWith('@c.us')) {
    formattedNumber = `${formattedNumber}@c.us`;
  }

  // Build the WhatsApp message string with Markdown and Emojis
  let formattedMessage = `🚨 *GREENPULSE CRITICAL ALERT* 🚨\n\n`;
  formattedMessage += `${aiText}\n\n`;
  formattedMessage += `📊 *Current Sensor Readings:*\n`;
  
  if (sensorData) {
    formattedMessage += `💧 *Soil Moisture:* ${sensorData.soilMoisture}% \n${generateWhatsAppProgressBar(sensorData.soilMoisture, 0, 100)}\n\n`;
    formattedMessage += `🌡️ *Temperature:* ${sensorData.temperature}°C \n${generateWhatsAppProgressBar(sensorData.temperature, 0, 50)}\n\n`;
    formattedMessage += `☁️ *Humidity:* ${sensorData.humidity}% \n${generateWhatsAppProgressBar(sensorData.humidity, 0, 100)}\n\n`;
    formattedMessage += `☀️ *Light Level:* ${sensorData.light} Lux \n${generateWhatsAppProgressBar(sensorData.light, 0, 1000)}\n`;
  }

  formattedMessage += `\n_Generated by GreenPulse AI Engine_`;

  try {
    await client.sendMessage(formattedNumber, formattedMessage);
    console.log(`📩 WhatsApp alert sent successfully to ${phone}!`);
  } catch (error) {
    console.error('❌ Error sending WhatsApp alert:', error.message);
  }
};

// Handle graceful shutdown to prevent session cache corruption
const cleanup = async () => {
  console.log('\n🛑 Safely shutting down WhatsApp client to save session...');
  if (isClientReady) {
    await client.destroy();
  }
  process.exit(0);
};

process.on('SIGINT', cleanup);
process.on('SIGTERM', cleanup);

module.exports = { sendWhatsAppAlert };
