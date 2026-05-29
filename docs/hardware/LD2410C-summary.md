````markdown
# HLK-LD2410C Human Presence Sensor: Developer & AI Integration Guide

## 1. Sensor Overview

The **HLK-LD2410C** is a highly sensitive 24GHz FMCW (Frequency-Modulated Continuous Wave) human presence sensing module.

Unlike basic PIR sensors, it can accurately detect:

- Moving humans
- Micro-moving (fretting) humans
- Standing humans
- Sitting humans
- Lying humans

### Key Features

- **Dimensions:** 16mm × 22mm
- **Detection Range:** Up to 5 meters
- **Distance Gates:** 8 configurable gates
- **Coverage Angle:** ±60°
- **Environmental Resilience:**
  - Unaffected by temperature
  - Unaffected by brightness changes
  - Unaffected by humidity
  - Can penetrate non-metallic shells

---

## 2. Hardware Specifications & Wiring

To interface with the module correctly, ensure your power supply and logic levels match the following specifications.

### Electrical Specifications

| Parameter                 | Value            |
| ------------------------- | ---------------- |
| Operating Frequency       | 24GHz ~ 24.25GHz |
| Power Supply (VCC)        | 5V ~ 12V DC      |
| Recommended Voltage       | 5V               |
| Required Current          | > 200mA          |
| Average Operating Current | 79mA             |
| Logic Level (IO & UART)   | 3.3V             |
| Operating Temperature     | -40℃ ~ 85℃       |

---

### Pinout

The module uses a 5-pin interface with a 2.54mm pitch (0.9mm hole diameter).

| Pin | Name    | Description                  |
| --- | ------- | ---------------------------- |
| 1   | UART_Tx | Serial transmit (3.3V logic) |
| 2   | UART_Rx | Serial receive (3.3V logic)  |
| 3   | OUT     | Presence detection output    |
| 4   | GND     | Ground                       |
| 5   | VCC     | Power input (5V ~ 12V)       |

#### OUT Pin Behavior

By default:

- **High (3.3V)** → Human detected
- **Low (0V)** → No human detected

> Note: Logic levels can be inverted via serial commands.

---

## 3. Core Configuration Parameters

The radar operates using configurable parameters stored in non-volatile memory.

### Distance Gates & Resolution

The detection range is divided into up to **8 distance gates**.

Supported resolutions:

- `0.75m` (default)
- `0.2m`

---

### Maximum Distance Gate

Defines the furthest gate monitored for:

- Motion detection
- Static presence detection

Supported range:

- `1 ~ 8`

---

### Sensitivity (0–100)

Sensitivity is configured independently for:

- Motion detection
- Static detection

for each distance gate.

Rules:

- A target is detected only if its energy exceeds the gate sensitivity.
- Setting sensitivity to `100` effectively blinds that gate.

---

### Unoccupied Duration (Delay)

Defines how long the radar remains in an occupied state after a person leaves.

- Range: `0 ~ 65535 seconds`

---

## 4. Advanced Features

### Light Sensing Auxiliary Control

The module includes a photo diode.

The `OUT` pin can be configured to trigger only when:

- Human presence is detected
- Ambient light is below a configured threshold

Example:

- Turn on lights only when:
  - someone is present
  - the room is dark

---

### Background Noise Auto-Tuning

The module can automatically calculate optimal sensitivity values.

Procedure:

1. Ensure the room is empty
2. Send the auto-tuning command
3. The radar samples environmental noise for 10 seconds
4. Sensitivity values are automatically adjusted

---

### Bluetooth Configuration

The module supports Bluetooth configuration.

| Parameter        | Value           |
| ---------------- | --------------- |
| Default Password | `HiLink`        |
| Case Sensitive   | Yes             |
| Mobile App       | `HLKRadarTools` |

Supported features:

- Wireless configuration
- OTA firmware updates

---

## 5. Serial Communication Protocol (For Devs & AI)

The module communicates over UART using **Little-Endian** format.

### Default UART Settings

