# 👕 Smart Jacket Monitoring System (IoT with ESP32 + Node.js)

A real-time **wearable safety solution** built using the **ESP32 microcontroller** and an integrated web dashboard. It monitors health and environmental parameters, supports RFID-based session tracking, Bluetooth connectivity checks, and emergency alerts—ideal for hazardous environments like **mines**, **construction sites**, or **industrial zones**.

---

## 🚀 Features

- 🎫 **RFID-based Session Authentication**
  - Starts/stops sessions based on tag detection.
  - Displays messages like `Hello, Kunal` or `Goodbye, Kunal | Duration: ...`.

- 📡 **Live Sensor Monitoring via Web Dashboard**
  - 🌡️ Temperature (DHT11)
  - 💓 Pulse Sensor
  - 🔥 Gas Detection (MQ2)
  - 💡 Light Intensity (LDR)
  - 🔗 Bluetooth Connection Status

- 🔔 **Smart Alerts**
  - Alerts for high temperature, gas levels, high pulse, and Bluetooth disconnection.
  - Visual (LED), sound (buzzer), and vibration (motor) alerts.

- 🚨 **Panic Mode**
  - Emergency button triggers an alert tone and vibration.

- 📖 **Session Logging**
  - Each session is saved with user name, start time, end time, and duration to `logs.json`.

---


---

## 🌐 Web Interface

### ✅ `index.html` - Real-time Dashboard

Live sensor values and smart alerts are displayed in real time using Socket.IO.

- Temperature, Pulse, Gas, and Bluetooth status
- Smart alerts based on thresholds
- Live status messages (e.g., session start/end)
- Navigation to session logs

> ![Dashboard Preview](assets/Screenshot%202025-06-26%20104739.png)

---

### 📚 `log.html` - Session Log Viewer

Fetches and displays all saved session logs from `logs.json`.

| Name   | Session Start | Session End | Duration |
|--------|----------------|--------------|----------|
| Kunal  | 12:10:05 PM     | 12:12:47 PM  | 162 sec  |

> ![Log Preview](assets/Screenshot%202025-06-26%20104818.png)

---

## 🔌 How to Run

### 1. Upload Arduino Code
- Flash `smart_jacket.ino` to your ESP32 board
- Make sure all sensors are connected as per defined pins

### 2. Start Node.js Server
```markdown
```bash
npm install
node server.js
---

🧠 Thresholds Used
Parameter	Threshold	Alert Triggered If
Temperature	> 37°C	High temp alert
Pulse	> 120 BPM	High pulse alert
Gas Level	> 1000	Dangerous gas level alert
BT Connected	0 (false)	Bluetooth disconnected alert
---

🧰 Tech Stack
ESP32 (Arduino IDE)
Node.js (Express + Socket.IO + SerialPort)
HTML/CSS/JS (Frontend UI)
RFID MFRC522
Sensors: DHT11, MQ2, Pulse Sensor, LDR
LCD 16x2 I2C, Buzzer, Motor, Bluetooth
---
👨‍💻 Developed By
Kunal Raj.S
📍 BMS College of Engineering


