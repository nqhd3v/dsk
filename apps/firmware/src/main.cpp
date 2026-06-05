// dsk-guard — Main (Module 3: sensors + FSM + WiFi + MQTT)
// Target: MKE-K01 (ESP32-S3-WROOM-1 N16R8) + ILI9488 TFT
//
// Boot: serial → backlight → TFT init → splash → I2C → sensors → FSM → WiFi → NTP → MQTT
// Loop: poll radar, read sensors @2s, run FSM, render @50Hz, heartbeat @10s, MQTT publish @10s.
//
// Screen modes:
//   ACTIVE  — countdown timer + env rows (person present)
//   ALERT   — full-screen "Stand up!" (sitting threshold exceeded)
//   SUMMARY — 3×2 env cards (absent 1 min)
//   SLEEP   — backlight off (absent 10 min)

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

#include "sensors/bh1750.h"
#include "sensors/bme680.h"
#include "sensors/ld2450.h"   // active radar (LD2450, 5V, 256000 baud, multi-target)
// #include "sensors/ld2410s.h"  // parked — replaced by LD2450, kept for future
// #include "sensors/ld2410c.h"  // parked — hardware issue, kept for future
#include "sensors/acd1200.h"

#include "logic/ScreenFsm.h"
#include "ui/ActiveScreen.h"
#include "ui/AlertScreen.h"
#include "ui/SummaryScreen.h"

#include "network/wifi_manager.h"
#include "network/mqtt_manager.h"

// WiFi + MQTT creds (gitignored)
#include "secrets.h"

#ifndef DG_FW_VERSION
#define DG_FW_VERSION "0.0.0"
#endif

// ----- Pin definitions -----
static constexpr uint8_t PIN_RGB       = 48;
static constexpr uint8_t PIN_BACKLIGHT = 14;
static constexpr uint8_t PIN_SDA       = 8;
static constexpr uint8_t PIN_SCL       = 9;
static constexpr uint8_t PIN_RADAR_OT2 = 4;   // LD2410S OT2 (unused with LD2450 — no OT2 pin)

// ----- Timing -----
static constexpr uint32_t RENDER_INTERVAL_MS  = 20;     // ~50 Hz
static constexpr uint32_t SENSOR_INTERVAL_MS  = 2000;   // read env sensors every 2 s
static constexpr uint32_t RADAR_INTERVAL_MS   = 250;    // read radar presence every 250 ms (fast)
static constexpr uint32_t HEARTBEAT_MS        = 10000;
static constexpr uint32_t TELEMETRY_INTERVAL_MS = 10000; // MQTT publish every 10 s

// ----- NTP -----
static constexpr long     GMT_OFFSET_SEC  = 7 * 3600;   // UTC+7 (Vietnam)
static constexpr int      DST_OFFSET_SEC  = 0;

// ----- Globals -----
TFT_eSPI        tft = TFT_eSPI();
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

Bh1750Sensor    luxSensor;
Bme680Sensor    envSensor;
Ld2450Sensor    radarSensor;
// Ld2410sSensor   radarSensor;  // parked — replaced by LD2450
// Ld2410cSensor   radarSensor;  // parked — LD2410C hardware issue
Acd1200Sensor   co2Sensor;

// Screens
ActiveScreen    activeScreen;
AlertScreen     alertScreen;
SummaryScreen   summaryScreen;
Screen         *currentScreen = nullptr;

// State machine
ScreenFsm       fsm;
ScreenMode       lastMode = ScreenMode::ACTIVE;

// NVS config persistence
Preferences      prefs;

// NVS namespace / keys
static constexpr const char *NVS_NS        = "dg_cfg";
static constexpr const char *NVS_SIT_MIN   = "sit_min";
static constexpr const char *NVS_RST_S     = "rst_s";
static constexpr const char *NVS_SUM_S     = "sum_s";
static constexpr const char *NVS_SLP_S     = "slp_s";
static constexpr const char *NVS_RANGE_CM  = "rng_cm";

// Radar presence range (cm) — target beyond this = ABSENT. Editable via web.
static constexpr uint16_t DEFAULT_RANGE_CM = 150;
static uint16_t presenceRangeCm = DEFAULT_RANGE_CM;

