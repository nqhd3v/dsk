# Desk Guardian — Project Summary

Two-module IoT system. ESP32-S3 edge node senses environment + presence and runs reminders **fully standalone**. Raspberry Pi 5 gateway is an **optional add-on** that stores history and serves a dashboard. RPi acts as Wi-Fi AP so system is portable (works any location, no home router needed).

---

## 1. Hardware

| #   | Device                                 | Role                                     | Interface                         | Notes                                                                                                            |
| --- | -------------------------------------- | ---------------------------------------- | --------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| 1   | MKE-K01 board (ESP32-S3-WROOM-1 N16R8) | Edge MCU                                 | —                                 | 16 MB Flash, 8 MB OPI PSRAM, CH343P USB-UART, on-board RGB LED on GPIO 48. **GPIO 35/36/37 reserved for PSRAM — do not use.** |
| 2   | Raspberry Pi 5                         | Gateway / AP / dashboard host (optional) | LAN + Wi-Fi AP                    | Bookworm 64-bit on NVMe, optional UPS, optional LTE                                                              |
| 3   | BH1750                                 | Ambient light (lux)                      | I2C @ 0x23                        | Confirmed working, 200–400 lux indoor                                                                            |
| 4   | BME680                                 | Temp / RH / pressure / VOC (gas)         | I2C @ 0x77                        | Adafruit lib; BSEC2 needs 5–20 min warmup for IAQ                                                                |
| 5   | ACD1200 (Aosong)                       | CO2 (NDIR)                               | UART @ 1200 baud                  | Aosong custom protocol (NOT Modbus). 5V TX → needs BSS138 level shifter to ESP RX. Pin 5 SET = GND for UART mode |
| 6   | HLK-LD2410C                            | mmWave presence + distance (24 GHz FMCW) | UART @ 256000 baud, **5V**        | **Parked** — hardware wiring issue (zero UART data). Driver code kept. Has moving/stationary classification + energy. |
| 7   | HLK-LD2450                             | mmWave 1T2R motion target tracking (24 GHz) | UART @ 256000 baud, **5V** (3.3V IO) | **Active.** Tracks up to 3 targets: X/Y/speed/distRes. Frame `AA FF 03 00 [3×8B] 55 CC` @10Hz. Sign-magnitude int16 (bit15=1→positive). Distance = sqrt(x²+y²). **No OT2 pin.** Presence = nearest target ≤150cm. Azimuth ±60°, pitch ±35°, range 6m. Driver `ld2450.{h,cpp}` (raw parser, no lib). |
| 7b  | HLK-LD2410S                            | mmWave presence + distance (24 GHz FMCW) | UART @ 115200 baud, **3.3V**      | **Parked** — replaced by LD2450. Driver kept. Minimal frame: `6E [state] [dist_lo] [dist_hi] 62`. OT2 pin = digital presence (GPIO 4). 2-state model: PRESENT (≤150cm) / ABSENT. 60° cone, ~8 m moving, ~4 m stationary. |
| 8   | PIR 5V                                 | Coarse motion wake-up                    | GPIO                              | **Skipped** — LD2410C radar covers motion + stationary detection. PIR adds no value |
| 9   | HCHO sensor (DFRobot)                  | Formaldehyde                             | I2C (SEN0568) or analog (SEN0231) | SKU TBC, postponed                                                                                               |
| 10  | ILI9488 3.5" TFT (480×320)             | UI display (touch unused)                | SPI                               | nshopvn.com module, 3V3/5V tolerant, backlight on GPIO 14. **`setRotation(3)`** — display mounted 180° in box. XPT2046 touch wired but **not used in firmware** (no touch features yet). |
| 11  | BSS138 level shifter board             | 5V ↔ 3V3 bidirectional                   | —                                 | For ACD1200 TX → ESP RX                                                                                          |
| 12  | Buzzer                                 | Reminder beeper                          | GPIO 41                           | Stand-up / hydration cues                                                                                        |
| 13  | 2× 18650 Li-ion + BQ24074 + boost      | Portable power path                      | —                                 | Power node off USB                                                                                               |

---

## 2. How System Works

