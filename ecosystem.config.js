// PM2 ecosystem config — dsk-guard
// Deploy on Raspberry Pi 5
// Docs: https://pm2.keymetrics.io/docs/usage/application-declaration/

module.exports = {
  apps: [
    {
      name: "dg-server",
      cwd: "./apps/server",
      script: "node",
      args: "dist/main.js",
      interpreter: "none",
      instances: 1,
      autorestart: true,
      watch: false,
      max_memory_restart: "300M",
      env: {
        NODE_ENV: "production",
        PORT: 3001,
      },
    },
  ],
};