/** Load presence range (cm) from NVS, falls back to default. */
static uint16_t loadPresenceRange() {
    prefs.begin(NVS_NS, /*readOnly=*/true);
    uint32_t v = prefs.getUInt(NVS_RANGE_CM, 0);
    prefs.end();
    return (v >= 30 && v <= 600) ? (uint16_t)v : DEFAULT_RANGE_CM;
}

/** Persist presence range (cm) + apply to radar driver. */
static void applyPresenceRange(uint16_t cm) {
    presenceRangeCm = cm;
    Ld2450Config rc;
    rc.maxRangeCm = cm;
    radarSensor.configure(rc);
    prefs.begin(NVS_NS, /*readOnly=*/false);
    prefs.putUInt(NVS_RANGE_CM, cm);
    prefs.end();
    Serial.printf("[CFG] Presence range = %ucm (saved)\n", cm);
}

/** Load thresholds from NVS; falls back to ScreenFsmConfig defaults. */
static ScreenFsmConfig loadConfig() {
    ScreenFsmConfig cfg;  // constructed with defaults
    prefs.begin(NVS_NS, /*readOnly=*/true);
    uint32_t sitMin = prefs.getUInt(NVS_SIT_MIN, 0);
    uint32_t rstS   = prefs.getUInt(NVS_RST_S,   0);
    uint32_t sumS   = prefs.getUInt(NVS_SUM_S,   0);
    uint32_t slpS   = prefs.getUInt(NVS_SLP_S,   0);
    prefs.end();

    if (sitMin > 0) cfg.sitThresholdMs    = sitMin * 60UL * 1000;
    if (rstS   > 0) cfg.resetDelayMs      = rstS   * 1000UL;
    if (sumS   > 0) cfg.summaryDelayMs    = sumS   * 1000UL;
    if (slpS   > 0) cfg.sleepDelayMs      = slpS   * 1000UL;

    Serial.printf("[CFG] Loaded: sit=%um rst=%us sum=%us slp=%us\n",
        cfg.sitThresholdMs / 60000,
        cfg.resetDelayMs   / 1000,
        cfg.summaryDelayMs / 1000,
        cfg.sleepDelayMs   / 1000);
    return cfg;
}

/** Persist current FSM config to NVS. */
static void saveConfig(const ScreenFsmConfig &cfg) {
    prefs.begin(NVS_NS, /*readOnly=*/false);
    prefs.putUInt(NVS_SIT_MIN, cfg.sitThresholdMs / 60000);
    prefs.putUInt(NVS_RST_S,   cfg.resetDelayMs   / 1000);
    prefs.putUInt(NVS_SUM_S,   cfg.summaryDelayMs / 1000);
    prefs.putUInt(NVS_SLP_S,   cfg.sleepDelayMs   / 1000);
    prefs.end();
    Serial.println("[CFG] Saved to NVS.");
}

// Latest sensor bundle
SensorBundle    sensors = {};

// Network
WifiManager     wifiMgr;
MqttManager     mqttMgr;
bool            ntpSynced   = false;
bool            mqttStarted = false;

// ----- Backlight -----
static bool backlightOn = false;

// ----- Helpers -----

static void printChipInfo() {
    Serial.println();
    Serial.println(F("=== dsk-guard firmware ==="));
    Serial.printf("version       : %s\n", DG_FW_VERSION);
    Serial.printf("chip model    : %s rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("cpu freq      : %lu MHz\n", (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("flash size    : %lu bytes\n", (unsigned long)ESP.getFlashChipSize());
    Serial.printf("psram size    : %lu bytes\n", (unsigned long)ESP.getPsramSize());
    Serial.printf("psram free    : %lu bytes\n", (unsigned long)ESP.getFreePsram());
    Serial.printf("heap free     : %lu bytes\n", (unsigned long)ESP.getFreeHeap());
    Serial.printf("sdk version   : %s\n", ESP.getSdkVersion());
    Serial.println(F("=========================="));
}

static void tftSplash() {
    Serial.println(F("[TFT] Splash screen..."));
    tft.fillScreen(Theme::BG);

    // Horizontal sweep bar (cyan accent) — non-blocking feel
    uint16_t barH = 4;
    uint16_t barY = Theme::H / 2 + 30;
    for (int16_t x = 0; x < Theme::W; x += 8) {
        tft.fillRect(x, barY, 8, barH, Theme::CYAN);
        delay(3);  // ~180ms total sweep
    }

    // Title
    tft.setFreeFont(FONT_XL);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::TEXT, Theme::BG);
    tft.drawString("Desk Guardian", Theme::W / 2, Theme::H / 2 - 10);

    // Version
    tft.setFreeFont(FONT_SM);
    tft.setTextColor(Theme::DIM, Theme::BG);
    tft.drawString("fw " DG_FW_VERSION, Theme::W / 2, Theme::H / 2 + 20);

    delay(1200);  // hold splash

    // Fade sweep bar out
    tft.fillRect(0, barY, Theme::W, barH, Theme::BG);

    Serial.println(F("[TFT] Splash done."));
}

