/**************************************************************************
 *  FOOT MODULE – ESP32 lolin32_lite                                      *
 *  Sensors: MPU-6050 + 2x FSRs                                           *
 *  Publishes raw sensor data via BLE notifications                       *
 **************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Pin Definitions
#define FSR_LEFT_PIN 34  // FSR under left foot
#define FSR_RIGHT_PIN 35 // FSR under right foot

MPU6050 mpu;

// BLE definitions
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Sampling
unsigned long lastPacket = 0;
const uint16_t SAMPLE_PERIOD_MS = 100;  // 10 Hz

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Boot (FOOT BLE)");

    Wire.begin(27, 26); // SDA = 27, SCL = 26
    mpu.initialize();
    Serial.println(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    pinMode(FSR_LEFT_PIN, INPUT);
    pinMode(FSR_RIGHT_PIN, INPUT);

    // Setup BLE
    BLEDevice::init("ESP32_FOOT");
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

        // Read FSR values
        int fsrLeftValue = analogRead(FSR_LEFT_PIN);
        int fsrRightValue = analogRead(FSR_RIGHT_PIN);

        // Read MPU6050 values
        int16_t ax, ay, az, gx, gy, gz;
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // Build and send BLE JSON
        char payload[256];
        snprintf(payload, sizeof(payload),
            "{\"event\":\"foot_raw\",\"ts\":%lu,\"fl\":%d,\"fr\":%d,\"ax\":%d,\"ay\":%d,\"az\":%d,\"gx\":%d,\"gy\":%d,\"gz\":%d}",
            now, fsrLeftValue, fsrRightValue, ax, ay, az, gx, gy, gz
        );
        sendBLE(payload);

        // Debugging output to Serial Monitor
        Serial.print("FSR Left: "); Serial.print(fsrLeftValue);
        Serial.print(", FSR Right: "); Serial.print(fsrRightValue);
        Serial.print(", AX: "); Serial.print(ax);
        Serial.print(", AY: "); Serial.print(ay);
        Serial.print(", AZ: "); Serial.print(az);
        Serial.print(", GX: "); Serial.print(gx);
        Serial.print(", GY: "); Serial.print(gy);
        Serial.print(", GZ: "); Serial.println(gz);
    }
}