**Operating model — standalone-first:** the ESP32 edge node runs the full product loop (sense + reminders + local UI) without any RPi. Adding the RPi is purely additive — it enables history, remote dashboard, and multi-node management.

**Edge node (ESP32-S3) — always required:**

- Read sensors every ~10 s: lux, T, RH, VOC, CO2, HCHO.
- Radar + PIR feed presence FSM → track sitting time.
- Local TFT shows live readings + reminders.
- Reminder logic: stand-up after sitting threshold, hydrate timer, exercise nudge, suggest "open door / fan" when CO2 high or VOC bad.
- Buzzer + on-screen alert when threshold crossed.
- Thresholds stored in NVS (Preferences API), defaults from §5.
- **If** Wi-Fi + MQTT creds present in NVS → also publish telemetry JSON over MQTT every 10 s.
- Wi-Fi creds delivered via BLE provisioning (Arduino `WiFiProv` wrapper over ESP-IDF wifi_provisioning) → saved in NVS. Bench builds may hardcode creds in `secrets.h`.

**Gateway (RPi 5) — optional add-on:**

- Runs **hostapd + dnsmasq** as Wi-Fi AP `DG-<mac>` → ESP nodes join this isolated network.
- Optionally also joins home Wi-Fi for internet (dual-interface).
- Docker stack: **Mosquitto** (MQTT broker), **PostgreSQL + TimescaleDB** (single store for both device registry and time-series telemetry), **NestJS** (REST + WebSocket + MQTT subscriber + serves Next.js).
- NestJS subscribes to Mosquitto, writes telemetry rows into a Timescale hypertable, and pushes live updates to the Next.js dashboard via WebSocket (<2 s).
- Grafana optional — can point at the same Postgres for ad-hoc charts.

**Presence model (2-state):**

Simple binary: **PRESENT** (sensor says someone AND smoothed distance ≤ 150 cm) or **ABSENT** (no one OR distance > 150 cm). OT2 digital pin used as presence ground truth when wired (faster than UART state which has built-in unmanned delay). Distance smoothed with 5-sample moving average.

**Screen FSM (4-state display system):**

The TFT display runs a 4-state finite state machine driven by presence:

| Mode | Trigger | Display | Backlight |
|------|---------|---------|-----------|
| ACTIVE | Person PRESENT | Left: countdown timer (MM:SS to sit reminder). Right: 6 env rows (Radar, Light, Temp, Humidity, CO2, Air). Footer: pressure + sitting duration | ON |
| ALERT | Sitting countdown reaches 0 (default 45 min) | Full-screen "Stand up!" with pulsing red/orange animation, sitting duration, "Move to dismiss" hint | ON |
| SUMMARY | Absent > 60 s | Centered 3×2 env cards (Light, Temp, Humidity, CO2, Air, Pressure). "Away" header | ON |
| SLEEP | Absent > 5 min | Nothing rendered | OFF |

Transitions: ACTIVE+present→countdown accumulates. Away >10s→reset countdown. Away >60s→SUMMARY. Away >5min→SLEEP. ALERT→ACTIVE on presence return or 30s auto-dismiss. SUMMARY/SLEEP→ACTIVE on any presence. Thresholds configurable (hardcoded now, NVS later, web dashboard after RPi).

**Data flow (when RPi present):** sensor → ESP32 → MQTT topic `dg/<node>/telemetry/env` → NestJS MQTT subscriber → PostgreSQL/Timescale → NestJS WebSocket → browser tile.

**Device management (add / remove from dashboard):**

_Add ESP node:_

1. New ESP boot → no NVS Wi-Fi creds → BLE advertise as `DG-prov-<chipid>`.
2. User opens RPi dashboard → "Add device" → RPi scans BLE (or phone bridges).
3. Pairing: dashboard pushes AP SSID + PSK + MQTT host/user/pass to ESP over BLE (`wifi_provisioning` proto).
4. ESP saves to NVS, reboots, joins `DG-<mac>` AP, publishes retained `dg/<node>/hello`.
5. Dashboard sees hello → shows "pending" tile → user names it + approves.
6. RPi writes node row in `devices` table (Postgres), enables Mosquitto ACL for that node.

_Remove ESP node:_