static void i2cScan() {
    Serial.println(F("[I2C] Scanning bus..."));
    uint8_t count = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[I2C] Found device @ 0x%02X\n", addr);
            count++;
        }
    }
    Serial.printf("[I2C] Scan done. %d device(s) found.\n", count);
}

static void setBacklight(bool on) {
    digitalWrite(PIN_BACKLIGHT, on ? HIGH : LOW);
    backlightOn = on;
    Serial.printf("[BL] Backlight %s\n", on ? "ON" : "OFF");
}

static void readSensors() {
    sensors.lux   = luxSensor.read();
    sensors.env   = envSensor.read();
    // NOTE: sensors.radar refreshed on the fast radar tick (RADAR_INTERVAL_MS),
    // not here — presence must react in ~250ms, not every 2s.
    sensors.co2   = co2Sensor.read();

    // Serial debug
    if (sensors.lux.ok)
        Serial.printf("[BH1750] %.1f lux\n", sensors.lux.lux);
    if (sensors.env.ok)
        Serial.printf("[BME680] T=%.1fC RH=%.1f%% P=%.0fhPa Gas=%lu ohm\n",
                      sensors.env.temp_c, sensors.env.humidity, sensors.env.pressure,
                      (unsigned long)sensors.env.gas_ohm);
    {
        if (sensors.radar.ok) {
            Serial.printf("[LD2450] %s dist=%ucm raw=%ucm targets=%u\n",
                          sensors.radar.state == PresenceState::PRESENT ? "PRESENT" : "ABSENT",
                          sensors.radar.distance_cm,
                          sensors.radar.raw_distance_cm,
                          sensors.radar.raw_state);
        } else {
            Serial.printf("[LD2450] NO DATA (frames=%lu uart_avail=%d)\n",
                          (unsigned long)radarSensor.frameCount(),
                          radarSensor.uartAvailable());
        }
    }
    if (sensors.co2.ok)
        Serial.printf("[ACD1200] CO2=%u ppm%s\n", sensors.co2.co2_ppm,
                      sensors.co2.preheating ? " (preheating)" : "");
}

static void feedScreens() {
    // Feed sensor data to all screens so they're ready when switched to
    activeScreen.setSensors(sensors);
    summaryScreen.setSensors(sensors);

    // Feed countdown to active screen
    uint32_t threshSec = 45 * 60;  // default, will come from config later
    activeScreen.setCountdown(fsm.countdownSec(), threshSec, fsm.sittingSec());

    // Status pill
    if (sensors.radar.ok && sensors.radar.state == PresenceState::PRESENT) {
        activeScreen.setStatus("Active", Theme::GREEN);
    } else {
        activeScreen.setStatus("Away", Theme::DIM);
    }

    // Alert screen sitting time
    alertScreen.setSittingMin(fsm.sittingSec() / 60);

    // Network status in footer
    activeScreen.setNetStatus(wifiMgr.statusText().c_str());
}

static void switchScreen(ScreenMode mode) {
    switch (mode) {
        case ScreenMode::ACTIVE:
            currentScreen = &activeScreen;
            setBacklight(true);
            break;
        case ScreenMode::ALERT:
            currentScreen = &alertScreen;
            setBacklight(true);
            break;
        case ScreenMode::SUMMARY:
            currentScreen = &summaryScreen;
            setBacklight(true);
            break;
        case ScreenMode::SLEEP:
            setBacklight(false);
            currentScreen = nullptr;
            break;
    }

    if (currentScreen) {
        currentScreen->begin(tft);
    }

    const char *names[] = {"ACTIVE", "ALERT", "SUMMARY", "SLEEP"};
    Serial.printf("[UI] Screen → %s\n", names[(uint8_t)mode]);
}

