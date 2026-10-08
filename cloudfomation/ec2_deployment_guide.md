# GreenPulse IoT - AWS EC2 & CloudFormation Deployment Guide

This guide provides step-by-step instructions for hosting the **Node.js Backend** and **Node-RED dashboard** on an AWS EC2 server.

---

## Step 1: Launch an EC2 Instance
If you are not using CloudFormation, set it up manually:
1. Log in to the **AWS Console** and go to the **EC2** service.
2. Click **Launch Instance**.
3. **Name:** Enter a name like `GreenPulse-Server`.
4. **AMI (OS):** Select **Ubuntu 22.04 LTS**.
5. **Instance Type:** Select `t3.small` (2GB RAM) or `t3.medium` (4GB RAM). *(Note: A 1GB `t2.micro` is not recommended as the WhatsApp AI bot requires more RAM to run Puppeteer smoothly).*
6. **Key Pair:** Create a new Key Pair and download it (e.g., `greenpulse-key.pem`).
7. **Network Settings:**
   - Create a security group.
   - Check **Allow SSH traffic from Anywhere**.
   - Check **Allow HTTP/HTTPS traffic from the internet**.
8. Click **Launch Instance**.

---

## Step 2: Open Security Group Ports
Once the instance is running, go to its Security Group and add the following **Inbound Rules**:
- **Port 1880** (Custom TCP) - Required for Node-RED.
- **Port 3000** (Custom TCP) - Required for the Node.js Backend API.

---

## Step 3: Connect to the EC2 Instance
Open your Command Prompt or Terminal in the folder where your `.pem` key was downloaded and run:
```bash
ssh -i "greenpulse-key.pem" ubuntu@<your-ec2-public-ip>
```

---

## Step 4: Install Required Software
Run the following commands sequentially on the EC2 terminal:
```bash
# Update the server
sudo apt update && sudo apt upgrade -y

# Install Node.js (Version 20)
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs

# Install PM2 (Process Manager)
sudo npm install -g pm2

# Install required Linux dependencies for WhatsApp Web (Puppeteer/Chromium)
sudo apt-get install -y libx11-xcb1 libxcomposite1 libxcursor1 libxdamage1 libxext6 libxi6 libxrender1 libxtst6 libnss3 libcups2 libxss1 libxrandr2 libasound2 libpangocairo-1.0-0 libatk1.0-0 libatk-bridge2.0-0 libgtk-3-0 libgbm-dev
```

---

## Step 5: Transfer Project Files
```bash
# Clone the repository
git clone <your-github-repo-link>
```

**Important:** Because `.env` and the AWS certificates (`keys/` folder) are ignored by Git for security, you must create them manually on the server:
1. `cd GreenPulse_IoT_System/backend`
2. Run `nano .env` and paste your local `.env` content. (Ensure `WHATSAPP_PHONE` contains the correct authorized number without the '+'). Save and exit (`Ctrl+X`, `Y`, `Enter`).
3. Run `mkdir ../keys`
4. Run `nano ../keys/certificate.pem.crt` and paste the key. Repeat this for `private.pem.key` and `AmazonRootCA1.pem`.

---

## Step 6: Start the Backend
```bash
cd ~/GreenPulse_IoT_System/backend
npm install

# Start the backend using PM2
pm2 start src/index.js --name "greenpulse-backend"

# Save the PM2 list to automatically restart the app on server reboots
pm2 save
pm2 startup
```
*(If `pm2 startup` outputs a command, copy and run it in the terminal).*

Check the logs to authenticate WhatsApp:
```bash
pm2 logs
```
Wait a few seconds for the **QR code** to appear. Scan it using the WhatsApp app on your phone to link the bot. Press `Ctrl + C` to exit the logs.

---

## Step 7: Setup Node-RED
```bash
# Install Node-RED globally
sudo npm install -g --unsafe-perm node-red

# Start Node-RED using PM2
pm2 start node-red --name "node-red"
pm2 save
```

Now, navigate to `http://<your-ec2-public-ip>:1880/ui` in your browser to access Node-RED. Import the `node-red-flow.json` file from the repository to set up the dashboard.
