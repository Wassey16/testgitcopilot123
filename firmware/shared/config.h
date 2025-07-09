/******************************
 *  Shared WiFi / MQTT config *
 ******************************/
/*#pragma once
#define LWT_TOPIC   "foot/status"
#define LWT_MESSAGE "offline"
// ========== WIFI ==========
#define WIFI_SSID     "R9"
#define WIFI_PASSWORD "03334833840"

// ========== MQTT ==========
#define MQTT_BROKER     "192.168.0.101"
#define MQTT_PORT       1883
#define MQTT_USER       "mqttuser"
#define MQTT_PASSWORD   "mqttpass"

// Topics
#define TOPIC_GLOVE_RAW     "basket/glove/raw"
#define TOPIC_FOOT_RAW      "basket/foot/raw"
#define TOPIC_HOOP_EVENT    "basket/hoop/event"*/

#pragma once
#define WIFI_SSID     "R9"
#define WIFI_PASSWORD "03334833840"


/* Where the Flask/SocketIO backend can be reached (IP or mDNS)      */
#define HTTP_COLLECTOR "192.168.0.101"        // no protocol, just host
#define HTTP_PORT      8000                  // default Flask port
