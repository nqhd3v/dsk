# Desk Guardian — Wiring Reference

All connections for the ESP32-S3 MKE-K01 (N16R8) edge node.

**⚠ ALWAYS power off ESP (unplug USB) before wiring any new connection.**

**⚠ GPIO 35/36/37 are reserved for PSRAM — do NOT use.**

---

## Power Rails

| Rail | Source           | Feeds                                      |
| ---- | ---------------- | ------------------------------------------ |
| 3V3  | ESP on-board LDO | BH1750, BME680, BSS138 A-side, ILI9488 VCC |
| 5V   | ESP USB 5V pin   | ACD1200, LD2410C, BSS138 B-side            |
| GND  | Common           | **All modules share same GND**             |

---

## I2C Bus (shared, 2 devices)

| Signal | ESP GPIO | BH1750 | BME680 |
| ------ | -------- | ------ | ------ |
| SDA    | **8**    | SDA    | SDA    |
| SCL    | **9**    | SCL    | SCL    |
| VCC    | 3V3      | VCC    | VCC    |
| GND    | GND      | GND    | GND    |

**Notes:**

- Both modules include on-board pull-ups — no external resistors needed.
- BME680 SDO pin → tie to **VCC (3V3)** for address **0x77**. Tie to GND for 0x76.
- BH1750 ADDR pin → leave floating or GND for address **0x23**.
- Bus speed: 400 kHz (fast mode).

---

## UART1 — ACD1200 CO2 Sensor (via BSS138 level shifter)

### ACD1200 Sensor

| ACD1200 Pin | Connect to      | Notes                                    |
| ----------- | --------------- | ---------------------------------------- |
| VCC         | ESP **5V**      | Sensor needs 5V                          |
| GND         | ESP **GND**     |                                          |
| RX          | ESP **GPIO 17** | Direct wire — 3V3 into 5V-tolerant input |
| TX          | BSS138 **BSDA** | 5V output — **MUST go through shifter**  |
| SET (pin 5) | **GND**         | Tie to GND = UART mode                   |

### BSS138 Level Shifter Board

Your board labels: A-side (3.3V), B-side (5V).

| BSS138 Pin | Connect to           | Role                               |
| ---------- | -------------------- | ---------------------------------- |
| **AVCC**   | ESP **3V3**          | A-side reference voltage           |
| **AGND**   | ESP **GND**          |                                    |
| **ASDA**   | ESP **GPIO 18** (RX) | Shifted 3.3V output → safe for ESP |
| **BVCC**   | ESP **5V**           | B-side reference voltage           |
| **BGND**   | ESP **GND**          |                                    |
| **BSDA**   | ACD1200 **TX**       | 5V signal input from sensor        |
| ASCL       | —                    | Unused, leave empty                |
| BSCL       | —                    | Unused, leave empty                |

**Signal path:**

```
ACD1200 TX (5V) → BSDA → [BSS138 shifts 5V→3V3] → ASDA → ESP GPIO 18
ESP GPIO 17 (TX) ──────────────────────────────────→ ACD1200 RX (direct)
```

**UART config:** 1200 baud, 8N1. Protocol: Aosong custom (NOT Modbus).

---

## UART2 — LD2410C Radar

| LD2410C Pin | ESP GPIO             | Notes                                    |
| ----------- | -------------------- | ---------------------------------------- |
| VCC         | **5V**               |                                          |
| GND         | **GND**              |                                          |
| RX          | **GPIO 15** (ESP TX) |                                          |
| TX          | **GPIO 16** (ESP RX) | Radar TX is 3.3V — direct wire OK        |
| OUT         | GPIO 4 (optional)    | Digital presence pin, unused in firmware |

**UART config:** 256000 baud, 8N1.

**Notes:**

- No level shifter needed — LD2410C TX is already 3.3V.
- Must use Serial2 in firmware (`Serial2.begin(256000, SERIAL_8N1, 16, 15)`).

---

## SPI — ILI9488 TFT Display (480×320)

| ILI9488 Pin | ESP GPIO | Notes                          |
| ----------- | -------- | ------------------------------ |
| SCK         | **12**   | HSPI clock                     |
| MOSI / SDI  | **11**   | Data to display                |
| MISO / SDO  | **13**   | Optional (read-back)           |
| CS          | **21**   | Display chip select            |
| DC / RS     | **10**   | Data/command select            |
| RST         | **47**   | Reset — **wire, not button**   |
| LED (BL)    | **14**   | Backlight enable (active HIGH) |
| VCC         | **3V3**  | 5V also OK per module          |
| GND         | **GND**  |                                |

**SPI config:** 40 MHz, SPI mode 0.

---

## SPI — XPT2046 Touch (shares SPI bus with TFT)

| XPT2046 Pin | ESP GPIO | Notes                |
| ----------- | -------- | -------------------- |
| T_CLK       | **12**   | Shared with TFT SCK  |
| T_DIN       | **11**   | Shared with TFT MOSI |
| T_DO        | **13**   | Shared with TFT MISO |
| T_CS        | **42**   | Separate chip select |
| T_IRQ       | **45**   | Pen-down interrupt   |

**Notes:**

- SPI bus shared with ILI9488 — only one CS active at a time.
- Touch not wired yet (Phase 5).

---

## Misc GPIO

| Device               | ESP GPIO | Direction | Notes                                          |
| -------------------- | -------- | --------- | ---------------------------------------------- |
| PIR 5V motion        | **6**    | Input     | Pull-down, debounce 50 ms. **Skipped for now** |
| NeoPixel strip (ext) | **7**    | Output    | Optional, not wired yet                        |
| Buzzer               | **41**   | Output    | Piezo + transistor, PWM tone                   |
| On-board RGB LED     | **48**   | Output    | Built-in WS2812, always available              |

---

## Pin Summary (quick reference)

| GPIO  | Used by                        | Interface   |
| ----- | ------------------------------ | ----------- |
| 4     | LD2410C OUT (optional)         | Digital in  |
| 6     | PIR (skipped)                  | Digital in  |
| 7     | NeoPixel ext (optional)        | WS2812      |
| 8     | I2C SDA                        | I2C         |
| 9     | I2C SCL                        | I2C         |
| 10    | ILI9488 DC                     | SPI         |
| 11    | SPI MOSI (TFT + touch)         | SPI         |
| 12    | SPI SCK (TFT + touch)          | SPI         |
| 13    | SPI MISO (TFT + touch)         | SPI         |
| 14    | ILI9488 backlight              | PWM out     |
| 15    | LD2410C RX (ESP TX)            | UART2 TX    |
| 16    | LD2410C TX (ESP RX)            | UART2 RX    |
| 17    | ACD1200 RX (ESP TX)            | UART1 TX    |
| 18    | ACD1200 TX via BSS138 (ESP RX) | UART1 RX    |
| 21    | ILI9488 CS                     | SPI         |
| 35-37 | **RESERVED (PSRAM)**           | ⛔          |
| 41    | Buzzer                         | PWM out     |
| 42    | XPT2046 T_CS                   | SPI         |
| 45    | XPT2046 T_IRQ                  | Digital in  |
| 47    | ILI9488 RST                    | Digital out |
| 48    | On-board RGB LED               | WS2812      |
