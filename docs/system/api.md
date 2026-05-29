# dsk-guard — Backend API Reference

NestJS server running on port **4000**. Base URL: `http://<rpi-ip>:4000` (or via Caddy on `:80`).

---

## REST API

### Devices — `/api/devices`

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/devices` | List all non-removed devices (ordered by `created_at` ASC) |
| `POST` | `/api/devices/:id/approve` | Approve a pending device — sets status to `active` |
| `POST` | `/api/devices/:id/config` | Push threshold config to device via MQTT |
| `PATCH` | `/api/devices/:id` | Update device (rename) |
| `DELETE` | `/api/devices/:id` | Soft-delete device (status → `removed`) |

#### `GET /api/devices`

Returns all devices where `status != 'removed'`.

**Response:** `DeviceEntity[]`

```json
[
  {
    "id": "uuid",
    "node_id": "node1",
    "name": "My Desk",
    "status": "active",
    "fw_version": "0.1.0",
    "mac": "AA:BB:CC:DD:EE:FF",
    "ip": "192.168.4.2",
    "last_seen_at": "2026-05-28T12:00:00.000Z",
    "created_at": "2026-05-28T10:00:00.000Z"
  }
]
```

#### `POST /api/devices/:id/approve`

Approve a pending device and assign a user-friendly name.

**Body:**

```json
{ "name": "My Desk" }
```

**Validation:** `ApproveDeviceSchema` (zod) — `name` required, string.

#### `POST /api/devices/:id/config`

Push threshold configuration to an ESP node. Server looks up `node_id` from device UUID, validates body with `ConfigCmdSchema`, adds `ts`, publishes to `dg/<node_id>/cmd/config` via MQTT.

**Body** (all fields optional — send only changed values):

```json
{
  "co2_max_ppm": 1000,
  "lux_min": 200,
  "lux_max": 400,
  "sit_minutes": 45,
  "hydrate_minutes": 60,
  "reset_delay_s": 10,
  "summary_delay_s": 60,
  "sleep_delay_s": 300
}
```

**Validation:** `ConfigCmdSchema` (zod).

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `co2_max_ppm` | int | 400–5000 | CO2 alert threshold |
| `lux_min` | number | ≥ 0 | Low light warning |
| `lux_max` | number | ≥ 0 | High light warning |
| `sit_minutes` | int | 1–180 | Stand-up reminder interval |
| `hydrate_minutes` | int | 1–180 | Hydration reminder interval |
| `reset_delay_s` | int | 5–300 | Away seconds before countdown reset |
| `summary_delay_s` | int | 10–600 | Away seconds before summary screen |
| `sleep_delay_s` | int | 60–3600 | Away seconds before sleep mode |

**Response:**

```json
{
  "topic": "dg/node1/cmd/config",
  "payload": { "ts": 1716912000, "sit_minutes": 30 }
}
```

#### `PATCH /api/devices/:id`

Rename a device.

**Body:**

```json
{ "name": "New Name" }
```

**Validation:** `UpdateDeviceSchema` (zod).

#### `DELETE /api/devices/:id`

Soft-delete — sets `status = 'removed'`. Device disappears from list but telemetry rows stay in Postgres.

> **Note:** Caller should also revoke Mosquitto creds + publish `cmd/factory_reset`. Not yet automated.

---

### Telemetry — `/api/telemetry`

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/telemetry/:nodeId/recent` | Recent readings for a node |
| `GET` | `/api/telemetry/:nodeId/latest` | Single latest reading |

#### `GET /api/telemetry/:nodeId/recent`

**Query params:**

| Param | Default | Max | Description |
|-------|---------|-----|-------------|
| `limit` | 100 | 1000 | Number of rows to return |

**Response:** `TelemetryEntity[]` ordered by `time DESC`.

#### `GET /api/telemetry/:nodeId/latest`

**Response:** single `TelemetryEntity` or `null`.

**TelemetryEntity shape:**

```json
{
  "time": "2026-05-28T12:00:00.000Z",
  "node_id": "node1",
  "lux": 320.5,
  "temp_c": 26.3,
  "humidity": 55.2,
  "pressure_hpa": 1013.25,
  "gas_ohm": 150000,
  "co2_ppm": 680,
  "co2_preheating": false,
  "presence": true,
  "distance_cm": 85,
  "sit_seconds": 1200,
  "hcho_ppb": null,
  "fw_version": "0.1.0"
}
```

---

## WebSocket

### Namespace: `/telemetry`

Transport: socket.io. Connect to `ws://<host>:4000/telemetry`.

**Server → Client events:**

| Event | Payload | Description |
|-------|---------|-------------|
| `telemetry` | `{ nodeId: string, data: EnvTelemetry }` | Live sensor reading broadcast (every ~10s per node) |

Events emit to all connected clients AND to a room named `<nodeId>` (clients can join a room to filter by device).

---

## MQTT Topics (internal)

Handled by `MqttSubscriberService`. Not exposed to web clients directly.

### Subscribed (server listens)

| Topic | Source | Handler |
|-------|--------|---------|
| `dg/+/telemetry/env` | ESP node | `TelemetryService.handleTelemetry()` — validate, persist, broadcast WS |
| `dg/+/hello` | ESP node (retained) | `DevicesService.handleHello()` — auto-create pending device or update metadata |

### Published (server sends)

| Topic | Trigger | Payload |
|-------|---------|---------|
| `dg/<node>/cmd/config` | `POST /api/devices/:id/config` | `ConfigCmd` (threshold overrides + ts) |
| `dg/<node>/cmd/factory_reset` | Manual (not yet wired to REST) | `FactoryResetCmd` (reason + ts) |

---

## Authentication

None — local network only. No auth middleware. Mosquitto uses per-node username/password (see `infrastructure/mosquitto/passwd`).

---

## Error Handling

- Zod validation failures → 400 Bad Request (NestJS default exception filter)
- Device not found → 404 `NotFoundException`
- Invalid MQTT payloads → logged + silently dropped (no crash)
