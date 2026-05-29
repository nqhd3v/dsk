# Desk Guardian — Build Plan & Status

This file is the **single source of truth** for what is done, in progress, and next. Update it as work progresses. New chats should read this file first.

---

## How to resume work in a new chat

Paste this kickoff message into a new conversation. The agent will know exactly where things stand.

```
We are continuing work on the dsk-guard project (IoT desk wellness monitor).
Repo root: /Users/huynguyen/apps/dsk-guard

Please read these files in order before doing anything:
  1. CLAUDE.md                       — project rules, stack, monorepo layout
  2. docs/system/README.md           — full system spec, hardware + protocols
  3. docs/system/planning.md         — build plan & current status (THIS IS THE SOURCE OF TRUTH)
  4. docs/hardware/README.md         — datasheet index

Then check the "Current state" section in planning.md and tell me:
  - what phase we are on
  - what the next concrete step is
  - any open questions or blockers listed

Do NOT start coding until I confirm the next step.
Caveman mode is the default response style.
```

If the agent has memory access, it will also find these notes:

- `user_role.md` — collaboration style
- `project_dskguard_stack.md` — locked tech decisions

---

## Current state

| Field | Value |
|-------|-------|
| Last completed phase | **Module 2 (schemas) + Module 3 (ESP WiFi+MQTT) + Module 4 (infra) + Module 5 (NestJS backend)** |
| Current phase | **Module 6 — Next.js dashboard** |
| Next step | Build live dashboard (W.0→W.1) |
| Hardware on bench | ESP32-S3 MKE-K01, ILI9488 TFT, BH1750, BME680, LD2410S (3.3V, UART2 + OT2), ACD1200 (via BSS138), buzzer |
| All wired? | All sensors wired. OT2 (GPIO 4) optional but recommended for faster presence. |
| ESP standalone working? | Yes. All sensors reading on TFT. 4-state FSM transitions working. Countdown smooth. |
| ESP WiFi+MQTT working? | Yes. Non-blocking WiFi, NTP sync, MQTT telemetry every 10s, hello beacon retained. |
| RPi gateway infra | Docker stack running: Mosquitto + Postgres/TimescaleDB + Caddy. Wi-Fi AP (hostapd+dnsmasq on wlan1) configured. |
| Full pipeline verified? | **Yes.** ESP→MQTT→Postgres→WebSocket broadcast confirmed 2026-05-29. Server logs "WS broadcast: node1". |
| NestJS backend | **Done.** Port 4000. MQTT sub, telemetry persist, device CRUD, config push, WebSocket broadcast. |
| Schemas package | `@dsk/schemas` built (CJS). EnvTelemetry, Hello, Cmd, Device, MQTT topics. |
| Web app | Socket.io client + hooks done (`lib/socket.ts`, `hooks/use-socket.tsx`). Dashboard pages not started. |
| Open decisions | BLE provisioning UX (phone vs web BLE) |
| Last verified | Full pipeline ESP→MQTT→Postgres→WS working 2026-05-29. |

---

## Status legend

- `todo` — not started
- `wip` — code written, not yet verified on hardware
- `done` — finished AND verified on hardware
- `blocked` — needs decision or hardware
- `skip` — descoped for this version

When marking `done`, always include the verification result in the **Notes** column or the phase detail subsection.

---

## Module 1 — ESP32-S3 edge node (standalone)

### Phase 0 — Skeleton

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 0.1 Create `apps/firmware/` PlatformIO project, board `esp32-s3-devkitc-1`, framework `arduino` | done | 0.5 d | `platformio.ini`, `.gitignore`, `secrets.h.example`, `README.md` written |
| 0.2 Configure PSRAM `qio_opi`, 16 MB partition | done |  | `board_build.arduino.memory_type=qio_opi`, `default_16MB.csv` |
| 0.3 RGB smoke test on GPIO 48 | done |  | Verified on hardware 2026-05-24: PSRAM 8388608 detected, heap stable, R→G→B cycle visible. See log in chat history. |

**Goal:** confirm toolchain, flash, PSRAM, on-board WS2812 LED, CH343P serial.

**Key files:**
- `apps/firmware/platformio.ini`
- `apps/firmware/src/main.cpp`
- `apps/firmware/include/secrets.h.example`
- `apps/firmware/README.md`

