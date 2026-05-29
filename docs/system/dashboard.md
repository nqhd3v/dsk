# Desk Guardian — Web Dashboard Build Plan

Next.js 16 dashboard for monitoring ESP nodes. Connects to NestJS backend (port 4000) via REST + WebSocket.

**Important:** This is Next.js 16 — read `node_modules/next/dist/docs/` before writing any code. App Router APIs differ from older versions.

---

## Prerequisites

- NestJS backend running on port 4000 (Module 5 — done)
- Mosquitto + Postgres/TimescaleDB running (Module 4 — done)
- `@dsk/schemas` package built (Module 2 — done)
- At least one ESP node publishing telemetry (Module 3 — code written, pending flash test)

---

## Existing state

- `apps/web/` scaffolded with `create-next-app` (Next.js 16.2.6, React 19, Tailwind v4)
- shadcn v4 initialized (`components.json` exists, `shadcn` in deps)
- Default starter page still in `app/page.tsx`
- No API client, no socket.io client, no real pages yet

---

## Steps

### Phase 0 — Foundation (est. 1 day)

| # | Task | Notes |
|---|------|-------|
| 0.1 | Strip starter page — replace `app/page.tsx` with redirect to `/dashboard` | Remove Next.js boilerplate |
| 0.2 | Add shadcn components: `button`, `card`, `badge`, `progress`, `tabs`, `dialog`, `input`, `label`, `separator`, `tooltip` | `pnpm dlx shadcn@latest add <component>` |
| 0.3 | Install socket.io-client + `@dsk/schemas` | `pnpm add socket.io-client @dsk/schemas --filter web` |
| 0.4 | Create `lib/api.ts` — typed REST client wrapping `fetch()` for NestJS endpoints | Base URL from env var `NEXT_PUBLIC_API_URL` (default `http://localhost:4000`) |
| 0.5 | Create `lib/socket.ts` — socket.io client singleton, auto-reconnect, typed events | Connect to `ws://<host>:4000/telemetry` |
| 0.6 | Create `hooks/use-socket.ts` — React hook for subscribing to WS telemetry events | Returns live `EnvTelemetry` per nodeId |
| 0.7 | App layout shell: sidebar nav (Dashboard / Devices) + main content area | `app/layout.tsx`, no auth, local network only |

### Phase 1 — Live Dashboard (est. 1.5 days)

| # | Task | Notes |
|---|------|-------|
| 1.1 | `app/dashboard/page.tsx` — main dashboard page | Server component fetches device list, client components for live tiles |
| 1.2 | `DeviceTile` component — card per ESP node showing live sensor data | Subscribe to WS `telemetry` event per nodeId. Show: presence status, countdown, env quality (reuse same quality logic from firmware) |
| 1.3 | Env quality indicators — colored badges: Fresh/Good/Fair/Poor/Bad | Match firmware thresholds: CO2 <600 Fresh, <800 Good, <1000 Fair, <1500 Poor, else Bad. Same for light/temp/humidity/air |
| 1.4 | Connection status indicator — show if device is online (last_seen within 30s) | Compare `last_seen_at` vs now. Green dot = online, gray = offline |
| 1.5 | Auto-refresh — WS updates tile in real-time, REST fallback poll every 30s | Tile updates without page reload |
| 1.6 | Empty state — "No devices yet" with instructions to pair an ESP node | Show when device list is empty |

### Phase 2 — Device Management (est. 1 day)

| # | Task | Notes |
|---|------|-------|
| 2.1 | `app/devices/page.tsx` — device list page | Table: name, nodeId, status, firmware version, IP, last seen |
| 2.2 | Pending device approval flow — badge "Pending", approve button opens dialog | `POST /api/devices/:id/approve` with name input |
| 2.3 | Rename device — inline edit or dialog | `PATCH /api/devices/:id` |
| 2.4 | Remove device — confirmation dialog, then soft delete | `DELETE /api/devices/:id`. Show warning: "This will disconnect the device" |

### Phase 3 — Threshold Config (est. 0.5 day)

