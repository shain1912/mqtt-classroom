// XIAO ESP32C6 MQTT node for the classroom broker.
//
//   subscribes  classroom/<id>/led/set      payload "on" | "off" | "toggle"
//   publishes   classroom/<id>/led/state    "on" | "off"            (retained)
//   publishes   classroom/<id>/sensor/a0    {"raw":2048,"mv":1650}  every 2s
//   publishes   classroom/<id>/status       "online" | "offline"    (retained, LWT)
//
// <id> is DEVICE_NAME from arduino_secrets.h. Leave that empty and the board
// falls back to "c6-" plus the last 3 bytes of its MAC, so an unconfigured
// board still gets a distinct topic tree.

#include <WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"

#define LED_PIN 15                  // XIAO user LED, active LOW
#define A0_PIN  A0                  // D0 / GPIO0

const unsigned long PUBLISH_INTERVAL_MS = 2000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String deviceId;
String topicLedSet, topicLedState, topicSensor, topicStatus;
unsigned long lastPublish = 0;

// ---------------------------------------------------------------- helpers

void setLed(bool on) {
  digitalWrite(LED_PIN, on ? LOW : HIGH);
  mqtt.publish(topicLedState.c_str(), on ? "on" : "off", true);  // retained
  Serial.printf("LED -> %s\n", on ? "ON" : "OFF");
}

bool ledIsOn() {
  return digitalRead(LED_PIN) == LOW;
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  msg.toLowerCase();

  Serial.printf("RX %s = %s\n", topic, msg.c_str());

  if (String(topic) != topicLedSet) return;

  if (msg == "on" || msg == "1" || msg == "true") {
    setLed(true);
  } else if (msg == "off" || msg == "0" || msg == "false") {
    setLed(false);
  } else if (msg == "toggle") {
    setLed(!ledIsOn());
  } else {
    Serial.printf("ignored payload: '%s'\n", msg.c_str());
  }
}

void connectWiFi() {
  Serial.printf("WiFi: connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\nWiFi: connected, IP %s, RSSI %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.printf("MQTT: connecting to %s:%d as %s ... ",
                  MQTT_HOST, MQTT_PORT, deviceId.c_str());

    // Last will: the broker publishes "offline" if this board drops off.
    bool ok = mqtt.connect(deviceId.c_str(),
                           nullptr, nullptr,
                           topicStatus.c_str(), 0, true, "offline");

    if (ok) {
      Serial.println("connected");
      mqtt.publish(topicStatus.c_str(), "online", true);
      mqtt.subscribe(topicLedSet.c_str());
      Serial.printf("subscribed to %s\n", topicLedSet.c_str());
      setLed(ledIsOn());  // republish current state so late subscribers see it
    } else {
      Serial.printf("failed rc=%d, retrying in 3s\n", mqtt.state());
      delay(3000);
    }
  }
}

void publishSensor() {
  int raw = analogRead(A0_PIN);
  int mv  = analogReadMilliVolts(A0_PIN);

  char payload[64];
  snprintf(payload, sizeof(payload), "{\"raw\":%d,\"mv\":%d}", raw, mv);

  mqtt.publish(topicSensor.c_str(), payload);
  Serial.printf("TX %s = %s\n", topicSensor.c_str(), payload);
}

// ---------------------------------------------------------------- sketch

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);   // start off
  pinMode(A0_PIN, INPUT);

  delay(500);

  deviceId = DEVICE_NAME;
  if (deviceId.length() == 0) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char idBuf[16];
    snprintf(idBuf, sizeof(idBuf), "c6-%02x%02x%02x", mac[3], mac[4], mac[5]);
    deviceId = idBuf;
  }

  String base   = "classroom/" + deviceId;
  topicLedSet   = base + "/led/set";
  topicLedState = base + "/led/state";
  topicSensor   = base + "/sensor/a0";
  topicStatus   = base + "/status";

  Serial.printf("\n=== XIAO ESP32C6 MQTT node ===\ndevice id: %s\n", deviceId.c_str());
  Serial.printf("led set:   %s\n", topicLedSet.c_str());
  Serial.printf("sensor:    %s\n", topicSensor.c_str());

  connectWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  connectMqtt();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;
    publishSensor();
  }
}