**Acceptance (met):** Serial shows `flash size: 16777216`, `psram size: 8388608`. On-board RGB cycles R→G→B every 1 s. Heartbeat log every 10 s, heap stays >300 KB.

---

### Phase 1 — Display foundation

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 1.1 Wire ILI9488 to ESP per `docs/system/README.md` §4 — SCK 12, MOSI 11, MISO 13, CS 21, DC 10, RST 47, BL 14 | done | 0.25 d | Wired 2026-05-24 |
| 1.2 Add TFT_eSPI to `lib_deps`, create `apps/firmware/src/User_Setup.h` with ILI9488 + ESP32-S3 pin map | done | 0.5 d | Verified 2026-05-24. SPI @ 27 MHz, no corruption |
| 1.3 Smoke test: `tft.fillScreen(TFT_RED)` → green → blue with 1 s interval | done | 0.25 d | Verified 2026-05-24. R→G→B fills visible, no artifacts |
| 1.4 SPI clock — start 27 MHz, raise to 40 MHz if stable | done | 0.25 d | 40 MHz stable, no artifacts. Confirmed 2026-05-24 |
| 1.5 Build `Screen` base class + `HomeScreen` skeleton (placeholder text) | done | 0.25 d | Verified: title "Desk Guardian" + heap/PSRAM/uptime stats display correctly |
| 1.6 Render loop in `loop()` at ~10 Hz, only redraw dirty regions | done | 0.25 d | Stats update every 1 s, no flicker. Heap stable at 360 KB |
| 1.7 Backlight control on GPIO 14 — start always-on, prep for PWM later | done | 0.1 d | Backlight active HIGH confirmed working |

**Goal:** ILI9488 wired, driven by TFT_eSPI, render loop in place, "Hello" placeholder visible.

**Key files (created):**
- `apps/firmware/src/User_Setup.h` — TFT_eSPI config for ILI9488 + S3 pins
- `apps/firmware/src/ui/Screen.h` — base class with dirty-flag pattern
- `apps/firmware/src/ui/HomeScreen.{h,cpp}` — placeholder: title + heap/PSRAM/uptime stats
- `apps/firmware/src/main.cpp` — TFT init, smoke test, render loop, backlight

**Acceptance:** boot shows red→green→blue full-screen fills, then HomeScreen text. No SPI corruption. Heap still >250 KB after init.

**Risk notes:**
- ILI9488 module from nshopvn.com may need `TFT_BL` active-HIGH (some clones are inverted).
- If RST stays low → display blanks. Confirm GPIO 47 actually wired to RST pin.
- TFT_eSPI is picky about `User_Setup.h` discovery — must use the project-local include flag pattern, NOT edit the lib's vendored file.

---

### Phase 2 — Sensor drivers (one at a time)

Order chosen to build confidence: easiest first.

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 2.1 BH1750 (I2C @ 0x23, SDA 8, SCL 9). Add `claws/BH1750` lib. Print lux to TFT debug screen. | done | 0.5 d | Verified 2026-05-24. I2C scan confirms 0x23. Lux reads on TFT + serial |
| 2.2 BME680 (I2C @ 0x77). Add `adafruit/Adafruit BME680 Library` + Adafruit Unified Sensor. Read T/RH/pressure/gas resistance. | done | 0.5 d | Verified 2026-05-24. Board SDO=HIGH → addr 0x77 (not 0x76). T/RH/P/gas on TFT. Gas warmup 5-20 min. **Note:** I2C intermittent — sometimes not detected on scan. Wiring/loose connection suspected. |
| 2.3 PIR (GPIO 6 digital input). | skip | — | Skipped — LD2410C radar covers motion + presence detection. PIR adds no value over mmWave |
| 2.4a LD2410C radar (UART2 @ 256000 baud, 5V). | blocked | 1 d | Driver written (`sensors/ld2410c.{h,cpp}`), **parked** — zero UART data on bench. Wiring issue suspected. `ncmreynolds/ld2410` lib has swapped fields — driver compensates. Code kept for future use. |
| 2.4b LD2410S radar (UART2 @ 115200 baud, **3.3V**, GPIO 15 TX, 16 RX, OT2 GPIO 4). Raw minimal frame parser, no library. | done | 0.5 d | Driver written (`sensors/ld2410s.{h,cpp}`). Parses minimal 5-byte frames `6E [state] [dist_lo] [dist_hi] 62` (factory default). OT2 digital pin for instant presence. 2-state model: PRESENT (someone + dist ≤150cm) / ABSENT. Distance smoothed (5-sample moving average). Verified on hardware — frames parsing, distance + state on screen. |
| 2.5 ACD1200 CO2 (UART1 @ 1200 baud, GPIO 17 TX, 18 RX **via BSS138**). Custom driver `acd1200.{h,cpp}`. SET pin = GND for UART mode. | done | 1 d | Verified 2026-05-27. Aosong custom protocol (NOT Modbus). CMD: FE A6 00 01 A7, 9-byte reply, CO2 = D1×256+D2. BSS138 wired with user's board labels (AVCC/BVCC). 120s preheat tracked. Reads CO2 ppm on TFT |
| 2.6 Buzzer (GPIO 41). PWM tone test, single beep. | todo | 0.25 d | `ledcAttach(41, 4000, 8)` etc. Will integrate with AlertScreen |

