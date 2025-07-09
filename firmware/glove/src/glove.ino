/**************************************************************************
 *  GLOVE MODULE – ESP32 lolin32_lite (BLE for Mac)                       *
 *  Sensors: MPU-6050 + 2x FSRs                                           *
 *  Publishes raw data and event-based JSON packets via BLE notifications *
 *  Captures finger pressure and wrist motion during ball pickup/release. *
 **************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Pin Definitions
#define FSR1_PIN 34  // FSR under finger 1
#define FSR2_PIN 35  // FSR under finger 2

MPU6050 mpu;

// FSR threshold (adjust as needed for your setup)
const int FSR_THRESHOLD = 2000; // Lower means more sensitive to release

// Ball event tracking
int maxFSR1 = 0;
int maxFSR2 = 0;
bool ballHeld = false;
unsigned long ballPickupTs = 0;
unsigned long ballReleaseTs = 0;

// Motion at release
int16_t ax, ay, az, gx, gy, gz;

// BLE definitions
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Sampling
unsigned long lastPacket = 0;
const uint16_t SAMPLE_PERIOD_MS = 10;  // 100 Hz

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Boot (GLOVE BLE)");

    Wire.begin(27, 26); // SDA = 27, SCL = 26
    mpu.initialize();
    Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    pinMode(FSR1_PIN, INPUT);
    pinMode(FSR2_PIN, INPUT);

    // Setup BLE
    BLEDevice::init("ESP32_GLOVE");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    BLEDevice::startAdvertising();
    Serial.println("BLE advertising started.");
}

void sendBLE(const char* json) {
    if (deviceConnected) {
        pCharacteristic->setValue((uint8_t*)json, strlen(json));
        pCharacteristic->notify();
        Serial.print("BLE notify: ");
        Serial.println(json);
    }
}

void loop() {
    unsigned long now = millis();
    if (now - lastPacket >= SAMPLE_PERIOD_MS) {
        lastPacket = now;

        int fsr1Value = analogRead(FSR1_PIN);
        int fsr2Value = analogRead(FSR2_PIN);

        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // Ball held logic (if any FSR pressed)
        if (fsr1Value > FSR_THRESHOLD || fsr2Value > FSR_THRESHOLD) {
            if (!ballHeld) {
                ballHeld = true;
                maxFSR1 = 0;
                maxFSR2 = 0;
                ballPickupTs = now;
                Serial.println("Ball picked up!");
            }
            if (fsr1Value > maxFSR1) maxFSR1 = fsr1Value;
            if (fsr2Value > maxFSR2) maxFSR2 = fsr2Value;
        }

        // Ball release event
        if (ballHeld && fsr1Value < FSR_THRESHOLD && fsr2Value < FSR_THRESHOLD) {
            ballReleaseTs = now;

            Serial.println("=== BALL RELEASED ===");
            Serial.print("Max FSR1: "); Serial.println(maxFSR1);
            Serial.print("Max FSR2: "); Serial.println(maxFSR2);

            Serial.println("Wrist motion at release:");
            Serial.print("Accel: X="); Serial.print(ax); Serial.print(" Y="); Serial.print(ay); Serial.print(" Z="); Serial.println(az);
            Serial.print("Gyro:  X="); Serial.print(gx); Serial.print(" Y="); Serial.print(gy); Serial.print(" Z="); Serial.println(gz);
            Serial.println("======================");

            // Build and send BLE JSON
            char payload[256];
            snprintf(payload, sizeof(payload),
                "{\"event\":\"release\",\"ts\":%lu,\"pickup\":%lu,\"maxFSR1\":%d,\"maxFSR2\":%d,"
                "\"ax\":%d,\"ay\":%d,\"az\":%d,\"gx\":%d,\"gy\":%d,\"gz\":%d}",
                ballReleaseTs, ballPickupTs, maxFSR1, maxFSR2,
                ax, ay, az, gx, gy, gz
            );
            sendBLE(payload);

            ballHeld = false; // Reset state
            delay(500); // Prevent multiple triggers
        } else {
            // Publish raw/live data for debugging
            char livePayload[256];
            snprintf(livePayload, sizeof(livePayload),
                "{\"event\":\"raw\",\"ts\":%lu,\"fsr1\":%d,\"fsr2\":%d,"
                "\"ax\":%d,\"ay\":%d,\"az\":%d,\"gx\":%d,\"gy\":%d,\"gz\":%d}",
                now, fsr1Value, fsr2Value, ax, ay, az, gx, gy, gz
            );
            sendBLE(livePayload);
        }
    }
}