// ----- NTP -----

static bool ntpConfigured = false;

/// Non-blocking NTP: configure once, then poll.
/// Returns true once time is valid. Never blocks.
static bool tryNTPSync() {
    if (!ntpConfigured) {
        configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, DG_MQTT_HOST, "pool.ntp.org");
        ntpConfigured = true;
        Serial.println("[NTP] Configured — waiting for sync (non-blocking)...");
    }
    struct tm ti;
    if (getLocalTime(&ti, 0)) {  // 0 = no wait
        Serial.printf("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                      ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
                      ti.tm_hour, ti.tm_min, ti.tm_sec);
        return true;
    }
    return false;
}

static time_t getUnixTime() {
    struct tm ti;
    if (getLocalTime(&ti, 0)) {
        return mktime(&ti);
    }
    // Fallback: seconds since boot (not accurate but non-zero)
    return (time_t)(millis() / 1000);
}

// ----- MQTT Telemetry -----

static void publishTelemetry() {
    if (!mqttMgr.isConnected()) return;

    JsonDocument doc;

    doc["ts"]         = (long)getUnixTime();
    doc["node_id"]    = DG_NODE_ID;
    doc["fw_version"] = DG_FW_VERSION;

    // BH1750
    if (sensors.lux.ok)
        doc["lux"] = round(sensors.lux.lux * 10.0) / 10.0;
    else
        doc["lux"] = nullptr;

    // BME680
    if (sensors.env.ok) {
        doc["temp_c"]       = round(sensors.env.temp_c * 10.0) / 10.0;
        doc["humidity"]     = round(sensors.env.humidity * 10.0) / 10.0;
        doc["pressure_hpa"] = round(sensors.env.pressure * 100.0) / 100.0;
        doc["gas_ohm"]      = sensors.env.gas_ohm;
    } else {
        doc["temp_c"]       = nullptr;
        doc["humidity"]     = nullptr;
        doc["pressure_hpa"] = nullptr;
        doc["gas_ohm"]      = nullptr;
    }

    // ACD1200
    if (sensors.co2.ok && !sensors.co2.preheating)
        doc["co2_ppm"] = sensors.co2.co2_ppm;
    else
        doc["co2_ppm"] = nullptr;
    doc["co2_preheating"] = sensors.co2.preheating;

    // LD2450
    doc["presence"] = (sensors.radar.ok && sensors.radar.state == PresenceState::PRESENT)
                      ? "PRESENT" : "ABSENT";
    if (sensors.radar.ok && sensors.radar.state == PresenceState::PRESENT)
        doc["distance_cm"] = sensors.radar.distance_cm;
    else
        doc["distance_cm"] = nullptr;
    // Raw nearest-target distance — ALWAYS sent when any target seen (ignores range gate)
    if (sensors.radar.ok && sensors.radar.raw_distance_cm > 0)
        doc["radar_nearest_cm"] = sensors.radar.raw_distance_cm;
    else
        doc["radar_nearest_cm"] = nullptr;
    doc["sit_seconds"] = (int)fsm.sittingSec();

    // Serialize + publish
    char buf[480];
    size_t len = serializeJson(doc, buf, sizeof(buf));

    char topic[64];
    snprintf(topic, sizeof(topic), "dg/%s/telemetry/env", DG_NODE_ID);
    mqttMgr.publish(topic, buf);

    Serial.printf("[MQTT] Telemetry %u bytes → %s\n", (unsigned)len, topic);
}

static void publishHello() {
    if (!mqttMgr.isConnected()) return;

    JsonDocument doc;

    doc["node_id"]       = DG_NODE_ID;
    doc["fw_version"]    = DG_FW_VERSION;
    doc["chip_model"]    = ESP.getChipModel();
    doc["chip_revision"] = ESP.getChipRevision();
    doc["flash_size"]    = ESP.getFlashChipSize();
    doc["psram_size"]    = ESP.getPsramSize();
    doc["mac"]           = wifiMgr.getMAC();
    doc["ip"]            = wifiMgr.getIP();
    doc["uptime_s"]      = (int)(millis() / 1000);
    doc["ts"]            = (long)getUnixTime();

    char buf[320];
    serializeJson(doc, buf, sizeof(buf));

    char topic[64];
    snprintf(topic, sizeof(topic), "dg/%s/hello", DG_NODE_ID);
    mqttMgr.publish(topic, buf, true);  // retained

    Serial.printf("[MQTT] Hello (retained) → %s\n", topic);
}