**Layout convention:** each sensor in its own module under `apps/firmware/src/sensors/<name>.{h,cpp}` exposing a common shape:

```cpp
struct Reading { /* sensor-specific fields */ bool ok; uint32_t at_ms; };
class Bh1750Sensor {
public:
  bool begin();
  Reading read();
};
```

**Acceptance per sensor:** values displayed on a debug TFT screen, updated every 1 s, looks sane against a phone lux app / breath test / hand wave.

**Risk notes:**
- I2C bus pull-ups: BH1750 + BME680 modules normally include them. If both pulled up → fine. If only one → still fine. If neither → add 4.7 kΩ to 3V3.
- ACD1200: 5 V TX MUST go through BSS138 before ESP RX (GPIO 18). Wrong wiring → fried ESP pin.
- LD2410C must be wired to UART2 (Serial2), not USB UART. Set `Serial2.begin(256000, SERIAL_8N1, 16, 15)`.

---

### Phase 3 — Presence + reminder FSM

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 3.1 Presence FSM: 2-state `PRESENT` / `ABSENT` from radar | done | 0.5 d | Simplified to binary. ScreenFsm uses PRESENT/ABSENT only. No moving/stationary distinction. `radar_types.h` PresenceState enum |
| 3.2 Sitting timer — accumulate seconds while PRESENT | done | 0.25 d | In ScreenFsm: `_sitAccumMs` accumulates via delta-time while present. Away >10s resets countdown. Away >60s→SUMMARY. Away >5min→SLEEP |
| 3.3 Stand-up reminder trigger when countdown reaches 0 | done | 0.25 d | Default 45 min → ACTIVE→ALERT transition. AlertScreen shows "Stand up!" with pulse animation |
| 3.4 Hydration timer + nudge | todo | 0.25 d | Default 60 min while present. Not yet implemented |
| 3.5 Thresholds in NVS (Preferences API): `co2_max`, `lux_min`, `lux_max`, `sit_minutes`, `hydrate_minutes` | todo | 0.25 d | Currently hardcoded in ScreenFsmConfig. NVS storage deferred to hardening phase |
| 3.6 Reminder dispatcher: any rule fires → buzz + AlertScreen | done | 0.25 d | ScreenFsm transitions drive screen switch. Alert auto-dismisses on movement or 30s timeout. Buzzer integration pending (2.6) |

**Key files:**
- `apps/firmware/src/logic/ScreenFsm.{h,cpp}` — 4-state machine (ACTIVE/ALERT/SUMMARY/SLEEP)
- `apps/firmware/src/ui/AlertScreen.{h,cpp}` — full-screen pulsing reminder

**Acceptance:** sit at desk → countdown runs from 45:00 down → reaches 0 → AlertScreen fires. Return presence → dismissed back to ACTIVE. Walk away >10s → countdown resets. Away >60s → SummaryScreen. Away >5min → SLEEP (backlight off). Any presence → wakes to ACTIVE.

---

