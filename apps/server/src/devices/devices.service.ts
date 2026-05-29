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
