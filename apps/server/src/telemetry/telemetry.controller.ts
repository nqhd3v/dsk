import { Controller, Get, Param, Query } from '@nestjs/common';
import { TelemetryService } from './telemetry.service.js';
import { TelemetryEntity } from '../entities/telemetry.entity.js';

/** Map DB entity → EnvTelemetry wire shape (ts: unix seconds) */
function toEnvTelemetry(row: TelemetryEntity) {
  return {
    ts: Math.floor(new Date(row.time).getTime() / 1000),
    node_id: row.node_id,
    lux: row.lux,
    temp_c: row.temp_c,
    humidity: row.humidity,
    pressure_hpa: row.pressure_hpa,
    gas_ohm: row.gas_ohm,
    co2_ppm: row.co2_ppm,
    co2_preheating: row.co2_preheating,
    presence: row.presence,
    distance_cm: row.distance_cm,
    sit_seconds: row.sit_seconds,
    hcho_ppb: row.hcho_ppb ?? null,
    fw_version: row.fw_version,
  };
}

@Controller('api/telemetry')
export class TelemetryController {
  constructor(private readonly telemetryService: TelemetryService) {}

  @Get(':nodeId/recent')
  async getRecent(
    @Param('nodeId') nodeId: string,
    @Query('limit') limit?: string,
  ) {
    const n = Math.min(parseInt(limit ?? '100', 10) || 100, 1000);
    const rows = await this.telemetryService.getRecent(nodeId, n);
    return rows.map(toEnvTelemetry);
  }

  @Get(':nodeId/latest')
  async getLatest(@Param('nodeId') nodeId: string) {
    const row = await this.telemetryService.getLatest(nodeId);
    return row ? toEnvTelemetry(row) : null;
  }
}