### Phase 4 — UI polish

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 4.1 ActiveScreen — redesigned v3 | done | 0.5 d | Left: HH:MM clock (primary) + "Stand up in XX min" hint + [Active] pill + actionable suggestion. Right: 2×2 mini cards (Temp °C, Humidity %, CO2 quality, Air quality). Footer: raw env data + online dot (green/dim). |
| 4.2 Color-code each value by threshold (green / amber / red) | done | 0.25 d | Lux <200 orange, >400 yellow. CO2 >800 yellow, >1000 red. Countdown <2min red, <10min yellow |
| 4.3 SummaryScreen — 3×2 env cards when absent | done | 0.25 d | Centered cards: Light, Temp, Humidity, CO2, Air, Pressure. "Away" header. No countdown |
| 4.4 AlertScreen — full-screen "Stand up!" with pulse animation | done | 0.25 d | Pulses red/orange every 800ms. Exclamation circle + sitting duration + "Move to dismiss" hint |
| 4.5 Theme system — shared colors + fonts across all screens | done | 0.1 d | `Theme.h`: RGB565 palette, FreeFonts via extern declarations (not Free_Fonts.h — PlatformIO path issue) |
| 4.6 SLEEP mode — backlight off when absent >5 min | done | 0.1 d | `switchScreen()` controls backlight via LEDC PWM. Any presence → wake |
| 4.7 Backlight PWM — 2 levels (max 255, dim 128) + auto-dim at ≤50 lux | done | 0.1 d | LEDC ch0 @ 5kHz. Hysteresis: dim ≤50 lux, max ≥100 lux. 10s grace on boot. SUMMARY uses dim. |
| 4.8 Splash screen — cyan sweep bar + "Desk Guardian" title + version | done | 0.1 d | Replaces R/G/B smoke test. ~1.4s total. |
| 4.9 Non-blocking NTP — poll with 0ms timeout, retry next loop | done | 0.1 d | `tryNTPSync()`. No more 10s block on boot. |
| 4.10 Actionable suggestions — "Turn on fan", "Open a window" etc. | done | 0.1 d | Replaces raw status text ("Too hot"). Priority-based worst-first. |

---

### Phase 5 — Touch (Stage B)

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 5.1 Wire XPT2046 (shares SPI: SCK 12, MOSI 11, MISO 13). T_CS 42, T_IRQ 45. | todo | 0.25 d |  |
| 5.2 Add `PaulStoffregen/XPT2046_Touchscreen` lib | todo | 0.1 d |  |
| 5.3 Calibration routine — touch 4 corners → save to NVS | todo | 0.5 d |  |
| 5.4 Tap-to-dismiss alerts | todo | 0.25 d |  |
| 5.5 Settings screen — adjust thresholds via touch | todo | 0.5 d |  |
| 5.6 Swipe between home / detail / settings | todo | 0.25 d |  |

---

### Phase 6 — Hardening

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 6.1 Watchdog timer (esp_task_wdt) on main loop | todo | 0.25 d | Reset on hang |
| 6.2 Brown-out detector verified | todo | 0.1 d | Default fine, verify panic on under-voltage |
| 6.3 Crash log to NVS (last reset reason + last loop tick) | todo | 0.25 d |  |
| 6.4 Battery voltage read (if BQ24074 exposes ADC or via divider on Vbat) | todo | 0.5 d | Optional |
| 6.5 Low-power: dim backlight after N min idle | todo | 0.25 d | LEDC PWM on GPIO 14 |

---

## Module 2 — Shared schemas package

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| S.1 Create `packages/schemas` workspace package: `pnpm init`, name `@dsk/schemas`, TS + zod | done | 0.5 d | CJS output (NestJS compat). zod validation + TS types |
| S.2 Define `EnvTelemetry` schema (v1) | done | 0.25 d | `telemetry.ts` — all sensor fields + presence + sit_seconds |
| S.3 Define `Hello` + `Cmd.FactoryReset` + `Cmd.Config` + `Device` schemas | done | 0.25 d | `hello.ts`, `commands.ts`, `device.ts` + DTOs |
| S.4 Export TS types + MQTT topic builders | done | 0.25 d | `mqtt-topics.ts` — topic functions + subscription wildcards. JSON Schema export deferred (not needed yet) |

---

