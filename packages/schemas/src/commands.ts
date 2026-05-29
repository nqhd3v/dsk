import { z } from 'zod';

// ── cmd/factory_reset ────────────────────────────────────────────
// Published by server on: dg/<node>/cmd/factory_reset
// ESP receives → wipes NVS → reboots → back to BLE prov mode
export const FactoryResetCmdSchema = z.object({
  reason: z.string().optional().describe('Human-readable reason for reset'),
  ts: z.number().int().describe('Unix timestamp (seconds)'),
});

export type FactoryResetCmd = z.infer<typeof FactoryResetCmdSchema>;

// ── cmd/config ───────────────────────────────────────────────────
// Published by server on: dg/<node>/cmd/config
// ESP receives → updates thresholds in NVS → acks via hello
export const ConfigCmdSchema = z.object({
  ts: z.number().int().describe('Unix timestamp (seconds)'),

  // All optional — only send fields that changed
  co2_max_ppm: z.number().int().min(400).max(5000).optional(),
  lux_min: z.number().min(0).optional(),
  lux_max: z.number().min(0).optional(),
  sit_minutes: z.number().int().min(1).max(180).optional(),
  hydrate_minutes: z.number().int().min(1).max(180).optional(),
  reset_delay_s: z.number().int().min(5).max(300).optional(),
  summary_delay_s: z.number().int().min(10).max(600).optional(),
  sleep_delay_s: z.number().int().min(60).max(3600).optional(),
});

export type ConfigCmd = z.infer<typeof ConfigCmdSchema>;