1. Dashboard → device → "Remove".
2. RPi revokes MQTT user, drops ACL, deletes registry row, publishes `dg/<node>/cmd/factory_reset`.
3. ESP receives cmd → wipes NVS → reboots → back to BLE prov mode.
4. Historical telemetry stays in Postgres; no new writes accepted.

_Auth model:_ each ESP gets own MQTT creds (`node1`, `node2`, …) so per-device revoke does not affect others.

**Portable:** whole system runs off RPi AP, no external infrastructure required. ESP alone runs off 2× 18650.

---

## 3. System Schema

```
                ┌─────────────────────────────────────────────┐
                │              RASPBERRY PI 5                 │
                │                 (optional)                  │
                │  ┌────────┐  ┌─────────┐  ┌──────────────┐  │
                │  │hostapd │  │Mosquitto│  │  PostgreSQL  │  │
                │  │dnsmasq │  │ (MQTT)  │  │ + Timescale  │  │
                │  └────┬───┘  └────┬────┘  └──────┬───────┘  │
                │       │           │              ▲           │
                │  Wi-Fi AP    pub/sub             │           │
                │   DG-<mac>        │              │           │
                │       │           ▼              │           │
                │       │      ┌─────────────────┐ │           │
                │       │      │     NestJS      │─┘           │
                │       │      │ MQTT sub + REST │             │
                │       │      │ + WebSocket     │             │
                │       │      └────────┬────────┘             │
                │       │               │                       │
                │       │          ┌────▼────┐                  │
                │       │          │ Next.js │ ← served by NestJS│
                │       │          └─────────┘                  │
                │       │                                       │
                │  (opt) wlan1 ──→ Home Wi-Fi → Internet        │
                └───────┼───────────────────────────────────────┘
                        │ Wi-Fi (isolated AP, only if RPi present)
                        ▼
         ┌────────────────────────────────────┐
         │       ESP32-S3 EDGE NODE           │
         │       (runs fully standalone)      │
         │                                    │
         │  ┌──────────────┐                  │
         │  │   ILI9488    │ ← SPI            │
         │  │ TFT + touch  │                  │
         │  └──────────────┘                  │
         │                                    │
         │  I2C bus ──┬── BH1750 (lux)        │
         │            ├── BME680 (T/RH/VOC)   │
         │            └── HCHO (optional)     │
         │                                    │
         │  UART1 ──── ACD1200 (CO2, via BSS138) │
         │  UART2 ──── LD2410C (radar)        │
         │                                    │
         │  GPIO ───── PIR, Buzzer, LED       │
         │                                    │
         │  Power: 2× 18650 → BQ24074 → 3V3   │
         └────────────────────────────────────┘
```

---

## 4. Wiring (ESP32-S3 MKE-K01)

### I2C bus (shared)

| Signal | ESP32 GPIO | BH1750 | BME680 | HCHO (I2C SKU) |
| ------ | ---------- | ------ | ------ | -------------- |
| SDA    | 8          | SDA    | SDA    | SDA            |
| SCL    | 9          | SCL    | SCL    | SCL            |
| VCC    | 3V3        | VCC    | VCC    | VCC            |
| GND    | GND        | GND    | GND    | GND            |

### UART1 — ACD1200 (CO2)

| Signal            | ESP32 GPIO | ACD1200 | Notes                                   |
| ----------------- | ---------- | ------- | --------------------------------------- |
| TX (ESP → sensor) | 17         | RX      | Direct OK (3V3 → 5V tolerant input)     |
| RX (sensor → ESP) | 18         | TX      | **Via BSS138 level shifter** (5V → 3V3) |
| VCC               | 5V         | VCC     |                                         |
| GND               | GND        | GND     |                                         |
| SET (pin 5)       | —          | GND     | Forces UART mode                        |

### UART2 — LD2410C (radar)

| Signal           | ESP32 GPIO | LD2410C | Notes                                            |
| ---------------- | ---------- | ------- | ------------------------------------------------ |
| TX (ESP → radar) | 15         | RX      | 256000 baud                                      |
| RX (radar → ESP) | 16         | TX      | Radar TX is 3V3 — direct OK                      |
| VCC              | 5V         | VCC     |                                                  |
| GND              | GND        | GND     |                                                  |
| OUT (optional)   | 4          | OUT     | Digital presence pin, unused (UART carries data) |

