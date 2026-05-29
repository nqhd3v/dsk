# Desk Guardian — Wiring Reference

All connections for the ESP32-S3 MKE-K01 (N16R8) edge node.

**⚠ ALWAYS power off ESP (unplug USB) before wiring any new connection.**

**⚠ GPIO 35/36/37 are reserved for PSRAM — do NOT use.**

---

## Power Rails

| Rail | Source | Feeds |
|------|--------|-------|
| 3V3 | ESP on-board LDO | BH1750, BME680, BSS138 A-side, ILI9488 VCC |
| 5V | ESP USB 5V pin | ACD1200, LD2410C, BSS138 B-side |
| GND | Common | **All modules share same GND** |

---

## I2C Bus (shared, 2 devices)

| Signal | ESP GPIO | BH1750 | BME680 |
|--------|----------|--------|--------|
| SDA | **8** | SDA | SDA |
| SCL | **9** | SCL | SCL |
| VCC | 3V3 | VCC | VCC |
| GND | GND | GND | GND |

**Notes:**
- Both modules include on-board pull-ups — no external resistors needed.
- BME680 SDO pin → tie to **VCC (3V3)** for address **0x77**. Tie to GND for 0x76.
- BH1750 ADDR pin → leave floating or GND for address **0x23**.
- Bus speed: 400 kHz (fast mode).

---

## UART1 — ACD1200 CO2 Sensor (via BSS138 level shifter)

### ACD1200 Sensor

| ACD1200 Pin | Connect to | Notes |
|-------------|-----------|-------|
| VCC | ESP **5V** | Sensor needs 5V |
| GND | ESP **GND** | |
| RX | ESP **GPIO 17** | Direct wire — 3V3 into 5V-tolerant input |
| TX | BSS138 **BSDA** | 5V output — **MUST go through shifter** |
| SET (pin 5) | **GND** | Tie to GND = UART mode |

### BSS138 Level Shifter Board

Your board labels: A-side (3.3V), B-side (5V).

| BSS138 Pin | Connect to | Role |
|------------|-----------|------|
| **AVCC** | ESP **3V3** | A-side reference voltage |
| **AGND** | ESP **GND** | |
| **ASDA** | ESP **GPIO 18** (RX) | Shifted 3.3V output → safe for ESP |
| **BVCC** | ESP **5V** | B-side reference voltage |
| **BGND** | ESP **GND** | |
| **BSDA** | ACD1200 **TX** | 5V signal input from sensor |
| ASCL | — | Unused, leave empty |
| BSCL | — | Unused, leave empty |

**Signal path:**
```
ACD1200 TX (5V) → BSDA → [BSS138 shifts 5V→3V3] → ASDA → ESP GPIO 18
ESP GPIO 17 (TX) ──────────────────────────────────→ ACD1200 RX (direct)
```

**UART config:** 1200 baud, 8N1. Protocol: Aosong custom (NOT Modbus).

---

## UART2 — LD2410S Radar (active)

**⚠ LD2410S VCC = 3.3V (3.0–3.6V). DO NOT connect to 5V — will damage the module!**

| LD2410S J2 Pin | ESP GPIO | Notes |
|----------------|----------|-------|
| Pin1 (3V3) | **3V3** | **NOT 5V!** |
| Pin2 (GND) | **GND** | |
| Pin4 (RX) | **GPIO 15** (ESP TX) | |
| Pin3 (OT1/TX) | **GPIO 16** (ESP RX) | Radar TX is 3.3V — direct wire OK |
| Pin5 (OT2) | GPIO 4 (optional) | Digital presence output, unused in firmware |

**UART config:** 115200 baud, 8N1.

**Notes:**
- No level shifter needed — both sides are 3.3V.
- Must use Serial2 in firmware (`Serial2.begin(115200, SERIAL_8N1, 16, 15)`).
- LD2410S reports presence + distance only (no separate moving/stationary energy).
  Sitting inference done via distance stability in driver.

---

## UART2 — LD2410C Radar (parked — hardware wiring issue)

> **Not in use.** LD2410C produced zero UART data during bench test. Kept for future re-test.
> LD2410C is 5V (unlike LD2410S which is 3.3V).

| LD2410C Pin | ESP GPIO | Notes |
|-------------|----------|-------|
| VCC | **5V** | |
| GND | **GND** | |
| RX | **GPIO 15** (ESP TX) | |
| TX | **GPIO 16** (ESP RX) | Radar TX is 3.3V — direct wire OK |
| OUT | GPIO 4 (optional) | Digital presence pin, unused in firmware |

**UART config:** 256000 baud, 8N1.

---

## SPI — ILI9488 + XPT2046 (board pin order, left to right)

Board has 14 pins in a single row. TFT (pins 1–9) and touch (pins 10–14) share SPI bus.

| # | Board Label | ESP GPIO | Notes |
|---|------------|----------|-------|
| 1 | VCC | **3V3** | Power (5V also OK per module) |
| 2 | GND | **GND** | |
| 3 | CS | **21** | Display chip select |
| 4 | RESET | **47** | Wire, not button |
| 5 | DC/RS | **10** | Data/command select |
| 6 | SDI (MOSI) | **11** | SPI data to display |
| 7 | SCK | **12** | SPI clock (HSPI) |
| 8 | LED | **14** | Backlight enable (active HIGH) |
| 9 | SDO (MISO) | **13** | SPI data from display (optional) |
| 10 | T_CLK | **12** | Shared with SCK |
| 11 | T_CS | **42** | Touch chip select (separate) |
| 12 | T_DIN | **11** | Shared with MOSI |
| 13 | T_DO | **13** | Shared with MISO |
| 14 | T_IRQ | **45** | Pen-down interrupt |

**SPI config:** 40 MHz, SPI mode 0.

**Notes:**
- SPI bus shared between TFT and touch — only one CS active at a time.
- Pins 10–14 (touch) = Phase 5, skip wiring for now unless wiring all at once.

---

## Misc GPIO

| Device | ESP GPIO | Direction | Notes |
|--------|----------|-----------|-------|
| PIR 5V motion | **6** | Input | Pull-down, debounce 50 ms. **Skipped for now** |
| NeoPixel strip (ext) | **7** | Output | Optional, not wired yet |
| Buzzer | **41** | Output | Piezo + transistor, PWM tone |
| On-board RGB LED | **48** | Output | Built-in WS2812, always available |

