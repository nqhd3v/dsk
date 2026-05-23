# Desk Guardian — Build Plan

Firmware-first, standalone-first. Each phase produces a runnable artifact. Estimates are rough and assume part-time work — multiply by your actual pace.

## Status legend

- `todo` — not started
- `wip` — in progress
- `done` — finished and verified on hardware
- `blocked` — needs decision or hardware
- `skip` — descoped for this version

---

## Module 1 — ESP32-S3 edge node (standalone)

| Phase | Step | Status | Est. | Notes |
| ----- | ---- | ------ | ---- | ----- |
| 0. Skeleton | Create `apps/firmware/` PlatformIO project, board `esp32-s3-devkitc-1`, framework `arduino` | done | 0.5 d | `platformio.ini`, `.gitignore`, `secrets.h.example`, `README.md` written |
| 0. Skeleton | Configure PSRAM `qio_opi`, 16 MB partition table | done |  | `board_build.arduino.memory_type=qio_opi`, `default_16MB.csv` |
| 0. Skeleton | Smoke test — blink onboard RGB on GPIO 48 | wip |  | Code written (Adafruit NeoPixel, R→G→B cycle + serial heartbeat). **Needs flash test on hardware.** |
| 1. Display | Vendor TFT_eSPI, write `User_Setup.h` for ILI9488 + SPI pins (11/12/13, CS 21, DC 10, RST 47, BL 14) | todo | 1.5 d | Start at 27 MHz SPI |
| 1. Display | RGB fill + text test on TFT | todo |  | Sanity check |
| 1. Display | `Screen` base class + `HomeScreen` skeleton (no real data yet) | todo |  | Render loop at ~10 Hz |
| 2. Sensors | BH1750 driver (I2C @ 0x23), print lux to TFT | todo | 0.5 d | Lib: `claws/BH1750` |
| 2. Sensors | BME680 driver (I2C @ 0x76), T/RH/pressure/gas resistance | todo | 0.5 d | Lib: `adafruit/Adafruit BME680 Library`. BSEC2 optional later |
| 2. Sensors | PIR digital read on GPIO 6 | todo | 0.25 d |  |
| 2. Sensors | LD2410C radar driver (UART2 256000 baud, GPIO 15/16) | todo | 1 d | Lib: `ncmreynolds/ld2410`. Parse moving/stationary + distance |
| 2. Sensors | ACD1200 CO2 driver (UART1 1200 baud, GPIO 17/18, BSS138 on RX) | todo | 1 d | Custom Aosong protocol — no public lib. Reference `/docs/hardware/[Hshop.vn] ACD1200 datasheet Dec 2024.pdf` |
| 2. Sensors | Buzzer PWM tone test on GPIO 41 | todo | 0.25 d |  |
| 3. Logic | Presence FSM: `ABSENT` / `PRESENT_MOVING` / `PRESENT_STATIONARY` from radar + PIR | todo | 0.75 d |  |
| 3. Logic | Sitting timer + stand-up reminder trigger | todo | 0.25 d | Default 45 min |
| 3. Logic | Hydration timer + nudge | todo | 0.25 d | Configurable interval |
| 3. Logic | Threshold persistence in NVS (Preferences API) with defaults | todo | 0.25 d | CO2/lux/sit/hydrate |
| 4. UI | Tiled home layout (lux / T-RH / CO2 / VOC / presence / sit timer) | todo | 0.5 d |  |
| 4. UI | Color-coded thresholds (green/amber/red) | todo | 0.25 d |  |
| 4. UI | Alert overlay screen + buzzer when threshold crossed | todo | 0.25 d |  |
| 5. Touch | XPT2046 driver on shared SPI (T_CS 42, T_IRQ 45) | todo | 0.5 d | Lib: `PaulStoffregen/XPT2046_Touchscreen` |
| 5. Touch | Touch calibration routine stored in NVS | todo | 0.25 d |  |
| 5. Touch | Tap-to-dismiss alerts, settings screen for thresholds | todo | 0.25 d |  |
| 6. Hardening | Watchdog timer + brown-out detector check | todo | 0.5 d |  |
| 6. Hardening | Crash/reboot log to NVS | todo | 0.25 d |  |
| 6. Hardening | Battery-voltage read + low-battery alert (if PMIC exposes ADC) | todo | 0.25 d |  |

**Module 1 subtotal: ~10 days.** Output: working desk guardian, no network, no cloud.

---

## Module 2 — Shared schemas package

| Phase | Step | Status | Est. | Notes |
| ----- | ---- | ------ | ---- | ----- |
| S.1 | `packages/schemas` workspace package, TypeScript + JSON Schema source of truth | todo | 0.5 d | Used by server + web; firmware reads JSON Schema manually |
| S.2 | Define telemetry payload schema (env_v1) | todo | 0.25 d | Fields: ts, node_id, lux, t_c, rh, pressure, gas_ohm, co2_ppm, hcho_ppb?, presence_state, sit_seconds |
| S.3 | Define `hello` + `cmd/*` schemas | todo | 0.25 d |  |

**Subtotal: ~1 day.**

---

## Module 3 — ESP-to-RPi link (additive)