### SPI — ILI9488 display (Stage A: display only)

| Signal | ESP32 GPIO | ILI9488    | Notes                          |
| ------ | ---------- | ---------- | ------------------------------ |
| SCK    | 12         | SCK        | HSPI                           |
| MOSI   | 11         | MOSI / SDI |                                |
| MISO   | 13         | MISO / SDO | Optional, only if reading back |
| CS     | 21         | CS         | Display chip select            |
| DC     | 10         | DC / RS    | Data/command                   |
| RST    | 47         | RST        | **Wire, not button**           |
| BL     | 14         | LED        | Backlight enable               |
| VCC    | 3V3        | VCC        | 5V also OK per module          |
| GND    | GND        | GND        |                                |

### SPI — XPT2046 touch (Stage B, shared SPI bus)

| Signal | ESP32 GPIO | XPT2046 | Notes              |
| ------ | ---------- | ------- | ------------------ |
| SCK    | 12         | T_CLK   | Shared with TFT    |
| MOSI   | 11         | T_DIN   | Shared             |
| MISO   | 13         | T_DO    | Shared             |
| T_CS   | 42         | T_CS    | Separate CS        |
| T_IRQ  | 45         | T_IRQ   | Pen-down interrupt |

### Misc GPIO

| Signal       | ESP32 GPIO | Device                             |
| ------------ | ---------- | ---------------------------------- |
| LD2410S OT2  | 4          | Radar digital presence (HIGH=someone, LOW=no one) |
| PIR OUT      | 6          | PIR 5V motion (input) — skipped    |
| LED_DIN      | 7          | External NeoPixel strip (optional) |
| Buzzer       | 41         | Piezo + transistor                 |
| On-board RGB | 48         | Built-in WS2812 (reserved)         |

### Power

| Source          | Path                                                                                 |
| --------------- | ------------------------------------------------------------------------------------ |
| 2× 18650 Li-ion | → BQ24074 charger/PMIC → boost to 5V → MKE-K01 USB-UART 5V rail → on-board LDO → 3V3 |
| USB-C charge    | BQ24074 input                                                                        |

### Wiring rules

> **Detailed wiring with board pin labels:** see `docs/hardware/wire.md` for pin-by-pin reference matching physical board labels (ILI9488 left-to-right board order, BSS138 AVCC/BVCC labels, ACD1200 pinout).

- Unplug ESP USB **before** wiring any new VCC.
- I2C devices share bus — add pull-ups only if module has none (BH1750 + BME680 modules usually include them).
- ACD1200 sensor TX is 5V → **must** go through BSS138 before reaching ESP RX.
- Common GND across all modules + ESP + RPi during bench debug.
- **GPIO 35/36/37 are reserved** by the N16R8 module for octal PSRAM — do not route them out.

---

## 5. Reminder thresholds (defaults)

Stored in NVS, editable via touch UI (later) or web dashboard (after RPi added).

| Signal | Threshold | Action |
| ------ | --------- | ------ |
| CO2    | > 1000 ppm | Ventilate / open door / fan |
| VOC    | elevated (BME680 gas resistance drop) | Air quality alert |
| Lux    | < 200 or > 400 lux | Adjust lighting |
| Sitting time | > 45 min continuous PRESENT_STATIONARY | Stand-up reminder |
| Idle (no movement) | configurable | Hydration nudge |

---

## 6. Tech stack — locked

| Layer | Choice |
| ----- | ------ |
| Firmware | PlatformIO + Arduino-ESP32, TFT_eSPI (FreeFonts via extern decl, not Free_Fonts.h) |
| MQTT broker | Mosquitto |
| Database | PostgreSQL + TimescaleDB extension (single DB for registry + telemetry) |
| Backend | NestJS (TypeScript) with `@nestjs/microservices` MQTT transport |
| Web | Next.js 16.2.6 + React 19 |
| Analytics (opt) | Grafana on top of Postgres |

See `docs/system/planning.md` for build order and status.