| # | Task | Notes |
|---|------|-------|
| 3.1 | `app/devices/[id]/page.tsx` — device detail + config page | Show current config values, form to update |
| 3.2 | Config form — sliders/inputs for each threshold | Fields from `ConfigCmdSchema`: co2_max_ppm, lux_min, lux_max, sit_minutes, hydrate_minutes, reset_delay_s, summary_delay_s, sleep_delay_s |
| 3.3 | Submit → `POST /api/devices/:id/config` → toast confirmation | Show published topic + payload in response |

### Phase 4 — History Charts (est. 1 day)

| # | Task | Notes |
|---|------|-------|
| 4.1 | Install Recharts | `pnpm add recharts --filter web` |
| 4.2 | `app/dashboard/[nodeId]/page.tsx` — device history page | Drill down from dashboard tile click |
| 4.3 | Time-series chart — CO2, temp, humidity, lux over time | Tabs: 1h / 24h / 7d. Fetch from `GET /api/telemetry/:nodeId/recent?limit=N` |
| 4.4 | Presence timeline — bar chart showing present/absent periods | Derived from `presence` field in telemetry rows |
| 4.5 | Sitting session log — list of sit sessions with duration | Derived from sit_seconds resets |

### Phase 5 — Polish (est. 0.5 day)

| # | Task | Notes |
|---|------|-------|
| 5.1 | Responsive layout — works on mobile (phone checking desk status) | Tailwind breakpoints, stack tiles vertically on mobile |
| 5.2 | Dark mode — match firmware dark theme aesthetic | Tailwind dark mode, default to dark |
| 5.3 | Loading states — skeleton cards while fetching | shadcn Skeleton component |
| 5.4 | Error handling — toast on API errors, retry button | Global error boundary + per-request error handling |
| 5.5 | Favicon + page titles | "Desk Guardian" branding |

---

## API endpoints used

| Page | Endpoints |
|------|-----------|
| Dashboard | `GET /api/devices`, `WS /telemetry` (live), `GET /api/telemetry/:nodeId/latest` (fallback) |
| Devices | `GET /api/devices`, `POST .../approve`, `PATCH .../`, `DELETE .../` |
| Device config | `POST /api/devices/:id/config` |
| History | `GET /api/telemetry/:nodeId/recent?limit=N` |

See `docs/system/api.md` for full API reference.

---

## Key files to create

```
apps/web/
├── app/
│   ├── layout.tsx              ← shell with sidebar nav
│   ├── page.tsx                ← redirect to /dashboard
│   ├── dashboard/
│   │   ├── page.tsx            ← live dashboard (device tiles)
│   │   └── [nodeId]/
│   │       └── page.tsx        ← device history charts
│   └── devices/
│       ├── page.tsx            ← device list + management
│       └── [id]/
│           └── page.tsx        ← device detail + config
├── components/
│   ├── device-tile.tsx         ← live sensor card per node
│   ├── env-quality.tsx         ← quality badge logic (shared)
│   ├── nav-sidebar.tsx         ← sidebar navigation
│   ├── sensor-chart.tsx        ← Recharts time-series wrapper
│   └── config-form.tsx         ← threshold config form
├── hooks/
│   └── use-socket.ts           ← WS subscription hook
└── lib/
    ├── api.ts                  ← typed REST client
    ├── socket.ts               ← socket.io singleton
    └── quality.ts              ← env quality assessment (match firmware thresholds)
```

---

## Env quality thresholds (match firmware)

```ts
// lib/quality.ts — must match firmware ActiveScreen thresholds
export function co2Quality(ppm: number) {
  if (ppm < 600)  return { label: 'Fresh', color: 'green' };
  if (ppm < 800)  return { label: 'Good',  color: 'green' };
  if (ppm < 1000) return { label: 'Fair',  color: 'yellow' };
  if (ppm < 1500) return { label: 'Poor',  color: 'orange' };
  return { label: 'Bad!', color: 'red' };
}
// Same pattern for lux, temp, humidity, air (gas_ohm)
```

---

## Notes

- No auth — local network only, single user
- Dark theme default — matches firmware dark UI aesthetic
- All env quality logic must match firmware thresholds exactly. Source of truth: `apps/firmware/src/ui/ActiveScreen.cpp` quality helpers
- WebSocket updates are real-time (~10s per node). REST is fallback only
- Next.js 16 App Router — read `node_modules/next/dist/docs/` before writing code