| Phase | Step | Status | Est. | Notes |
| ----- | ---- | ------ | ---- | ----- |
| 7. WiFi | STA mode connect to `DG-<mac>` AP, creds from `secrets.h` initially | todo | 0.5 d | Hardcoded for bench |
| 7. WiFi | Status bar shows WiFi state on TFT | todo | 0.25 d |  |
| 8. MQTT | PubSubClient (or arduino-mqtt) client, broker host/port/user/pass from NVS | todo | 0.5 d |  |
| 8. MQTT | Publish `dg/<node>/telemetry/env` every 10 s using schema | todo | 0.5 d |  |
| 8. MQTT | Retained `dg/<node>/hello` on boot | todo | 0.25 d |  |
| 8. MQTT | Subscribe `dg/<node>/cmd/factory_reset` → NVS wipe + reboot | todo | 0.25 d |  |
| 9. BLE prov | BLE provisioning via `WiFiProv.h` (Arduino wrapper) — replaces hardcoded creds | todo | 1 d | Provision pop, app or web BLE pairing |

**Subtotal: ~3.5 days.**

---

## Module 4 — Raspberry Pi 5 gateway infra

| Phase | Step | Status | Est. | Notes |
| ----- | ---- | ------ | ---- | ----- |
| G.0 | Pi 5 base: Bookworm 64-bit on NVMe, headless, SSH | blocked | 0.5 d | Need Pi on bench |
| G.1 | `infrastructure/mosquitto/` — broker config + per-node ACL files | todo | 0.5 d |  |
| G.2 | `infrastructure/postgres/` — Docker compose with TimescaleDB image, init script for hypertables | todo | 0.5 d | `timescale/timescaledb:latest-pg16` |
| G.3 | `hostapd + dnsmasq` to expose `DG-<mac>` AP on wlan0 | todo | 1 d | Dual-radio path optional |
| G.4 | `infrastructure/caddy/` — reverse proxy config (web + API + WS) | todo | 0.5 d |  |
| G.5 | Grafana container (optional) pointed at Postgres | todo | 0.25 d | Skip if not needed |

**Subtotal: ~3 days.**

---

## Module 5 — NestJS backend

| Phase | Step | Status | Est. | Notes |
| ----- | ---- | ------ | ---- | ----- |
| B.0 | Scaffold `apps/server` NestJS project in pnpm workspace | todo | 0.5 d |  |
| B.1 | Postgres + Timescale connection (TypeORM or Prisma); migrations for `devices` + telemetry hypertable | todo | 1 d | Pick ORM — open question |
| B.2 | MQTT subscriber module via `@nestjs/microservices` | todo | 0.5 d | Subscribe `dg/+/telemetry/env`, validate against schema |
| B.3 | Persist telemetry to Timescale hypertable | todo | 0.5 d |  |
| B.4 | Device registry REST: list / approve / rename / remove | todo | 1 d | Triggers Mosquitto ACL writes + `cmd/factory_reset` publish |
| B.5 | WebSocket gateway pushing live telemetry to web | todo | 0.5 d | `@nestjs/websockets` (socket.io) |
| B.6 | Threshold config REST (read/write per device) | todo | 0.5 d | Pushes config back over MQTT `dg/<node>/cmd/config` |
| B.7 | BLE bridge for provisioning (Pi side advertises/scans) | todo | 1.5 d | Or skip — provision via phone BLE app |

**Subtotal: ~5.5 days.**

---

## Module 6 — Next.js web dashboard

| Phase | Step | Status | Est. | Notes |
| ----- | ---- | ------ | ---- | ----- |
| W.0 | Strip Next.js starter page in `apps/web` | todo | 0.25 d |  |
| W.1 | Layout shell + auth-free local dashboard | todo | 0.5 d | Single-user, local network |
| W.2 | Live dashboard page: tiles, WebSocket subscription | todo | 1.5 d |  |
| W.3 | Device management page: list, approve pending, rename, remove | todo | 1 d |  |
| W.4 | Threshold config page per device | todo | 0.5 d |  |
| W.5 | History charts (1h / 24h / 7d) from Postgres via NestJS API | todo | 1 d |  |

**Subtotal: ~4.75 days.**

---

## Grand total (rough)

| Group | Days |
| ----- | ---- |
| ESP standalone (Module 1) | ~10 |
| Schemas (Module 2) | ~1 |
| ESP↔RPi link (Module 3) | ~3.5 |
| Pi infra (Module 4) | ~3 |
| NestJS (Module 5) | ~5.5 |
| Web (Module 6) | ~4.75 |
| **Total** | **~28 days part-time** |

---

## Open decisions to resolve before Module 4+

- ORM for NestJS — Prisma or TypeORM? (Prisma cleaner, TypeORM more native to NestJS.)
- BLE provisioning UX — phone app, Pi-side scan, or web BLE in browser?
- Grafana in or out?

---

## Devices/parts to verify on bench before Phase 1

- BSS138 wired between ACD1200 TX (5V) and ESP GPIO 18 (3V3 RX).
- Common GND across all sensors + ESP.
- CH343P driver installed on dev machine.
- 3V3 vs 5V rails verified with multimeter before plugging.
