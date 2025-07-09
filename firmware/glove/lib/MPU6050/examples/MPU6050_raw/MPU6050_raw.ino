#include "I2Cdev.h"
#include "MPU6050.h"
#include <Wire.h>

// ESP32 custom I2C pins
#define I2C_SDA 23
#define I2C_SCL 19

MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

#define LED_PIN 13
bool blinkState = false;

void setup() {
    // Use custom I2C pins
    Wire.begin(I2C_SDA, I2C_SCL);

    Serial.begin(115200);
    while(!Serial); // On some ESP32s, wait for serial port

    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    // Read raw accel/gyro measurements
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    Serial.print("a/g:\t");
    Serial.print(ax); Serial.print("\t");
    Serial.print(ay); Serial.print("\t");
    Serial.print(az); Serial.print("\t");
    Serial.print(gx); Serial.print("\t");
    Serial.print(gy); Serial.print("\t");
    Serial.println(gz);

    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
    delay(100);
}