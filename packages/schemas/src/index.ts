// @dsk/schemas — single source of truth for all MQTT + REST payload types

export {
  PresenceStateSchema,
  EnvTelemetrySchema,
  type PresenceState,
  type EnvTelemetry,
} from './telemetry';

export {
  HelloSchema,
  type Hello,
} from './hello';

export {
  FactoryResetCmdSchema,
  ConfigCmdSchema,
  type FactoryResetCmd,
  type ConfigCmd,
} from './commands';

export {
  DeviceStatusSchema,
  DeviceSchema,
  ApproveDeviceSchema,
  UpdateDeviceSchema,
  type DeviceStatus,
  type Device,
  type ApproveDevice,
  type UpdateDevice,
} from './device';

export {
  MQTT_TOPICS,
  MQTT_SUBSCRIPTIONS,
} from './mqtt-topics';
