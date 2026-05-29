import { HelloSchema } from '@dsk/schemas';
import { Injectable, Logger, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Not, Repository } from 'typeorm';
import { DeviceEntity } from '../entities/device.entity.js';

@Injectable()
export class DevicesService {
  private readonly logger = new Logger(DevicesService.name);

  constructor(
    @InjectRepository(DeviceEntity)
    private readonly repo: Repository<DeviceEntity>,
  ) {}

  /** List all non-removed devices */
  async findAll(): Promise<DeviceEntity[]> {
    return this.repo.find({
      where: { status: Not('removed') },
      order: { created_at: 'ASC' },
    });
  }

  /** Find by UUID */
  async findById(id: string): Promise<DeviceEntity | null> {
    return this.repo.findOne({ where: { id } });
  }

  /** Find by node_id */
  async findByNodeId(nodeId: string): Promise<DeviceEntity | null> {
    return this.repo.findOne({ where: { node_id: nodeId } });
  }

  /**
   * Handle MQTT hello beacon — create pending device if new,
   * update metadata if existing.
   */
  async handleHello(nodeId: string, rawPayload: unknown): Promise<void> {
    const result = HelloSchema.safeParse(rawPayload);
    if (!result.success) {
      this.logger.warn(`Invalid hello from ${nodeId}: ${result.error.message}`);
      return;
    }

    const data = result.data;
    let device = await this.findByNodeId(nodeId);

    if (!device) {
      // New device — create as pending
      device = this.repo.create({
        node_id: nodeId,
        name: nodeId, // default name = node_id until user renames
        status: 'pending',
        fw_version: data.fw_version,
        mac: data.mac,
        ip: data.ip,
        last_seen_at: new Date(),
      });
      await this.repo.save(device);
      this.logger.log(`New device registered: ${nodeId} (pending approval)`);
    } else {
      // Existing device — update metadata
      device.fw_version = data.fw_version;
      device.mac = data.mac;
      device.ip = data.ip;
      device.last_seen_at = new Date();
      if (device.status === 'offline') {
        device.status = 'active';
      }
      await this.repo.save(device);
      this.logger.debug(`Device hello updated: ${nodeId}`);
    }
  }

  /** Approve a pending device with a user-given name */
  async approve(id: string, name: string): Promise<DeviceEntity> {
    const device = await this.repo.findOne({ where: { id } });
    if (!device) throw new NotFoundException(`Device ${id} not found`);
    device.name = name;
    device.status = 'active';
    return this.repo.save(device);
  }

  /** Persist the last-pushed config thresholds (partial — only non-null fields overwritten) */
  async saveConfig(
    id: string,
    cfg: {
      cfg_sit_minutes: number | null;
      cfg_co2_max_ppm: number | null;
      cfg_lux_min: number | null;
      cfg_lux_max: number | null;
    },
  ): Promise<void> {
    const update: Partial<DeviceEntity> = {};
    if (cfg.cfg_sit_minutes !== null) update.cfg_sit_minutes = cfg.cfg_sit_minutes;
    if (cfg.cfg_co2_max_ppm !== null) update.cfg_co2_max_ppm = cfg.cfg_co2_max_ppm;
    if (cfg.cfg_lux_min !== null) update.cfg_lux_min = cfg.cfg_lux_min;
    if (cfg.cfg_lux_max !== null) update.cfg_lux_max = cfg.cfg_lux_max;
    if (Object.keys(update).length > 0) {
      await this.repo.update(id, update);
    }
  }

  /** Rename a device */
  async rename(id: string, name: string): Promise<DeviceEntity> {
    const device = await this.repo.findOne({ where: { id } });
    if (!device) throw new NotFoundException(`Device ${id} not found`);
    device.name = name;
    return this.repo.save(device);
  }

  /**
   * Remove a device — soft delete (status='removed').
   * Caller should also revoke MQTT creds + publish factory_reset cmd.
   */
  async remove(id: string): Promise<DeviceEntity> {
    const device = await this.repo.findOne({ where: { id } });
    if (!device) throw new NotFoundException(`Device ${id} not found`);
    device.status = 'removed';
    return this.repo.save(device);
  }
}
