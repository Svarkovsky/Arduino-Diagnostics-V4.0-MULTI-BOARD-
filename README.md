# Arduino Diagnostics V4.0 [MULTI-BOARD]

> **Professional No-Wires Self-Test Tool for Arduino Boards**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Arduino IDE](https://img.shields.io/badge/Arduino%20IDE-1.8%2B-blue.svg)](https://www.arduino.cc/en/software)
[![Boards](https://img.shields.io/badge/Boards-UNO%7CNano%7CMega-green.svg)]()

A comprehensive diagnostic suite that tests all major subsystems of Arduino boards **without requiring any external wires or components**. Only a USB cable is needed.

---

## 🚀 Features

| Feature | Description |
|---------|-------------|
| 🔍 **Automatic Board Detection** | Supports UNO, Nano, and Mega 2560 with automatic MCU identification |
| 💡 **LED PWM Test** | Visual confirmation of Timer0 PWM functionality |
| 💾 **EEPROM Multi-Sector Test** | 16 sub-tests (4 sectors × 4 patterns) with write timing analysis |
| ⏱️ **CPU Clock Accuracy** | Measures crystal/resonator precision in microseconds |
| 🔔 **Timer1 Interrupt Test** | Verifies interrupt system functionality |
| 📌 **Digital Pins Check** | Detects shorts to VCC/GND on all digital pins |
| 🔌 **Pull-up Resistors Test** | Tests internal ~20-50kΩ pull-ups on all digital pins |
| 📊 **Analog Noise Test** | Checks ADC health via floating pin noise analysis |
| 🎯 **ADC Multiplexer Stability** | VCC measurement via all analog channels |
| 📈 **Bandgap Reference Drift** | 10-second stability measurement of internal 1.1V reference |
| ⚡ **Brown-out Detection** | Checks MCUSR register for power-related resets |
| 🔋 **Power Stability Under Load** | Tests voltage drop under maximum pin load |
| 🔄 **Watchdog Reset Verification** | Confirms system recovery capability |
| 💾 **Results Persistence** | Stores results in EEPROM across reset |
| 📋 **Summary Table** | Professional pass/fail report with recommendations |

---

## 📋 Supported Boards

| Board | MCU | Digital Pins | Analog Pins | EEPROM | Test Time |
|-------|-----|--------------|-------------|--------|-----------|
| **Arduino UNO** | ATmega328P | D2-D12 (11) | A0-A5 (6) | 1 KB | ~50 seconds |
| **Arduino Nano** | ATmega328P | D2-D12 (11) | A0-A5 (6) | 1 KB | ~50 seconds |
| **Arduino Mega 2560** | ATmega2560 | D2-D23 (22) | A0-A15 (16) | 4 KB | ~70 seconds |

> ⚠️ **Note:** Other Arduino boards are not officially supported yet. Contributions welcome!

---

## 🛠️ Installation

### Option 1: Clone Repository
```bash
git clone https://github.com/Svarkovsky/Arduino-Diagnostics-V4.0-MULTI-BOARD-.git
cd Arduino-Diagnostics-V4.0-MULTI-BOARD-
