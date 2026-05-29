# Desk Guardian — Raspberry Pi 5 Setup Guide

RPi 5 runs as Wi-Fi AP + Docker host for Mosquitto, Postgres/TimescaleDB, NestJS, Caddy.

---

## Prerequisites

- Raspberry Pi 5 with Bookworm 64-bit (NVMe boot recommended)
- SSH access enabled
- Docker + Docker Compose installed
- Repo cloned to Pi (or `infrastructure/` folder copied)

---

## Step 1 — OS + Docker

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker (official method)
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
# Log out + back in for group change

# Verify
docker --version
docker compose version
```

---

## Step 2 — Wi-Fi AP (hostapd + dnsmasq)

### 2.1 Install packages

```bash
sudo apt install -y hostapd dnsmasq
sudo systemctl stop hostapd dnsmasq
```

### 2.2 Disable NetworkManager for wlan0

```bash
sudo nmcli device set wlan0 managed no
```

### 2.3 Static IP for wlan0

Create `/etc/network/interfaces.d/wlan0`:

```
auto wlan0
iface wlan0 inet static
    address 192.168.4.1
    netmask 255.255.255.0
    network 192.168.4.0
```

### 2.4 Configure hostapd

Create `/etc/hostapd/hostapd.conf`:

```ini
interface=wlan0
driver=nl80211
ssid=DG-CENTER
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=CHANGE_THIS_PASSWORD
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
country_code=VN
ieee80211n=1
```

> Change `wpa_passphrase` to a real password. Must match firmware `secrets.h`.

### 2.5 Point hostapd to config

Edit `/etc/default/hostapd`:

```
DAEMON_CONF="/etc/hostapd/hostapd.conf"
```

### 2.6 Configure dnsmasq

Create `/etc/dnsmasq.d/dg-ap.conf`:

```ini
interface=wlan0
dhcp-range=192.168.4.10,192.168.4.50,255.255.255.0,24h
domain=local
address=/dg.local/192.168.4.1
```

ESP nodes get DHCP IPs 192.168.4.10–50. `dg.local` resolves to Pi.

### 2.7 Enable + unmask services

```bash
sudo systemctl unmask hostapd
sudo systemctl enable hostapd dnsmasq
```

---

## Step 3 — NTP for isolated network (chrony)

ESP nodes need time sync. Pi serves NTP even without upstream internet.

```bash
sudo apt install -y chrony
```

Add to `/etc/chrony/chrony.conf`:

```
allow 192.168.4.0/24
local stratum 10
```

```bash
sudo systemctl enable chrony
```

---

## Step 4 — Internet sharing (optional)

Only needed if ESP nodes or Pi need internet access (OTA updates, external NTP, etc.).

```bash
# Enable IP forwarding
echo "net.ipv4.ip_forward=1" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p

# NAT from wlan0 → eth0
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

# Persist iptables rules
sudo apt install -y iptables-persistent
# Answer "Yes" to save current rules
```

Skip this step if running isolated (no internet needed).

---

## Step 5 — Docker stack

```bash
cd /path/to/dsk-guard/infrastructure
docker compose up -d
```

Starts: Mosquitto (1883), Postgres/TimescaleDB (5432), Caddy (80/443).

Verify:

```bash
docker compose ps            # all 3 running
docker compose logs mosquitto # no errors
docker compose logs postgres  # "database system is ready"
```

---

## Step 6 — NestJS backend

```bash
cd /path/to/dsk-guard
pnpm install
pnpm build --filter server
pnpm start --filter server
```

Or run as systemd service for auto-start — see Step 8.

Verify: `curl http://localhost:4000/api/devices` → `[]`

---

## Step 7 — Reboot + verify

```bash
sudo reboot
```

After reboot, check:

