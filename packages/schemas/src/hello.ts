import { z } from 'zod';

// ── Hello beacon ─────────────────────────────────────────────────
// Published RETAINED by ESP32 on: dg/<node>/hello (on boot)
export const HelloSchema = z.object({
  node_id: z.string().min(1),
  fw_version: z.string(),
  chip_model: z.string(),
  chip_revision: z.number().int(),
  flash_size: z.number().int().describe('Flash size in bytes'),
  psram_size: z.number().int().describe('PSRAM size in bytes'),
  mac: z.string().describe('WiFi MAC address'),
  ip: z.string().describe('Assigned IP address on AP'),
  uptime_s: z.number().int().min(0).describe('Seconds since boot'),
  ts: z.number().int().describe('Unix timestamp (seconds)'),
});

export type Hello = z.infer<typeof HelloSchema>;
