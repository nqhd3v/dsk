import {
  Controller,
  Get,
  Post,
  Patch,
  Delete,
  Param,
  Body,
  NotFoundException,
} from '@nestjs/common';
import { DevicesService } from './devices.service.js';
import { MqttSubscriberService } from '../mqtt/mqtt-subscriber.service.js';
import {
  ApproveDeviceSchema,
  ConfigCmdSchema,
  UpdateDeviceSchema,
  MQTT_TOPICS,
} from '@dsk/schemas';

@Controller('api/devices')
export class DevicesController {
  constructor(
    private readonly devicesService: DevicesService,
    private readonly mqttService: MqttSubscriberService,
  ) {}

  @Get()
  async list() {
    return this.devicesService.findAll();
  }

  @Post(':id/approve')
  async approve(@Param('id') id: string, @Body() body: unknown) {
    const parsed = ApproveDeviceSchema.parse(body);
    return this.devicesService.approve(id, parsed.name);
  }

  @Post(':id/config')
  async pushConfig(@Param('id') id: string, @Body() body: object) {
    // Look up device to get node_id
    const device = await this.devicesService.findById(id);
    if (!device) throw new NotFoundException(`Device ${id} not found`);

    // Validate + add timestamp
    const payload = ConfigCmdSchema.parse({
      ...body,
      ts: Math.floor(Date.now() / 1000),
    });

    // Publish to MQTT
    const topic = MQTT_TOPICS.cmdConfig(device.node_id);
    await this.mqttService.publish(topic, payload);

    // Persist pushed config so dashboard can read it back
    await this.devicesService.saveConfig(id, {
      cfg_sit_minutes: payload.sit_minutes ?? null,
      cfg_co2_max_ppm: payload.co2_max_ppm ?? null,
      cfg_lux_min: payload.lux_min ?? null,
      cfg_lux_max: payload.lux_max ?? null,
      cfg_presence_range_cm: payload.presence_range_cm ?? null,
    });

    return { topic, payload };
  }

  @Patch(':id')
  async update(@Param('id') id: string, @Body() body: unknown) {
    const parsed = UpdateDeviceSchema.parse(body);
    if (parsed.name) {
      return this.devicesService.rename(id, parsed.name);
    }
    return this.devicesService.findByNodeId(id);
  }

  @Delete(':id')
  async remove(@Param('id') id: string) {
    return this.devicesService.remove(id);
  }
}
