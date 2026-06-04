import { z } from 'zod';

// ── Device status (server-side) ──────────────────────────────────
export const DeviceStatusSchema = z.enum([
  'pending',     // hello received, user hasn't approved yet
  'active',      // approved + publishing telemetry
  'offline',     // no hello/telemetry for >60s
  'removed',     // soft-deleted, creds revoked
]);

export type DeviceStatus = z.infer<typeof DeviceStatusSchema>;

// ── Device record (Postgres devices table) ───────────────────────
export const DeviceSchema = z.object({
  id: z.string().uuid(),
  node_id: z.string().min(1).describe('MQTT node identifier (node1, node2, ...)'),
  name: z.string().min(1).describe('User-given display name'),
  status: DeviceStatusSchema,
  fw_version: z.string().nullable(),
  mac: z.string().nullable(),
  ip: z.string().nullable(),
  last_seen_at: z.string().datetime().nullable(),
  created_at: z.string().datetime(),
  updated_at: z.string().datetime(),
  // Last-pushed config thresholds (null = never pushed, use device defaults)
  cfg_sit_minutes: z.number().int().nullable(),
  cfg_co2_max_ppm: z.number().int().nullable(),
  cfg_lux_min: z.number().nullable(),
  cfg_lux_max: z.number().nullable(),
  cfg_presence_range_cm: z.number().int().nullable(),
});

export type Device = z.infer<typeof DeviceSchema>;

// ── Create/update DTOs ───────────────────────────────────────────
export const ApproveDeviceSchema = z.object({
  name: z.string().min(1).max(64),
});

export type ApproveDevice = z.infer<typeof ApproveDeviceSchema>;

export const UpdateDeviceSchema = z.object({
  name: z.string().min(1).max(64).optional(),
});

export type UpdateDevice = z.infer<typeof UpdateDeviceSchema>;
