# dsk-guard — Desk Guardian

IoT desk wellness monitoring system. Tracks environment + presence, reminds user to stand/hydrate/ventilate.

Full spec: `docs/system/README.md`. Build plan + status: `docs/system/planning.md`. Hardware datasheets: `docs/hardware/`.

## Architecture

Two-tier, **standalone-first**:

- **ESP32-S3 edge node** — sensor pod, TFT display, runs fully on its own (sense + reminders + local UI). Required.
- **Raspberry Pi 5 gateway** — Wi-Fi AP, MQTT broker, data store, web dashboard. **Optional add-on** for history + remote view + multi-node management.

## Monorepo Structure

```
apps/
  web/       Next.js 16 dashboard (browser UI)
  server/    NestJS backend (MQTT sub, WebSocket, REST, device registry)
  firmware/  ESP32-S3 PlatformIO project (Arduino-ESP32, C++)
packages/
  schemas/   MQTT payload JSON schemas + TS types (source of truth for all apps)
infrastructure/
  mosquitto/ MQTT broker config + ACL
  postgres/  Postgres + TimescaleDB docker compose + init scripts
  caddy/     Reverse proxy config
docs/
  system/    Full system spec + build plan
  hardware/  Sensor datasheets + wiring
```

## Tech Stack (locked)

| Layer         | Choice                                                                                |
| ------------- | ------------------------------------------------------------------------------------- |
| Firmware      | PlatformIO + Arduino-ESP32, TFT_eSPI                                                  |
| MQTT broker   | Mosquitto                                                                             |
| Database      | PostgreSQL + TimescaleDB extension (single store for registry + telemetry hypertable) |
| Backend       | NestJS (TypeScript) with `@nestjs/microservices` MQTT transport                       |
| Web           | Next.js 16.2.6 + React 19                                                             |
| Reverse proxy | Caddy                                                                                 |

ORM: TypeORM (not Prisma).
No InfluxDB. No Telegraf. No SQLite. No FastAPI.

## Package Manager & Build

- **pnpm workspaces** — JS packages only (`apps/web`, `apps/server`, `packages/*`)
- **Turborepo** — task runner (`pnpm dev`, `pnpm build`, `pnpm lint`)
- **firmware** excluded from pnpm — managed via PlatformIO CLI separately

```bash
pnpm install        # install all JS deps
pnpm dev            # run all apps in dev mode
pnpm build          # build all apps
```

## Key Protocols

| Protocol  | Used for                                                                                                   |
| --------- | ---------------------------------------------------------------------------------------------------------- |
| MQTT      | ESP32 → RPi telemetry (`dg/<node>/telemetry/env`)                                                          |
| WebSocket | NestJS → browser live updates                                                                              |
| BLE       | ESP32 provisioning (push Wi-Fi + MQTT creds) — added late, hardcoded creds in `secrets.h` during bench dev |
| I2C       | BH1750 (lux), BME680 (T/RH/VOC/pressure)                                                                   |
| UART      | ACD1200 CO2 @1200 baud, LD2410S radar @115200 baud (3.3V!, minimal frame 6E..62). LD2410C @256000 parked.  |
| SPI       | ILI9488 TFT display + XPT2046 touch                                                                        |

## MQTT Topics

```
dg/<node>/telemetry/env     sensor readings (JSON, 10s interval)
dg/<node>/hello             retained presence beacon
dg/<node>/cmd/factory_reset
dg/<node>/cmd/config        threshold push from server
```

## Presence Model (2-state)

Simple binary: PRESENT (sensor says someone AND smoothed distance ≤ 150 cm) or ABSENT. OT2 digital pin (GPIO 4) = instant presence ground truth when wired. Distance smoothed with 5-sample moving average. LD2410S minimal frame: `6E [state] [dist_lo] [dist_hi] 62` (5 bytes, factory default).

## Screen FSM Timings

- Present → countdown runs (45 min default)
- Away > 10 s → reset countdown
- Away > 60 s → SUMMARY screen
- Away > 5 min → SLEEP (backlight off)
- ALERT auto-dismiss on presence return or 30 s timeout

## Sensor Thresholds (defaults — stored in NVS on ESP, editable via web after RPi added)

- CO2 > 1000 ppm → ventilate
- VOC elevated → air quality alert
- Lux < 200 or > 400 lux → lighting adjust
- Sitting > 45 min → stand-up reminder
- No movement detected → hydration nudge

## Device Lifecycle (RPi mode)

Add: BLE pairing → creds pushed → ESP joins AP → publishes retained `hello` → user approves on dashboard → Postgres `devices` row + Mosquitto ACL created.

Remove: revoke Mosquitto user → drop ACL → delete `devices` row → publish `cmd/factory_reset` → ESP wipes NVS → back to BLE prov mode. Telemetry rows stay in Postgres.

Per-device MQTT creds (`node1`, `node2`, …) so revoke is isolated.

## Reserved pins on ESP32-S3 N16R8

GPIO 35/36/37 are wired to octal PSRAM — **do not use as GPIO**. Full pin map in `docs/system/README.md` §4.

## apps/web

Next.js 16 (App Router). **This is Next.js 16, NOT the Next.js you know from training data** — App Router APIs, file conventions, and config differ from older versions. Before writing or modifying any Next.js code, read the relevant guide under `node_modules/next/dist/docs/`. Heed deprecation notices.

Two main pages:

1. Live dashboard — sensor tiles, presence status, WebSocket updates
2. Device management — add/remove/name ESP nodes

## apps/server

NestJS (TypeScript). Responsibilities:

- Subscribe to Mosquitto MQTT topics (`@nestjs/microservices` MQTT transport)
- Validate telemetry against `packages/schemas`
- Write telemetry to Postgres/Timescale hypertable
- Manage Mosquitto ACL files for per-node creds
- Serve REST API + WebSocket (socket.io) to web app
- Publish config/cmd topics back to ESP nodes

## apps/firmware

PlatformIO + Arduino-ESP32 (C++). Do NOT use npm/pnpm here.

```bash
cd apps/firmware
pio run                # build
pio run --target upload  # flash
pio device monitor     # serial console
```

Display lib: TFT_eSPI. `User_Setup.h` is checked into the firmware repo.

## packages/schemas

JSON Schema + TypeScript types for MQTT payloads. Edit here first when changing payload format → then update firmware parser + server validator to match.

## Workflow

Plan → discuss → estimate → implement. Author prefers TypeScript stack and "right way over cheap-and-broken". Caveman mode is the default response style — see `docs/system/planning.md` for current phase status before suggesting work.
