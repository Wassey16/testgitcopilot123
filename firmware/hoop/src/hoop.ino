/**************************************************************************
 *  HOOP MODULE – ESP32 (BLE Version with Timestamps)                     *
 *  Sensor: IR beam-break (active low)                                    *
 *  Publishes a JSON event when the ball passes through the hoop          *
 *  Also sends periodic status JSON to show it's alive                    *
 **************************************************************************/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

const uint8_t IR_PIN = 2;    // D2 on ESP32 = GPIO2
bool lastState = HIGH;
uint32_t hoopEventCount = 0; // Track the number of hoop events

// BLE definitions
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID           "beb5483e-36e1-4688-b7f5-ea07361b26a8"

unsigned long lastStatus = 0;
const unsigned long STATUS_PERIOD = 2000;  // Send status every 2 seconds

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void setup()
{
    Serial.begin(115200);
    pinMode(IR_PIN, INPUT_PULLUP);

    // Setup BLE
    BLEDevice::init("ESP32_HOOP");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    BLEDevice::startAdvertising();
    Serial.println("BLE advertising started (ESP32_HOOP)");
}

void sendBLE(const char* json) {
    if (deviceConnected) {
        pCharacteristic->setValue((uint8_t*)json, strlen(json));
        pCharacteristic->notify();
        Serial.print("BLE notify: ");
        Serial.println(json);
    }
}

void loop()
{
    unsigned long now = millis();

    // Send periodic status (heartbeat)
    if (now - lastStatus > STATUS_PERIOD) {
        lastStatus = now;
        char payload[128];
        snprintf(payload, sizeof(payload),
                 "{\"event\":\"status\",\"ts\":%lu,\"ir_state\":%d,\"hoop_count\":%u}",
                 now, lastState, hoopEventCount);
        sendBLE(payload);
    }

    // Detect hoop event (beam break)
    bool state = digitalRead(IR_PIN);
    if (state != lastState)
    {
        lastState = state;
        Serial.print("IR state: ");
        Serial.println(state);
        if (state == LOW)  // Beam broken
        {
            hoopEventCount++; // Increment hoop count
            char payload[128];
            snprintf(payload, sizeof(payload),
                     "{\"event\":\"hoop\",\"ts\":%lu,\"score\":1,\"hoop_count\":%u}",
                     millis(), hoopEventCount);
            sendBLE(payload);
            Serial.println("Hoop event sent!");
        }
    }
}