| Parameter | Value  |
| --------- | ------ |
| Baud Rate | 256000 |
| Data Bits | 8      |
| Stop Bits | 1      |
| Parity    | None   |

---

## 5.1 Control Command Structure

To modify settings, follow this sequence:

1. Enable configuration mode
2. Send configuration command(s)
3. End configuration mode

---

### Command Frame Format

```text
Header (FD FC FB FA)
+ Length (2 bytes)
+ Command Word (2 bytes)
+ Command Value (N bytes)
+ End (04 03 02 01)
```

---

### Enable Configuration

| Field   | Value    |
| ------- | -------- |
| Command | `0x00FF` |
| Value   | `0x0001` |

Must be sent before changing parameters.

---

### End Configuration

| Field   | Value    |
| ------- | -------- |
| Command | `0x00FE` |

---

### Key Command Words

| Command  | Description                            |
| -------- | -------------------------------------- |
| `0x0060` | Set max distance & unoccupied duration |
| `0x0061` | Read current parameters                |
| `0x0064` | Set distance gate sensitivity          |
| `0x00AA` | Set distance resolution                |
| `0x000B` | Start background noise auto-tuning     |

#### Distance Resolution Values

| Value    | Resolution |
| -------- | ---------- |
| `0x0000` | 0.75m      |
| `0x0001` | 0.2m       |

#### Set All Gates at Once

Use distance gate:

```text
0xFFFF
```

---

## 5.2 Radar Data Streaming (Uplink Format)

Outside configuration mode, the radar continuously streams target data.

### Stream Frame Format

```text
Header (F4 F3 F2 F1)
+ Length (2 bytes)
+ Data Type (1 byte)
+ Target Data
+ End (F8 F7 F6 F5)
```

---

### Data Type Values

| Value  | Meaning                  |
| ------ | ------------------------ |
| `0x02` | Basic target information |

---

### Basic Target Data Payload

Contains:

- Target Status (1 byte)
- Motion Distance (2 bytes)
- Motion Energy (1 byte)
- Static Distance (2 bytes)
- Static Energy (1 byte)
- Detection Distance (2 bytes)

---

### Target Status Values

| Value  | Meaning             |
| ------ | ------------------- |
| `0x00` | No target           |
| `0x01` | Moving              |
| `0x02` | Stationary          |
| `0x03` | Moving & stationary |

---

## 5.3 Engineering Mode

Engineering Mode provides detailed per-gate energy values.

### Enable Command

| Command  | Value                   |
| -------- | ----------------------- |
| `0x0062` | Enable Engineering Mode |

---

### Engineering Mode Features

In Engineering Mode:

- Stream Data Type changes to `0x01`
- Real-time energy values are provided for every gate
- Both motion and stationary energy values are included

This allows developers to:

- Visualize energy peaks
- Tune sensitivity precisely
- Debug false positives
- Analyze detection behavior

---

## 6. Installation & Radome (Enclosure) Guidelines

### Mounting Recommendations

| Mount Type    | Recommended Height |
| ------------- | ------------------ |
| Ceiling Mount | 2.6m ~ 3m          |
| Wall Mount    | 1.5m ~ 2m          |

---

### Environmental Recommendations

Ensure:

- The radar antenna is firmly mounted
- The sensor does not vibrate

Avoid:

- Large metallic reflectors
- Swinging curtains
- Rotating fans
- Continuously moving non-human objects

---

## 6.1 Enclosure (Radome) Design

To hide the sensor inside a product:

- Use non-metallic materials
- Use non-conductive materials

---

### Distance from Antenna

Ideal internal clearance should be an integer multiple of the half wavelength in air.

Examples:

- `6.2mm`
- `12.4mm`

---

### Material Thickness

The ideal shell thickness should match an integer multiple of the half wavelength inside the enclosure material.

Example:

| Material    | Dielectric Constant | Ideal Thickness |
| ----------- | ------------------- | --------------- |
| ABS Plastic | ~2.5                | ~3.92mm         |
````
