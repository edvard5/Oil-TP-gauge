
# Oil TP Gauge

A robust and minimalistic **oil pressure + temperature gauge** for automotive or motorsport use. Built on a **Waveshare RP2040-Zero**, it uses a **Bosch Motorsport sensor** and a **0.96" ST7735S TFT screen** for real-time display.

---

## 🚗 Features

- 📊 Real-time **oil pressure** and **temperature** readings from a **single Bosch sensor**
- 🖼️ Color UI on a **0.96” SPI TFT (ST7735S)** display
- 🧠 Runs on **RP2040-Zero** with clean, Arduino-based firmware
- 🧩 Fully customizable UI: images, fonts, colors
- 🔌 Designed for **automotive 5V systems**, with proper analog scaling

---

## 🧰 Hardware Overview

| Component         | Description |
|------------------|-------------|
| **MCU**          | [Waveshare RP2040-Zero](https://www.waveshare.com/wiki/RP2040-Zero) |
| **Display**      | [0.96" SPI TFT LCD with ST7735S driver](https://a.aliexpress.com/_Exmp90m) |
| **Sensor**       | [Bosch Motorsport 10 bar](https://www.bosch-motorsport.com/content/downloads/Raceparts/en-GB/54249355.html) – **combined oil pressure and NTC temperature** |
| **Power Supply** | 5V input (USB or regulator)

---

## 🔌 Pinout (RP2040-Zero Connections)

### 📺 TFT Display (ST7735S, SPI)

| TFT Pin | RP2040 GPIO | Function        |
|---------|-------------|-----------------|
| **BLK** | GPIO 14     | Backlight (optional) |
| **CS**  | GPIO 15     | Chip Select     |
| **DC**  | GPIO 26     | Data/Command    |
| **RES** | GPIO 27     | Reset           |
| **SDA** | GPIO 10     | SPI MOSI        |
| **SCL** | GPIO 11     | SPI Clock       |
| **VCC** | 3.3V        | Power           |
| **GND** | GND         | Ground          |

🛠 Fully configured in the included `TFT_eSPI` folder with custom `User_Setup.h`.

---

### 🧪 Bosch Sensor (Temp + Pressure, 5V analog)

| Signal         | RP2040 GPIO | Notes |
|----------------|-------------|-------|
| **Temperature** | GPIO 28 (ADC2) | NTC analog — use **4.6kΩ pull-up to 5V** |
| **Pressure**    | GPIO 29 (ADC3) | 0–5V analog — use **voltage divider** to protect ADC (max 3.3V) |

### ⚡ Voltage Conditioning

- **Pressure Voltage Divider Example**  
  - R1 (top resistor): 9750Ω (from sensor output to ADC pin)  
  - R2 (bottom resistor): 4560Ω (from ADC pin to GND)  
  - This scales 0–5V sensor output down to approximately 0–3.3V safe for the RP2040 ADC

- **NTC Pull-up Resistor**  
  - 4.6kΩ **to 5V** (not 3.3V) to match Bosch sensor specs

---

## 🛠 Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software)
- [Arduino-Pico Core](https://github.com/earlephilhower/arduino-pico)
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) (already preconfigured in this repo)

📦 The repo includes a **preconfigured `TFT_eSPI/` folder** with `User_Setup.h` tailored for:
- **ST7735S driver**
- Your exact **GPIO assignments**
- **160×80** or **128×160** resolution (adjustable)

---

## 📦 Installation Steps

```bash
git clone https://github.com/edvard5/oil-TP-Gauge.git
```

1. Open `Oil_TP-gauge.ino` in Arduino IDE  
2. Select board: **Raspberry Pi Pico / RP2040**  
3. Ensure the included `TFT_eSPI` folder is used in the library path  
4. Upload to your RP2040-Zero

---

## 📁 File Structure

```
oil-TP-Gauge/
├── Oil_TP-gauge.ino       # Main sketch
├── bg.h                   # Background graphic
├── manifold.h             # Overlay gauge image
├── visitor1.ttf           # Custom UI font
└── include/
    └── TFT_eSPI/          # Preconfigured TFT library (ST7735S + GPIOs)
```

---

## 🧩 Customization

- 🖼 Replace `bg.h` and `manifold.h` to change UI visuals
- 🔤 Swap `visitor1.ttf` with any TTF font
- 📐 Adjust temperature/pressure scaling in code based on your setup

---

## 📄 License

MIT License — see [LICENSE](LICENSE)

---

## 🙏 Credits

- [TFT_eSPI by Bodmer](https://github.com/Bodmer/TFT_eSPI)  

