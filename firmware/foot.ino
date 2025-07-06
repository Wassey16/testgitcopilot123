/**************************************************************************
 *  FOOT MODULE – ESP32 lolin32_lite                                      *
 *  Sensors: MPU-6050 + 2x FSRs                                           *
 *  Publishes JSON packets to MQTT                                        *
 *  Also performs light preprocessing (jump start / apex detection).      *
 **************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Adafruit_MPU6050.h"
#include "Adafruit_Sensor.h"
#include "../shared/config.h"

// ---------- Pinout ----------
const uint8_t FSR_LEFT  = 34;      // any ADC-capable pins
const uint8_t FSR_RIGHT = 35;

// ---------- Jump state ----------
bool     inJump      = false;
unsigned long jumpStartTs = 0;
unsigned long apexTs      = 0;
#define GRAVITY 9.81

// ---------- Globals ----------
Adafruit_MPU6050 mpu;
WiFiClient      wifiClient;
PubSubClient    mqtt(wifiClient);

unsigned long lastPacket = 0;
const uint16_t SAMPLE_PERIOD_MS = 10;  // 100 Hz

float az;
uint16_t fl, fr;

void connectWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500); Serial.print(".");
    }
    Serial.println("OK");
}
void connectMQTT()
{
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    while (!mqtt.connected())
    {
        if (mqtt.connect("foot-module", MQTT_USER, MQTT_PASSWORD,
                         LWT_TOPIC, 1, true, LWT_MESSAGE))
        {
            Serial.println("MQTT OK");
        }
        else
        {
            Serial.print("failed rc=");
            Serial.print(mqtt.state());
            delay(1000);
        }
    }
}
void setup()
{
    Serial.begin(115200);
    pinMode(FSR_LEFT,  INPUT);
    pinMode(FSR_RIGHT, INPUT);

    Wire.begin();
    if (!mpu.begin())
    {
        Serial.println("MPU fail!"); while (1) delay(10);
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    connectWiFi();
    connectMQTT();
}

void loop()
{
    if (!mqtt.connected()) connectMQTT();
    mqtt.loop();

    unsigned long now = millis();
    if (now - lastPacket >= SAMPLE_PERIOD_MS)
    {
        lastPacket = now;

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        az = a.acceleration.z;     // positive up (device flat shoe-sole)
        fl = analogRead(FSR_LEFT);
        fr = analogRead(FSR_RIGHT);
        uint16_t fsum = fl + fr;

        /* ==== jump detection ==== */
        if (!inJump)
        {
            // Jump start if FSR unloads >40 % or az spike > 2g upwards
            if (fsum < 200 || az > (1.5 * GRAVITY))
            {
                inJump = true;
                jumpStartTs = now;
            }
        }
        else
        {
            // Detect apex when az crosses from >0 to <0 m/s² around 0 ± 0.5
            static bool upward = true;
            if (az < 0) upward = false;         // descending
            if (!upward && abs(az) < 1.0)       // near weightless
            {
                apexTs = now;
            }
            // Landing if FSR reloads or strong 'impact' az
            if (fsum > 400 || az < -3 * GRAVITY)
            {
                inJump = false;
            }
        }

        /* ===== send raw ===== */
        char payload[256];
        snprintf(payload, sizeof(payload),
                 "{\"ts\":%lu,\"fl\":%u,\"fr\":%u,\"az\":%.2f,"
                 "\"jump\":%d,\"jStart\":%lu,\"apex\":%lu}",
                 now, fl, fr, az, inJump ? 1 : 0,
                 jumpStartTs, apexTs);
        mqtt.publish(TOPIC_FOOT_RAW, payload);
    }
}
