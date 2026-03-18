# ⛏️ Smart Mining Monitoring System

> A dual-microcontroller IoT system (Arduino + ESP32) for real-time environmental and seismic monitoring inside mining environments. Sensor data is logged to an SD card and uploaded live to Firebase Firestore, viewable on a mobile app.

---

## 📋 Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Features](#features)
- [Hardware Components](#hardware-components)
- [Pin Configuration](#pin-configuration)
- [Installation & Upload](#installation--upload)
- [Required Libraries](#required-libraries)
- [Firebase Setup](#firebase-setup)
- [Data Format](#data-format)
- [Project Structure](#project-structure)
- [License](#license)

---

## Overview

Mining environments are hazardous — toxic gases, dust, fires, and earthquakes are all constant risks. This system uses an **Arduino Uno** to collect environmental sensor data and an **ESP32** to handle Wi-Fi connectivity, earthquake detection via accelerometer, SD card logging, and real-time cloud sync via **Firebase Firestore**.

All data is timestamped using an NTP server and saved as a CSV file on an SD card as a local backup, while simultaneously being uploaded to the cloud for remote monitoring via a mobile application.

---

## System Architecture

```
┌─────────────────────────┐         ┌──────────────────────────────┐
│      Arduino Uno        │         │           ESP32               │
│                         │         │                               │
│  DHT22 (Temp/Humidity)  │─Serial─▶│  ADXL345 Accelerometer       │
│  GP2Y1014 (Dust)        │         │  SD Card (CSV logging)        │
│  MQ135 (Air quality)    │         │  Wi-Fi → Firebase Firestore   │
│  MQ2 (Smoke/Gas)        │         │  NTP Timestamp               │
│  Flame Sensor           │         │  Earthquake Detection         │
└─────────────────────────┘         └──────────────┬───────────────┘
                                                   │
                                    ┌──────────────▼───────────────┐
                                    │        Firebase Firestore     │
                                    │        (Cloud Database)       │
                                    └──────────────┬───────────────┘
                                                   │
                                    ┌──────────────▼───────────────┐
                                    │        Mobile Application     │
                                    │        (Real-time Dashboard)  │
                                    └──────────────────────────────┘
```

---

## Features

### 🌡️ Environmental Monitoring (Arduino)
- **Temperature & Humidity** via DHT22 sensor
- **Dust density** via GP2Y1014 optical sensor (mg/m³)
- **Air quality** via MQ135 sensor (CO₂, NH₃, benzene)
- **Smoke & gas** via MQ2 sensor (LPG, methane, smoke)
- **Flame detection** via digital flame sensor

### 🌍 Seismic & Motion Monitoring (ESP32)
- **Earthquake detection** via ADXL345 accelerometer (I2C)
- Calculates pitch, roll, and absolute pitch change
- Three-level earthquake status: `0` = None, `1` = Potential, `2` = Active

### ☁️ Cloud & Storage
- **Firebase Firestore** upload every 5 seconds with full timestamp
- **SD card CSV logging** as local backup (`/Downloads/sensor_data.csv`)
- **NTP time sync** for accurate timestamps (UTC)

### 📱 Mobile App
- Real-time dashboard via Firebase (mobile app code managed separately)

---

## Hardware Components

| Component           | Quantity | Purpose                            |
|---------------------|----------|------------------------------------|
| Arduino Uno         | 1        | Environmental sensor controller    |
| ESP32               | 1        | Wi-Fi, cloud, accelerometer, SD    |
| DHT22               | 1        | Temperature & humidity             |
| GP2Y1014AU0F        | 1        | Dust density measurement           |
| MQ135               | 1        | Air quality (gases)                |
| MQ2                 | 1        | Smoke & combustible gas            |
| Flame Sensor        | 1        | Fire detection                     |
| ADXL345             | 1        | 3-axis accelerometer (earthquake)  |
| SD Card Module      | 1        | Local CSV data logging             |
| SD Card             | 1        | Storage media                      |

---

## Pin Configuration

### Arduino Uno

| Pin  | Component            |
|------|----------------------|
| D2   | DHT22 data pin       |
| D3   | Flame sensor         |
| D12  | GP2Y1014 LED control |
| A0   | GP2Y1014 analog out  |
| A1   | MQ2 analog out       |
| A2   | MQ135 analog out     |
| TX   | Serial → ESP32 RX    |

### ESP32

| Pin      | Component                  |
|----------|----------------------------|
| GPIO 16  | RX2 (from Arduino TX)      |
| GPIO 17  | TX2                        |
| GPIO 21  | SDA (ADXL345)              |
| GPIO 22  | SCL (ADXL345)              |
| GPIO 5   | SD Card CS                 |

---

## Installation & Upload

### Arduino Uno
1. Open `src/Arduino_smart_mining_system.ino` in Arduino IDE
2. Select **Board:** Arduino Uno
3. Install required libraries (see below)
4. Upload ✓

### ESP32
1. Open `src/Esp_smart_mining_system.ino` in Arduino IDE
2. Install **ESP32 board package** via Board Manager
3. Select **Board:** ESP32 Dev Module
4. Update Wi-Fi credentials and Firebase keys (see [Firebase Setup](#firebase-setup))
5. Upload ✓

> ⚠️ Upload Arduino first, then ESP32. Connect Arduino TX → ESP32 GPIO16 (RX2).

---

## Required Libraries

Install via **Arduino IDE → Sketch → Include Library → Manage Libraries**:

| Library                  | Install Name              | Used In  |
|--------------------------|---------------------------|----------|
| DHT sensor library       | `DHT sensor library`      | Arduino  |
| Adafruit Unified Sensor  | `Adafruit Unified Sensor` | Arduino  |
| Firebase ESP Client      | `Firebase ESP Client`     | ESP32    |
| NTPClient                | `NTPClient`               | ESP32    |
| SD                       | Built-in                  | ESP32    |
| Wire                     | Built-in                  | ESP32    |

---

## Firebase Setup

In `Esp_smart_mining_system.ino`, update these lines with your own credentials:

```cpp
#define WIFI_SSID       "your_wifi_name"
#define WIFI_PASSWORD   "your_wifi_password"
#define API_KEY         "your_firebase_api_key"
#define FIREBASE_PROJECT_ID "your_project_id"

auth.user.email    = "your@email.com";
auth.user.password = "yourpassword";
```

> ⚠️ Never share your real API keys publicly. Consider using environment variables or a `secrets.h` file that is listed in `.gitignore`.

---

## Data Format

Data is saved to `/Downloads/sensor_data.csv` on the SD card with these columns:

| Column               | Description                        |
|----------------------|------------------------------------|
| Timestamp            | YYYY-MM-DD-HH-MM-SS (NTP)          |
| Temperature          | °C (DHT22)                         |
| Humidity             | % (DHT22)                          |
| DustDensity          | mg/m³ (GP2Y1014)                   |
| MQ135                | Raw ADC value (air quality)        |
| MQ2                  | Raw ADC value (smoke/gas)          |
| FlameDetected        | 0 = No, 1 = Yes                    |
| X_out / Y_out / Z_out| Accelerometer axes (g)             |
| Pitch / Roll         | Orientation angles (degrees)       |
| PitchChangeAbsolute  | Change in pitch between readings   |
| EarthquakeStatus     | 0 = None, 1 = Potential, 2 = Active|

---

## Project Structure

```
smart-mining-system/
│
├── src/
│   ├── Arduino_smart_mining_system.ino   # Arduino sensor collection code
│   └── Esp_smart_mining_system.ino       # ESP32 cloud/SD/accelerometer code
│
├── schematics/
│   └── circuit_diagram.png               # Add your wiring diagram here
│
├── docs/
│   └── poster.pptx                       # Project research poster
│
├── libraries/                            # Custom libraries if any
│
├── .gitignore
├── LICENSE
└── README.md
```

---

## License

This project is licensed under the [MIT License](LICENSE).
