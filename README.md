# SwishSensei



## 🏀 Smart Basketball Coach – Computer Vision-Based Shot Form Analyzer

**Course:** IOT 
**Team Members:** Omar Sherbini, Abdul Wassey  

---

## 🔍 Project Overview

This project is a **computer vision-based coaching system** designed to help basketball players improve their shooting mechanics. Using a webcam and pose estimation algorithms, the system tracks the **player's upper body**, **jump**, and **wrist motion**, and provides **real-time corrective audio feedback** through Bluetooth headphones or speakers.

Optional sensors (e.g., MPU6050 IMU) may be integrated to support wrist analysis in case the CV system lacks reliability in certain conditions.

---

## 🧠 Key Features

- 📷 **Pose Detection:** Full upper-body motion tracking using computer vision
- 🖐️ **Wrist Flick Analysis:** Detects shooting form and wrist motion (via CV or fallback IMU)
- 🦘 **Jump Detection:** Recognizes jump timing and elevation for shot power estimation
- 🔊 **Real-Time Audio Feedback:** Sent via Bluetooth to headphones or speaker
- 🔁 **Sensor Integration (Optional):** Adds IMU data for more accurate wrist detection if needed

---

## 🏀 Use Cases

- Solo basketball training without a coach
- Continuous shot correction via voice instructions
- Helps identify mistakes like:
  - Poor wrist flick
  - No jump or late jump
  - Arm angle or shoulder misalignment
- CV system acts as a **virtual coach** guiding form improvement

---

## 🧰 Technology Stack

| Component         | Role                                        |
|------------------|---------------------------------------------|
| **OpenCV + MediaPipe** | Pose estimation and motion tracking     |
| **Python**        | Core programming language                   |
| **Bluetooth Audio** | Audio output for real-time feedback       |
| **MPU6050 (optional)** | Extra motion sensing for wrist accuracy |
| **ESP32 (optional)** | Microcontroller for sensor data (if used) |

---



