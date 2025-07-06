/******************************
 *  Shared WiFi / MQTT config *
 ******************************/
#pragma once

// ========== WIFI ==========
#define WIFI_SSID     "Swishsensei"
#define WIFI_PASSWORD "flushflush"

// ========== MQTT ==========
#define MQTT_BROKER     "192.168.1.10"
#define MQTT_PORT       1883
#define MQTT_USER       "mqttuser"
#define MQTT_PASSWORD   "mqttpass"

// Topics
#define TOPIC_GLOVE_RAW     "basket/glove/raw"
#define TOPIC_FOOT_RAW      "basket/foot/raw"
#define TOPIC_HOOP_EVENT    "basket/hoop/event"
