export default (): IEnvConfig => ({
  port: parseInt(process.env.PORT ?? '3001', 10),

  database: {
    host: process.env.DB_HOST ?? 'localhost',
    port: parseInt(process.env.DB_PORT ?? '5432', 10),
    username: process.env.DB_USER ?? 'dg',
    password: process.env.DB_PASS ?? 'dg_dev_pass',
    database: process.env.DB_NAME ?? 'deskguard',
  },

  mqtt: {
    url: process.env.MQTT_URL ?? 'mqtt://localhost:1883',
    username: process.env.MQTT_USER ?? 'dg_server',
    password: process.env.MQTT_PASS ?? 'server_dev_pass',
    clientId: process.env.MQTT_CLIENT_ID ?? 'dg-server',
  },
});

export interface IEnvConfig {
  port: number;

  database: {
    host: string;
    port: number;
    username: string;
    password: string;
    database: string;
  };

  mqtt: {
    url: string;
    username: string;
    password: string;
    clientId: string;
  };
}