## Module 3 — ESP → RPi link (additive on top of Module 1)

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| 7.1 WiFi STA connect — non-blocking, lazy retry every 10s | done | 0.5 d | `wifi_manager.{h,cpp}`. No retry spam. Only logs connect/disconnect. |
| 7.2 NTP sync after WiFi up — non-blocking | done | 0.1 d | `tryNTPSync()` polls with 0ms timeout. Uses broker IP as primary NTP, pool.ntp.org fallback. |
| 7.3 MQTT client (PubSubClient), auto-reconnect 5s backoff | done | 0.5 d | `mqtt_manager.{h,cpp}`. 512-byte buffer. Static callback routing via singleton. |
| 7.4 Publish `dg/<node>/telemetry/env` every 10s, JSON matches schema | done | 0.5 d | ArduinoJson v7. Null for bad sensors. Verified against zod schema on server. |
| 7.5 Retained `dg/<node>/hello` on MQTT connect | done | 0.25 d | fw_version, chip info, MAC, IP, uptime. |
| 7.6 Subscribe `dg/<node>/cmd/factory_reset` — stub | wip | 0.25 d | Subscription + log. NVS wipe not yet implemented. |
| 7.7 Subscribe `dg/<node>/cmd/config` — stub | wip | 0.25 d | Subscription + log. NVS threshold update not yet implemented. |
| 7.8 BLE provisioning via `WiFiProv.h` — replaces hardcoded creds | todo | 1 d | Final step before shipping. Until then bench dev uses `secrets.h` |

---