static void onMqttCmd(const char *topic, const uint8_t *payload, unsigned int len) {
    // Parse topic: dg/<node>/cmd/<cmd>
    String t(topic);
    Serial.printf("[MQTT] CMD: %s\n", topic);

    if (t.endsWith("/cmd/factory_reset")) {
        Serial.println("[CMD] Factory reset requested — not yet implemented (NVS wipe + reboot).");
        // TODO: Preferences clear + ESP.restart()
    } else if (t.endsWith("/cmd/config")) {
        Serial.printf("[CMD] Config push (%u bytes)\n", len);

        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload, len);
        if (err) {
            Serial.printf("[CMD] Config JSON parse error: %s\n", err.c_str());
            return;
        }

        // Read current config, overlay only provided fields
        ScreenFsmConfig cfg = loadConfig();

        if (doc.containsKey("sit_minutes") && doc["sit_minutes"].is<uint32_t>()) {
            uint32_t m = doc["sit_minutes"].as<uint32_t>();
            if (m >= 1 && m <= 180) cfg.sitThresholdMs = m * 60UL * 1000;
        }
        if (doc.containsKey("reset_delay_s") && doc["reset_delay_s"].is<uint32_t>()) {
            uint32_t s = doc["reset_delay_s"].as<uint32_t>();
            if (s >= 1 && s <= 300) cfg.resetDelayMs = s * 1000UL;
        }
        if (doc.containsKey("summary_delay_s") && doc["summary_delay_s"].is<uint32_t>()) {
            uint32_t s = doc["summary_delay_s"].as<uint32_t>();
            if (s >= 1 && s <= 600) cfg.summaryDelayMs = s * 1000UL;
        }
        if (doc.containsKey("sleep_delay_s") && doc["sleep_delay_s"].is<uint32_t>()) {
            uint32_t s = doc["sleep_delay_s"].as<uint32_t>();
            if (s >= 10 && s <= 3600) cfg.sleepDelayMs = s * 1000UL;
        }

        // Radar presence range (cm) — applies to LD2450 driver live + NVS
        if (doc.containsKey("presence_range_cm") && doc["presence_range_cm"].is<uint32_t>()) {
            uint32_t cm = doc["presence_range_cm"].as<uint32_t>();
            if (cm >= 30 && cm <= 600) applyPresenceRange((uint16_t)cm);
        }

        // Apply live + persist
        fsm.begin(cfg);
        saveConfig(cfg);

        Serial.printf("[CMD] Config applied: sit=%um\n", cfg.sitThresholdMs / 60000);
    }
}

// ----- Arduino entry points -----

