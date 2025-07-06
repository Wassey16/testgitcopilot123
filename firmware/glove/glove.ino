/**************************************************************************
 *  GLOVE MODULE – Adafruit Feather M0 WiFi                               *
 *  Sensors: MPU-6050 + 3x FSRs                                           *
 *  Publishes JSON packets to MQTT                                        *
 **************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <WiFi101.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Adafruit_MPU6050.h"
#include "Adafruit_Sensor.h"
#include "../shared/config.h"

// ---------- Pinout ----------
const uint8_t FSR_PIN_1 = A0;      // index
const uint8_t FSR_PIN_2 = A1;      // middle
const uint8_t FSR_PIN_3 = A2;      // ring

// ---------- Globals ----------
Adafruit_MPU6050 mpu;
WiFiClient      wifiClient;
PubSubClient    mqtt(wifiClient);

unsigned long lastPacket = 0;
const uint16_t SAMPLE_PERIOD_MS = 20;    // 50 Hz
const uint16_t SEND_PERIOD_MS   = 20;

float ax, ay, az, gx, gy, gz;
uint16_t fsr1, fsr2, fsr3;
uint16_t fsr_sum        = 0;
uint16_t fsr_sum_prev   = 0;
bool     released       = false;

// ---------- Helpers ----------
void connectWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print(F("Connecting WiFi"));
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(F("OK"));
}

void connectMQTT()
{
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    while (!mqtt.connected())
    {
        Serial.print(F("Connecting MQTT..."));
        if (mqtt.connect("glove-module", MQTT_USER, MQTT_PASSWORD,
                         LWT_TOPIC, 1, true, LWT_MESSAGE))
        {
            Serial.println(F("OK"));
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqtt.state());
            Serial.println(" retry");
            delay(1000);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(FSR_PIN_1, INPUT);
    pinMode(FSR_PIN_2, INPUT);
    pinMode(FSR_PIN_3, INPUT);

    Wire.begin();
    if (!mpu.begin())
    {
        Serial.println(F("Failed to find MPU6050 chip"));
        while (1)
            delay(10);
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    connectWiFi();
    connectMQTT();
}

void loop()
{
    if (!mqtt.connected())
        connectMQTT();
    mqtt.loop();

    unsigned long now = millis();
    if (now - lastPacket >= SAMPLE_PERIOD_MS)
    {
        lastPacket = now;

        /* === Read sensors === */
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        ax = a.acceleration.x;
        ay = a.acceleration.y;
        az = a.acceleration.z;
        gx = g.gyro.x;
        gy = g.gyro.y;
        gz = g.gyro.z;

        fsr1 = analogRead(FSR_PIN_1);
        fsr2 = analogRead(FSR_PIN_2);
        fsr3 = analogRead(FSR_PIN_3);
        fsr_sum_prev = fsr_sum;
        fsr_sum      = fsr1 + fsr2 + fsr3;

        /* === Event detection === */
        /* Released if summed FSR drops by >30% of previous value */
        released = false;
        if (fsr_sum_prev > 50 && fsr_sum_prev * 0.7 > fsr_sum)
            released = true;

        /* === Build JSON as string (hand-rolled to save RAM) === */
        char payload[256];
        snprintf(payload, sizeof(payload),
                 "{\"ts\":%lu,"
                 "\"fsr\":[%u,%u,%u],"
                 "\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,"
                 "\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f,"
                 "\"rel\":%d}",
                 millis(), fsr1, fsr2, fsr3,
                 ax, ay, az, gx, gy, gz,
                 released ? 1 : 0);

        mqtt.publish(TOPIC_GLOVE_RAW, payload);
    }
}