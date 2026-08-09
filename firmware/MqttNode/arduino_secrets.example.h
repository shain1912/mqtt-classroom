// Copy this file to `arduino_secrets.h` and fill in your own values.
// `arduino_secrets.h` is gitignored so credentials never reach the repository.

#pragma once

#define WIFI_SSID   "YOUR_WIFI_SSID"      // 2.4GHz only - the ESP32C6 has no 5GHz radio
#define WIFI_PASS   "YOUR_WIFI_PASSWORD"

#define MQTT_HOST   "192.168.0.49"        // LAN IP of the machine running Mosquitto
#define MQTT_PORT   1883

// Your board's name on the broker: topics become classroom/<name>/...
// Leave it empty ("") to fall back to "c6-" plus the last 3 bytes of the MAC.
// Pick something unique in the class - two boards sharing a name will fight
// over the same topics and keep kicking each other off the broker.
#define DEVICE_NAME "your-name"