void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0) < 1500) delay(10);

    printChipInfo();

    if (ESP.getPsramSize() == 0)
        Serial.println(F("[WARN] PSRAM not detected."));

    // Backlight (fixed digital)
    pinMode(PIN_BACKLIGHT, OUTPUT);
    setBacklight(true);

    // TFT
    tft.init();
    tft.setRotation(3);   // 180° from rot 1 (display mounted upside-down in box)
    tft.fillScreen(TFT_BLACK);
    Serial.printf("[TFT] Init OK. %dx%d\n", tft.width(), tft.height());
    tftSplash();

    // RGB LED
    rgb.begin();
    rgb.setBrightness(32);
    rgb.setPixelColor(0, 0x001000);
    rgb.show();

    // I2C
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    Serial.println(F("[I2C] Bus init: SDA=8, SCL=9, 400kHz"));
    i2cScan();

    // Sensors
    luxSensor.begin();
    envSensor.begin();
    radarSensor.begin(Serial2, 16, 15);   // LD2450: no OT2 pin
    (void)PIN_RADAR_OT2;
    // Apply persisted presence range (cm) from NVS
    presenceRangeCm = loadPresenceRange();
    {
        Ld2450Config rc;
        rc.maxRangeCm = presenceRangeCm;
        radarSensor.configure(rc);
    }
    Serial.printf("[CFG] Presence range = %ucm\n", presenceRangeCm);
    co2Sensor.begin(Serial1);

    delay(200);
    readSensors();
    sensors.radar = radarSensor.read();   // initial presence for first paint

    // FSM — load thresholds from NVS (falls back to defaults on first boot)
    ScreenFsmConfig cfg = loadConfig();
    fsm.begin(cfg);

    // Feed initial data and start on ACTIVE
    feedScreens();
    switchScreen(ScreenMode::ACTIVE);
    lastMode = ScreenMode::ACTIVE;

    // ----- Network (non-blocking: firmware works standalone if WiFi absent) -----
    wifiMgr.begin(DG_WIFI_SSID, DG_WIFI_PASSWORD);
    mqttMgr.config(DG_MQTT_HOST, DG_MQTT_PORT, DG_MQTT_USER, DG_MQTT_PASSWORD, DG_NODE_ID);
    mqttMgr.onMessage(onMqttCmd);

    Serial.printf("[MEM] Post-init heap=%lu psram=%lu\n",
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)ESP.getFreePsram());
}

void loop() {
    static uint32_t lastRender    = 0;
    static uint32_t lastSensor    = 0;
    static uint32_t lastHeartbeat = 0;
    static uint32_t lastTelemetry = 0;

    uint32_t now = millis();

    // Network maintenance (non-blocking, standalone-first)
    wifiMgr.loop();

    // Lazy MQTT init: start only when WiFi first connects
    if (wifiMgr.isConnected() && !mqttStarted) {
        if (!ntpSynced) {
            ntpSynced = tryNTPSync();  // non-blocking, retries next loop
        }
        if (ntpSynced) {
            mqttStarted = true;  // always true — loop() handles reconnect with backoff
            if (mqttMgr.begin()) publishHello();
        }
    }

    if (mqttStarted) mqttMgr.loop();

    // Poll radar (needs frequent calls)
    radarSensor.poll();

    // Fast radar presence read — FSM must react quickly (not every 2s)
    static uint32_t lastRadar = 0;
    if (now - lastRadar >= RADAR_INTERVAL_MS) {
        lastRadar = now;
        sensors.radar = radarSensor.read();
    }

    // Read env sensors (slow)
    if (now - lastSensor >= SENSOR_INTERVAL_MS) {
        lastSensor = now;
        readSensors();
        feedScreens();

    }

    // Run FSM with current presence
    PresenceState presence = sensors.radar.ok
        ? sensors.radar.state
        : PresenceState::ABSENT;

    bool changed = fsm.update(presence, now);

    if (changed) {
        switchScreen(fsm.mode());
        lastMode = fsm.mode();
    }

    // Update countdown every loop (even without sensor change)
    if (fsm.mode() == ScreenMode::ACTIVE) {
        activeScreen.setCountdown(fsm.countdownSec(),
                                   45 * 60,  // threshold sec (matches FSM default)
                                   fsm.sittingSec());
    }

    // Render ~10 Hz
    if (now - lastRender >= RENDER_INTERVAL_MS) {
        lastRender = now;
        if (currentScreen) {
            currentScreen->update(tft, now);
        }
    }

    // MQTT telemetry publish every 10s
    if (now - lastTelemetry >= TELEMETRY_INTERVAL_MS) {
        lastTelemetry = now;
        publishTelemetry();
    }

    // Heartbeat
    if (now - lastHeartbeat >= HEARTBEAT_MS) {
        lastHeartbeat = now;
        Serial.printf("[hb] up=%lus heap=%lu mode=%d sitting=%lus wifi=%d mqtt=%d\n",
                      (unsigned long)(now / 1000),
                      (unsigned long)ESP.getFreeHeap(),
                      (uint8_t)fsm.mode(),
                      (unsigned long)fsm.sittingSec(),
                      wifiMgr.isConnected() ? 1 : 0,
                      mqttMgr.isConnected() ? 1 : 0);
        rgb.setPixelColor(0, 0x000000);
        rgb.show();
        delay(50);
        rgb.setPixelColor(0, 0x001000);
        rgb.show();
    }
}
