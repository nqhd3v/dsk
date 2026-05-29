// ── MQTT topic patterns ──────────────────────────────────────────
// Single source of truth for topic strings used by ESP + server.

export const MQTT_TOPICS = {
  /** ESP → server: sensor readings (10s interval) */
  telemetryEnv: (nodeId: string) => `dg/${nodeId}/telemetry/env` as const,

  /** ESP → server: retained presence beacon (on boot) */
  hello: (nodeId: string) => `dg/${nodeId}/hello` as const,

  /** Server → ESP: factory reset command */
  cmdFactoryReset: (nodeId: string) => `dg/${nodeId}/cmd/factory_reset` as const,

  /** Server → ESP: threshold config push */
  cmdConfig: (nodeId: string) => `dg/${nodeId}/cmd/config` as const,
} as const;

// Wildcard patterns for server subscriptions
export const MQTT_SUBSCRIPTIONS = {
  allTelemetry: 'dg/+/telemetry/env',
  allHello: 'dg/+/hello',
} as const;