```bash
# AP broadcasting
sudo systemctl status hostapd     # active (running)
sudo systemctl status dnsmasq     # active (running)
sudo systemctl status chrony      # active (running)

# Docker stack
docker compose -f /path/to/infrastructure/docker-compose.yml ps

# From phone/laptop: scan WiFi → DG-CENTER visible
# Connect → get IP 192.168.4.x
# ping 192.168.4.1 → success
# curl http://192.168.4.1/api/devices → []
```

---

## Step 8 — Auto-start NestJS (systemd)

Create `/etc/systemd/system/dg-server.service`:

```ini
[Unit]
Description=Desk Guardian NestJS Server
After=network.target docker.service
Wants=docker.service

[Service]
Type=simple
User=pi
WorkingDirectory=/path/to/dsk-guard
ExecStart=/usr/bin/node apps/server/dist/main.js
Restart=on-failure
RestartSec=5
Environment=NODE_ENV=production
Environment=DB_HOST=localhost
Environment=DB_PORT=5432
Environment=DB_NAME=deskguard
Environment=DB_USER=dg
Environment=DB_PASS=dg_dev_pass
Environment=MQTT_HOST=localhost
Environment=MQTT_PORT=1883
Environment=MQTT_USER=dg_server
Environment=MQTT_PASS=CHANGE_THIS

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable dg-server
sudo systemctl start dg-server
sudo systemctl status dg-server
```

---

## Step 9 — Update firmware secrets

In `apps/firmware/include/secrets.h`:

```cpp
#define DG_WIFI_SSID     "DG-CENTER"
#define DG_WIFI_PASSWORD "CHANGE_THIS_PASSWORD"  // match hostapd
#define DG_MQTT_HOST     "192.168.4.1"
#define DG_MQTT_PORT     1883
#define DG_MQTT_USER     "node1"
#define DG_MQTT_PASSWORD "CHANGE_THIS"           // match mosquitto passwd
#define DG_NODE_ID       "node1"
```

Flash ESP → should see in serial:

```
[WiFi] Connected to DG-CENTER. IP=192.168.4.10 RSSI=-45
[NTP] Synced: 2026-05-29 15:30:00
[MQTT] Connected!
[MQTT] Hello (retained) → dg/node1/hello
[MQTT] Telemetry 280 bytes → dg/node1/telemetry/env
```

---

## Network diagram

```
                    Internet (optional)
                         │
                       eth0
                    ┌────┴────┐
                    │  RPi 5  │ 192.168.4.1
                    │         │
                    │ hostapd │ ← AP: DG-CENTER
                    │ dnsmasq │ ← DHCP: .10–.50
                    │ chrony  │ ← NTP server
                    │         │
                    │ Docker: │
                    │  Mosquitto :1883
                    │  Postgres  :5432
                    │  Caddy     :80
                    │         │
                    │ NestJS  :4000
                    └────┬────┘
                       wlan0
                    ┌────┴────┐
              ┌─────┤ Wi-Fi   ├─────┐
              │     └─────────┘     │
        ┌─────┴─────┐        ┌─────┴─────┐
        │  ESP node1 │        │  ESP node2 │
        │ .4.10      │        │ .4.11      │
        └────────────┘        └────────────┘
```

---

## Troubleshooting

**hostapd fails to start:**
- Check `sudo journalctl -u hostapd -n 50`
- Common: wlan0 still managed by NetworkManager → `sudo nmcli device set wlan0 managed no`
- Common: wrong country_code → set to `VN`

**ESP can't connect to AP:**
- Verify SSID + password match exactly
- Check `sudo journalctl -u dnsmasq` for DHCP lease

**ESP connected but MQTT fails:**
- Check Mosquitto logs: `docker compose logs mosquitto`
- Verify username/password in `infrastructure/mosquitto/passwd`
- Test from Pi: `mosquitto_pub -h localhost -u node1 -P PASS -t test -m "hello"`

**NTP not syncing on ESP:**
- Check chrony: `chronyc clients` (should show ESP IP)
- Firmware uses `DG_MQTT_HOST` as primary NTP server (192.168.4.1)
