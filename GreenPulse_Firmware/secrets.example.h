#ifndef SECRETS_H
#define SECRETS_H

// Wi-Fi Credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// AWS IoT Core Broker Configuration
const char* MQTT_BROKER = "your-ats-endpoint.iot.region.amazonaws.com";
const int MQTT_PORT = 8883;
const char* MQTT_CLIENT_ID = "GreenPulse_ESP32_01";

// Amazon Root CA 1
const char* ROOT_CA = \
"-----BEGIN CERTIFICATE-----\n" \
"... (Paste AmazonRootCA1.pem here) ...\n" \
"-----END CERTIFICATE-----\n";

// Device Certificate
const char* CERTIFICATE = \
"-----BEGIN CERTIFICATE-----\n" \
"... (Paste xxxxx-certificate.pem.crt here) ...\n" \
"-----END CERTIFICATE-----\n";

// Device Private Key
const char* PRIVATE_KEY = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"... (Paste xxxxx-private.pem.key here) ...\n" \
"-----END RSA PRIVATE KEY-----\n";

#endif
