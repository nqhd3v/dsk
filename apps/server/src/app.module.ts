import { Module } from '@nestjs/common';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { TypeOrmModule } from '@nestjs/typeorm';
import envConfig from './config/env.config.js';
import { DeviceEntity } from './entities/device.entity.js';
import { TelemetryEntity } from './entities/telemetry.entity.js';
import { TelemetryModule } from './telemetry/telemetry.module.js';
import { DevicesModule } from './devices/devices.module.js';
import { MqttModule } from './mqtt/mqtt.module.js';

@Module({
  imports: [
    // Config
    ConfigModule.forRoot({
      isGlobal: true,
      load: [envConfig],
    }),

    // Database
    TypeOrmModule.forRootAsync({
      inject: [ConfigService],
      useFactory: (config: ConfigService) => ({
        type: 'postgres' as const,
        host: config.get<string>('database.host'),
        port: config.get<number>('database.port'),
        username: config.get<string>('database.username'),
        password: config.get<string>('database.password'),
        database: config.get<string>('database.database'),
        entities: [DeviceEntity, TelemetryEntity],
        // Do NOT use synchronize in prod — tables created by init.sql
        synchronize: false,
        logging: process.env.NODE_ENV !== 'production',
      }),
    }),

    // Feature modules
    TelemetryModule,
    DevicesModule,
    MqttModule,
  ],
})
export class AppModule {}
