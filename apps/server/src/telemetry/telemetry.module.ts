import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { TelemetryEntity } from '../entities/telemetry.entity.js';
import { DeviceEntity } from '../entities/device.entity.js';
import { TelemetryService } from './telemetry.service.js';
import { TelemetryController } from './telemetry.controller.js';
import { TelemetryGateway } from './telemetry.gateway.js';

@Module({
  imports: [TypeOrmModule.forFeature([TelemetryEntity, DeviceEntity])],
  providers: [TelemetryService, TelemetryGateway],
  controllers: [TelemetryController],
  exports: [TelemetryService],
})
export class TelemetryModule {}
