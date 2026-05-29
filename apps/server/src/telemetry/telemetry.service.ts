import { EnvTelemetrySchema } from '@dsk/schemas';
import { Injectable, Logger } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { DeviceEntity } from '../entities/device.entity.js';
import { TelemetryEntity } from '../entities/telemetry.entity.js';
import { TelemetryGateway } from './telemetry.gateway.js';

@Injectable()
export class TelemetryService {
  private readonly logger = new Logger(TelemetryService.name);

  constructor(
    @InjectRepository(TelemetryEntity)
    private readonly telemetryRepo: Repository<TelemetryEntity>,
    @InjectRepository(DeviceEntity)
    private readonly deviceRepo: Repository<DeviceEntity>,
    private readonly gateway: TelemetryGateway,
  ) {}

  /**
   * Handle incoming MQTT telemetry message.
   * Validates with zod, persists to TimescaleDB, pushes to WebSocket clients.
   */
  async handleTelemetry(nodeId: string, rawPayload: unknown): Promise<void> {
    // Validate payload
    const result = EnvTelemetrySchema.safeParse(rawPayload);
    if (!result.success) {
      this.logger.warn(
        `Invalid telemetry from ${nodeId}: ${result.error.message}`,
      );
      return;
    }

    const data = result.data;

    // Persist to hypertable
    const row = this.telemetryRepo.create({
      time: new Date(data.ts * 1000),
      node_id: nodeId,
      lux: data.lux,
      temp_c: data.temp_c,
      humidity: data.humidity,
      pressure_hpa: data.pressure_hpa,
      gas_ohm: data.gas_ohm,
      co2_ppm: data.co2_ppm,
      co2_preheating: data.co2_preheating,
      presence: data.presence,
      distance_cm: data.distance_cm,
      sit_seconds: data.sit_seconds,
      hcho_ppb: data.hcho_ppb ?? null,
      fw_version: data.fw_version,
    });

    await this.telemetryRepo.save(row);

    // Update device last_seen
    await this.deviceRepo.update(
      { node_id: nodeId },
      {
        last_seen_at: new Date(),
        status: 'active',
        fw_version: data.fw_version,
      },
    );

    // Push to WebSocket clients
    this.gateway.broadcastTelemetry(nodeId, data);

    this.logger.debug(`Telemetry from ${nodeId} persisted + broadcast`);
  }

  /**
   * Query recent telemetry for a node.
   */
  async getRecent(nodeId: string, limit = 100): Promise<TelemetryEntity[]> {
    return this.telemetryRepo.find({
      where: { node_id: nodeId },
      order: { time: 'DESC' },
      take: limit,
    });
  }

  /**
   * Query latest single reading per node.
   */
  async getLatest(nodeId: string): Promise<TelemetryEntity | null> {
    return this.telemetryRepo.findOne({
      where: { node_id: nodeId },
      order: { time: 'DESC' },
    });
  }
}
