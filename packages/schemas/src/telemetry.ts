import { z } from 'zod';

// ── Presence state (matches firmware radar_types.h) ──────────────
export const PresenceStateSchema = z.enum(['PRESENT', 'ABSENT']);
export type PresenceState = z.infer<typeof PresenceStateSchema>;

// ── Environment telemetry payload ────────────────────────────────
// Published by ESP32 on: dg/<node>/telemetry/env  (every 10s)
export const EnvTelemetrySchema = z.object({
  // Metadata
  ts: z.number().int().describe('Unix timestamp (seconds)'),
  node_id: z.string().min(1).describe('Device node identifier'),
  fw_version: z.string().describe('Firmware version string'),

  // BH1750 — ambient light
  lux: z.number().min(0).nullable().describe('Ambient light (lux), null if sensor error'),

  // BME680 — environment
  temp_c: z.number().nullable().describe('Temperature (Celsius)'),
  humidity: z.number().min(0).max(100).nullable().describe('Relative humidity (%)'),
  pressure_hpa: z.number().nullable().describe('Atmospheric pressure (hPa)'),
  gas_ohm: z.number().int().nullable().describe('BME680 gas resistance (ohm), raw VOC proxy'),

  // ACD1200 — CO2
  co2_ppm: z.number().int().min(0).nullable().describe('CO2 concentration (ppm), null if preheating or error'),
  co2_preheating: z.boolean().describe('True if ACD1200 still in 120s preheat'),

  // LD2410S — presence
  presence: PresenceStateSchema.describe('2-state: PRESENT or ABSENT'),
  distance_cm: z.number().int().min(0).nullable().describe('Smoothed radar distance (cm), null if absent'),
  sit_seconds: z.number().int().min(0).describe('Continuous sitting time (seconds)'),

  // HCHO — future
  hcho_ppb: z.number().int().min(0).nullable().optional().describe('Formaldehyde (ppb), null if not wired'),
});

export type EnvTelemetry = z.infer<typeof EnvTelemetrySchema>;
