# dsk-guard firmware

ESP32-S3 edge node for the Desk Guardian. Built on **PlatformIO + Arduino-ESP32**.

Hardware: MKE-K01 (ESP32-S3-WROOM-1 N16R8). 16 MB Flash, 8 MB OPI PSRAM, CH343P USB-UART, on-board RGB on GPIO 48.

Full pinout + wiring rules: `../../docs/system/README.md` §4. Build plan + status: `../../docs/system/planning.md`.

## Prereqs

1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (CLI) or the VS Code extension.
2. Install **CH343P USB-UART driver** on the host machine:
   - macOS: WCH driver from <https://www.wch.cn/downloads/CH343SER_MAC_ZIP.html>
   - Linux: kernel ≥ 5.4 ships `ch341`; for full speed use the WCH `ch343` driver from <https://github.com/WCHSoftGroup/ch343ser_linux>
   - Windows: WCH driver from <https://www.wch.cn/downloads/CH343SER_ZIP.html>
3. Plug the board in. Confirm a serial device appears:
   - macOS: `/dev/tty.wchusbserial*`
   - Linux: `/dev/ttyACM*` or `/dev/ttyUSB*`
   - Windows: `COMx`

## Build / flash / monitor

From this directory:

```bash
pio run                       # build
pio run --target upload       # flash
pio device monitor            # serial console @ 115200
```

Or combined:

```bash
pio run -t upload -t monitor
```

Hold **BOOT** while pressing **RESET** to force download mode if auto-reset fails.

## What Phase 0 does

- Cycles the on-board RGB on GPIO 48 through dim red → green → blue, one step per second.
- Prints chip info, flash + PSRAM size, free heap, and a heartbeat to serial every 10 s.
- Verifies: toolchain works, 16 MB flash recognised, 8 MB OPI PSRAM initialised, CH343P serial flows.

## Acceptance check

When you flash and open the monitor you should see:

```
=== dsk-guard firmware ===
version       : 0.0.1
chip model    : ESP32-S3 rev 0
cpu freq      : 240 MHz
flash size    : 16777216 bytes
psram size    : 8388608 bytes
psram free    : ~8385000 bytes
heap free     : ~340000 bytes
...
[OK] Phase 0 smoke test running. RGB cycle R→G→B.
```

If `psram size : 0` → PSRAM flags wrong in `platformio.ini`. Re-check `board_build.arduino.memory_type = qio_opi`.

## Layout

```
apps/firmware/
  platformio.ini          # build config
  src/main.cpp            # entry point
  include/                # headers
    secrets.h.example     # template for WiFi + MQTT creds (Phase 6+)
  lib/                    # local libs (none yet)
  test/                   # unit tests (none yet)
  .gitignore
  README.md
```

## Out of pnpm workspace

This project is intentionally **not** part of the pnpm workspace. Don't run `pnpm install` here; PlatformIO manages its own deps under `.pio/`.