## Module 4 — Raspberry Pi gateway infra

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| G.1 Pi 5 OS install — Bookworm 64-bit, NVMe boot, SSH | todo | 0.5 d | Pi on bench, deploy later |
| G.2 `infrastructure/mosquitto/` Docker compose + per-node ACL file template | done | 0.5 d | `docker-compose.yml` + `mosquitto.conf` + `acl` + `passwd` (generated). Dev: dg_server + node1 accounts |
| G.3 `infrastructure/postgres/` Docker compose with TimescaleDB + init SQL | done | 0.5 d | `init.sql`: devices table + telemetry hypertable + indexes. 90-day retention + 5-min rollup commented out |
| G.4 Wi-Fi AP: hostapd + dnsmasq on wlan1 (USB dongle), SSID DG-CENTER | wip | 1 d | AP broadcasting, DHCP serving 192.168.4.x. Static IP via systemd oneshot. `no-resolv` in dnsmasq. NetworkManager unmanage wlan1. See `docs/system/rpi-setup.md`. |
| G.5 `infrastructure/caddy/` reverse proxy: web + API + WS on single port | done | 0.5 d | Caddyfile: /api/* + /socket.io/* → :4000, else → :3000 |
| G.6 Grafana container (optional) pointing at Postgres | skip | 0.25 d | Skipped — keep stack minimal |

---

## Module 5 — NestJS backend (`apps/server`)

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| B.1 Scaffold `apps/server` with NestJS CLI | done | 0.5 d | NestJS 11, port 4000, unused app.controller/service removed by user |
| B.2 TypeORM setup + pg driver + data-source config | done | 0.5 d | `@nestjs/typeorm` + `typeorm` + `pg`. `env.config.ts` with `IEnvConfig` interface. `synchronize: false` — tables from init.sql |
| B.3 Entity definitions: DeviceEntity + TelemetryEntity | done | 0.5 d | `entities/` folder. Telemetry = hypertable (created by init.sql, not TypeORM). |
| B.4 MQTT subscriber (`mqtt` npm package) on `dg/+/telemetry/env` + `dg/+/hello` | done | 0.5 d | `mqtt/mqtt-subscriber.service.ts` — connects, subscribes, routes to telemetry/device services. Has `publish()` for cmd topics |
| B.5 Persist telemetry to hypertable + update device last_seen | done | 0.5 d | `telemetry/telemetry.service.ts` — zod validate → save row → update device → WS broadcast |
| B.6 Device registry REST: list / approve / rename / remove | done | 1 d | `devices/` module. Hello handler auto-creates pending device. Approve sets name + active. Remove = soft delete |
| B.7 WebSocket gateway (socket.io) pushing live telemetry to web | done | 0.5 d | `telemetry/telemetry.gateway.ts` — `/telemetry` namespace, `ServerToClientEvents` typed |
| B.8 Threshold config REST → publish to `dg/<node>/cmd/config` | done | 0.5 d | POST /api/devices/:id/config — validates via ConfigCmdSchema, publishes MQTT. forwardRef wiring done |
| B.9 BLE bridge for provisioning (Pi side) | todo | 1.5 d | Or skip and use phone BLE app |

---

## Module 6 — Next.js dashboard (`apps/web`)

Detailed build plan: `docs/system/dashboard.md`

| Step | Status | Est. | Notes |
| ---- | ------ | ---- | ----- |
| W.0 Foundation: socket.io client + hooks | done | 1 d | `lib/socket.ts` (typed client), `hooks/use-socket.tsx` (`useTelemetry`, `useAllTelemetry`). socket.io-client installed. |
| W.1 Live dashboard: device tiles + real-time WS + quality badges | todo | 1.5 d | Match firmware quality thresholds |
| W.2 Device management: list, approve pending, rename, remove | todo | 1 d |  |
| W.3 Threshold config: per-device form → MQTT push | todo | 0.5 d |  |
| W.4 History charts: CO2/temp/humidity/lux over 1h/24h/7d | todo | 1 d | Recharts |
| W.5 Polish: responsive, dark mode, loading, errors | todo | 0.5 d |  |

---

## Open decisions

| # | Question | Blocking | Default if not decided |
| - | -------- | -------- | ---------------------- |
| 1 | ~~Prisma vs TypeORM~~ → **TypeORM locked** | — | TypeORM (decided 2026-05-28) |
| 2 | BLE provisioning UX — phone app, web BLE in browser, or Pi-side BLE? | Module 3.8 & Module 5.9 | Phone app (Espressif BLE Prov) |
| 3 | ~~Grafana in or out?~~ → **Out** (decided 2026-05-28) | — | — |
| 4 | NTP source on isolated AP | Module 3.2 | RPi runs `chrony` serving local time |

---

## Pre-flight checks before each phase

Before any new wiring:
1. Power off ESP (unplug USB).
2. Cross-check pin assignment against `docs/system/README.md` §4.
3. Confirm 3V3 vs 5V on the sensor module before powering.
4. Common GND across all modules.

Before any firmware flash:
1. `pio device list` shows the CH343P serial port.
2. Close other terminals holding the serial port.
3. If auto-reset fails: hold BOOT, tap RESET.

---

## Project file map (current)

```
dsk-guard/
├── CLAUDE.md                       # project rules (loaded into every chat)
├── package.json                    # root scripts (turbo + pnpm)
├── pnpm-workspace.yaml             # apps/web, apps/server, packages/*
├── turbo.json                      # task graph
├── apps/
│   ├── web/                        # Next.js 16, Tailwind v4 (WS hooks done, dashboard pages todo)
│   │   ├── lib/socket.ts           # socket.io client (typed, /telemetry namespace)
│   │   ├── hooks/use-socket.tsx    # useTelemetry(nodeId), useAllTelemetry()
│   └── firmware/                   # PlatformIO, Arduino-ESP32
│       ├── platformio.ini
│       ├── include/secrets.h.example
│       ├── README.md
│       ├── test/
│       │   ├── ld2410s_uno_r4_test.ino  # UNO R4 WiFi test sketch for LD2410S
│       │   └── ld2410c_uno_r4_test.ino  # UNO R4 WiFi test sketch for LD2410C
│       └── src/
│           ├── main.cpp            # setup/loop, FSM driver, screen switching, backlight
│           ├── User_Setup.h        # TFT_eSPI config (ILI9488 + S3 pins, 40 MHz SPI)
│           ├── sensors/
│           │   ├── radar_types.h   # shared PresenceState (PRESENT/ABSENT) + RadarReading
│           │   ├── bh1750.{h,cpp}  # I2C lux sensor
│           │   ├── bme680.{h,cpp}  # I2C T/RH/P/VOC
│           │   ├── ld2410s.{h,cpp} # UART2 mmWave radar (ACTIVE — minimal frame 6E..62, OT2, 115200, 3.3V)
│           │   ├── ld2410c.{h,cpp} # UART2 mmWave radar (PARKED — hardware issue, excluded from build)
│           │   └── acd1200.{h,cpp} # UART1 CO2 (custom Aosong protocol)
│           ├── network/
│           │   ├── wifi_manager.{h,cpp}  # non-blocking WiFi STA, lazy retry 10s
│           │   └── mqtt_manager.{h,cpp}  # PubSubClient wrapper, auto-reconnect 5s
│           ├── logic/
│           │   └── ScreenFsm.{h,cpp} # 4-state screen FSM (ACTIVE/ALERT/SUMMARY/SLEEP), 2-state presence
│           └── ui/
│               ├── Screen.h        # base class (begin/update/dirty)
│               ├── Theme.h         # shared colors (RGB565), fonts, layout constants
│               ├── ActiveScreen.{h,cpp}   # countdown + env rows + radar dist (person present)
│               ├── AlertScreen.{h,cpp}    # "Stand up!" pulsing reminder
│               ├── SummaryScreen.{h,cpp}  # 3×2 env cards (person absent)
│               └── HomeScreen.{h,cpp}     # legacy card dashboard (replaced, can delete)
│   ├── server/                     # NestJS 11 backend (port 4000)
│   │   ├── src/
│   │   │   ├── main.ts             # bootstrap, port 4000, CORS
│   │   │   ├── app.module.ts       # root: ConfigModule + TypeORM + feature modules
│   │   │   ├── config/env.config.ts  # DB + MQTT env config + IEnvConfig interface
│   │   │   ├── entities/
│   │   │   │   ├── device.entity.ts    # devices table (TypeORM)
│   │   │   │   └── telemetry.entity.ts # telemetry hypertable (TypeORM mapping only)
│   │   │   ├── devices/
│   │   │   │   ├── devices.module.ts
│   │   │   │   ├── devices.service.ts  # CRUD + hello handler
│   │   │   │   └── devices.controller.ts # REST /api/devices
│   │   │   ├── telemetry/
│   │   │   │   ├── telemetry.module.ts
│   │   │   │   ├── telemetry.service.ts  # validate + persist + query
│   │   │   │   ├── telemetry.controller.ts # REST /api/telemetry/:nodeId
│   │   │   │   └── telemetry.gateway.ts   # WebSocket (socket.io /telemetry)
│   │   │   └── mqtt/
│   │   │       ├── mqtt.module.ts
│   │   │       └── mqtt-subscriber.service.ts # MQTT client, subscribe + publish
│   │   └── package.json
├── packages/
│   └── schemas/                    # @dsk/schemas (CJS, zod)
│       ├── src/
│       │   ├── index.ts            # barrel export
│       │   ├── telemetry.ts        # EnvTelemetry schema
│       │   ├── hello.ts            # Hello beacon schema
│       │   ├── commands.ts         # FactoryReset + Config cmd schemas
│       │   ├── device.ts           # Device + DTOs
│       │   └── mqtt-topics.ts      # topic builders + subscription wildcards
│       ├── tsconfig.json           # CJS output, moduleResolution: node
│       └── package.json
├── infrastructure/
│   ├── docker-compose.yml          # Mosquitto + Postgres/TimescaleDB + Caddy
│   ├── mosquitto/
│   │   ├── mosquitto.conf          # listeners, auth, persistence
│   │   ├── acl                     # per-node ACL (dg_server + node1)
│   │   ├── passwd                  # hashed passwords (generated)
│   │   └── README.md               # setup instructions
│   ├── postgres/
│   │   └── init.sql                # devices table + telemetry hypertable + indexes
│   └── caddy/
│       └── Caddyfile               # reverse proxy: API→4000, web→3000
├── docs/
│   ├── system/
│   │   ├── README.md               # full spec
│   │   ├── planning.md             # ← this file
│   │   ├── api.md                  # backend API reference (REST + WS + MQTT)
│   │   ├── dashboard.md            # web dashboard build plan (W.0–W.5)
│   │   └── rpi-setup.md            # RPi 5 AP + Docker setup guide (9 steps)
│   └── hardware/
│       ├── README.md               # datasheet index
│       └── wire.md                 # complete wiring reference (board pin labels)
```

---

## Memory pointers (auto-loaded)

If your chat agent has the project memory system:
- `user_role.md` — Huy's collaboration style
- `project_dskguard_stack.md` — locked tech decisions
- `MEMORY.md` — index
