# ESP32-S3 Logitech G29 Shifter Adapter

A DIY USB HID adapter for the **Logitech G29 Shifter (Driving Force W-U0003)** built using an **ESP32-S3-WROOM-1**.  
The project features **web-based calibration**, **persistent gear mapping**, and **USB gamepad emulation** for PC and custom sim racing rigs.

---

## 🚗 Features

- USB HID Gamepad emulation (plug-and-play on PC)
- Web-based calibration interface (WiFi Access Point)
- Live gear detection and telemetry
- Configurable gear zones stored in flash
- Reverse gear priority logic
- Smoothed analog readings (jitter-free shifting)
- Optional DB9 (RS232-style) connector support
- Mobile-friendly modern web UI

---

## 🧰 Hardware Used

- **ESP32-S3-WROOM-1**
- **Logitech G29 Shifter (Driving Force W-U0003)**
- **DB9 RS232 D-SUB 9 Pin Male Connector**
- **5V Power Adapter**

---

## 🔌 Wiring Overview

| G29 Shifter  |  DB9 | ESP32-S3                             |       |
|------------  |------|--------------------------------------|-------|
| X Axis       |   4  |ADC Pin (X_AXIS_PIN)                  |   1   | 
| Y Axis       |   8  |ADC Pin (Y_AXIS_PIN)                  |   2   |
| Reverse      |   2  |Digital Input (REVERSE_PIN, pull-up)  |   3   |
| GND          |   6  |GND                                   |  GND  |
| 5V           |   3  |External 5V Supply                    |       |
| 5V           |   7  |External 5V Supply                    |       |

⚠️ **5V + is connected to DB9 pins 3 & 7 and ground/negative is connected to ESP GND.**

---

## 🌐 Web Calibration Interface

- ESP32 creates a WiFi AP named: `G29-Shifter`
- Password: `g29config`
- Open in browser: `http://192.168.4.1/`

### Web UI Features:
- Live X/Y axis readings
- Reverse switch status
- Detected gear display
- Editable gear zones
- Save settings to flash memory

---

## 🚀 Getting Started

1. Clone or download this repository
2. Open `firmware/esp32_g29_shifter.ino` in Arduino IDE
3. Select **ESP32-S3 board**
4. Flash the firmware
5. Power Shiter with 5V adapter
6. Connect to `G29-Shifter` WiFi
7. Calibrate gear positions in browser
8. Plug ESP32 into PC via USB

---

## 🎮 Usage

Each gear is mapped to a USB gamepad button:

| Gear | Button |
|------|--------|
| 1    | BTN_1 |
| 2    | BTN_2 |
| 3    | BTN_3 |
| 4    | BTN_4 |
| 5    | BTN_5 |
| 6    | BTN_6 |
| R    | BTN_REV |

Compatible with most PC racing simulators.

---

## 📸 Screenshots

*(Coming Soon)*

---

## 🛠️ Future Improvements

- 3D Printed casing
- Reset-to-defaults button in web UI
- Sequential mode support

---

## 📄 License

This project is released under the MIT License.
Feel free to use, modify, and share.

---

## 🙌 Credits

Developed using ESP32-S3 USB HID and Arduino framework.
Inspired by the sim racing and DIY hardware community.
