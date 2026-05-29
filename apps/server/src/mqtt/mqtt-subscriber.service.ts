import {
  Injectable,
  Logger,
  OnModuleInit,
  OnModuleDestroy,
} from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import * as mqtt from 'mqtt';
import { MQTT_SUBSCRIPTIONS } from '@dsk/schemas';
import { TelemetryService } from '../telemetry/telemetry.service.js';
import { DevicesService } from '../devices/devices.service.js';
import { IEnvConfig } from 'src/config/env.config.js';

@Injectable()
export class MqttSubscriberService implements OnModuleInit, OnModuleDestroy {
  private readonly logger = new Logger(MqttSubscriberService.name);
  private client!: mqtt.MqttClient;

  constructor(
    private readonly config: ConfigService,
    private readonly telemetryService: TelemetryService,
    private readonly devicesService: DevicesService,
  ) {}

  onModuleInit() {
    const mqttConfig: IEnvConfig['mqtt'] = this.config.get('mqtt')!;

    this.client = mqtt.connect(mqttConfig.url, {
      username: mqttConfig.username,
      password: mqttConfig.password,
      clientId: mqttConfig.clientId,
      clean: true,
      reconnectPeriod: 5000,
    });

    this.client.on('connect', () => {
      this.logger.log('Connected to MQTT broker');

      // Subscribe to all telemetry + hello topics
      this.client.subscribe(
        [MQTT_SUBSCRIPTIONS.allTelemetry, MQTT_SUBSCRIPTIONS.allHello],
        { qos: 1 },
        (err) => {
          if (err) {
            this.logger.error('MQTT subscribe failed', err);
          } else {
            this.logger.log(
              `Subscribed: ${MQTT_SUBSCRIPTIONS.allTelemetry}, ${MQTT_SUBSCRIPTIONS.allHello}`,
            );
          }
        },
      );
    });

    this.client.on('message', (topic, message) => {
      this.handleMessage(topic, message).catch((err) => {
        this.logger.error(`Error handling ${topic}: ${err}`);
      });
    });

    this.client.on('error', (err) => {
      this.logger.error(`MQTT error: ${err.message}`);
    });

    this.client.on('reconnect', () => {
      this.logger.warn('Reconnecting to MQTT broker...');
    });
  }

  async onModuleDestroy(): Promise<void> {
    if (this.client) {
      await this.client.endAsync();
      this.logger.log('MQTT client disconnected');
    }
  }

  private async handleMessage(topic: string, message: Buffer): Promise<void> {
    let payload: unknown;
    try {
      payload = JSON.parse(message.toString());
    } catch {
      this.logger.warn(`Non-JSON message on ${topic}`);
      return;
    }

    // Extract node_id from topic: dg/<nodeId>/...
    const parts = topic.split('/');
    if (parts.length < 3 || parts[0] !== 'dg') return;
    const nodeId = parts[1];

    if (topic.endsWith('/telemetry/env')) {
      await this.telemetryService.handleTelemetry(nodeId, payload);
    } else if (topic.endsWith('/hello')) {
      await this.devicesService.handleHello(nodeId, payload);
    }
  }

  /**
   * Publish a message to an MQTT topic (used for cmd/config, cmd/factory_reset).
   */
  async publish(topic: string, payload: object): Promise<void> {
    return new Promise((resolve, reject) => {
      this.client.publish(topic, JSON.stringify(payload), { qos: 1 }, (err) =>
        err ? reject(err) : resolve(),
      );
    });
  }
}
