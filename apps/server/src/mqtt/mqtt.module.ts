import { Module, forwardRef } from '@nestjs/common';
import { MqttSubscriberService } from './mqtt-subscriber.service.js';
import { TelemetryModule } from '../telemetry/telemetry.module.js';
import { DevicesModule } from '../devices/devices.module.js';

@Module({
  imports: [TelemetryModule, forwardRef(() => DevicesModule)],
  providers: [MqttSubscriberService],
  exports: [MqttSubscriberService],
})
export class MqttModule {}
