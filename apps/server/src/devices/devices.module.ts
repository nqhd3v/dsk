import { Module, forwardRef } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { DeviceEntity } from '../entities/device.entity.js';
import { DevicesService } from './devices.service.js';
import { DevicesController } from './devices.controller.js';
import { MqttModule } from '../mqtt/mqtt.module.js';

@Module({
  imports: [
    TypeOrmModule.forFeature([DeviceEntity]),
    forwardRef(() => MqttModule),
  ],
  providers: [DevicesService],
  controllers: [DevicesController],
  exports: [DevicesService],
})
export class DevicesModule {}
