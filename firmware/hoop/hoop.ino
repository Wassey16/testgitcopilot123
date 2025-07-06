/**************************************************************************
 *  HOOP MODULE – ESP32                                                  *
 *  Sensor: IR beam-break                                                *
 *  Publishes a single JSON event when the ball passes through the hoop  *
 **************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "../shared/config.h"

const uint8_t IR_PIN = 14;       // Digital input from IR receiver (LOW = beam broken)
bool lastState = HIGH;

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

void connectWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) delay(500);
}
void connectMQTT()
{
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    while (!mqtt.connected())
    {
        mqtt.connect("hoop-module", MQTT_USER, MQTT_PASSWORD,
                     LWT_TOPIC, 1, true, LWT_MESSAGE);
    }
}
void setup()
{
    Serial.begin(115200);
    pinMode(IR_PIN, INPUT_PULLUP);

    connectWiFi();
    connectMQTT();
}
void loop()
{
    if (!mqtt.connected()) connectMQTT();
    mqtt.loop();

    bool state = digitalRead(IR_PIN);
    if (state != lastState)
    {
        lastState = state;
        if (state == LOW)  // beam broken
        {
            char payload[64];
            snprintf(payload, sizeof(payload),
                     "{\"ts\":%lu,\"score\":1}", millis());
            mqtt.publish(TOPIC_HOOP_EVENT, payload);
        }
    }
}
