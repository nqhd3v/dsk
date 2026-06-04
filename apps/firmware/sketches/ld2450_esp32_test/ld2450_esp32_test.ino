// ============================================================================
// LD2450 mmWave radar — ESP32 standalone test sketch
// ----------------------------------------------------------------------------
// Prints per-target X / Y / distance (radius) / angle / speed to Serial Monitor.
// Self-contained: raw UART frame parser, NO external library.
//
// WIRING (ESP32 / ESP32-S3):
//   LD2450 5V   -> ESP 5V         (** module power = 5V, supply > 200 mA **)
//   LD2450 GND  -> ESP GND        (common ground)
//   LD2450 Tx   -> ESP GPIO 16    (ESP RX2)   radar TX is 3.3V -> direct, no shifter
//   LD2450 Rx   -> ESP GPIO 15    (ESP TX2)   ESP TX 3.3V -> radar 3.3V IO, OK
//
//   Module has NO OT2 / digital presence pin (only 5V / GND / Tx / Rx).
//
// SERIAL:
//   USB monitor : 115200 baud
//   Radar UART  : 256000 baud, 8N1 (LD2450 factory default)
//
// PROTOCOL (HLK-LD2450 manual V1.00 §6):
//   Frame (30 bytes @ 10 fps):
//     AA FF 03 00 | target1(8) | target2(8) | target3(8) | 55 CC
//   Per target (8 bytes, little-endian):
//     X(int16) Y(int16) speed(int16) distRes(uint16)
//   Sign-magnitude: bit15 = 1 -> positive (raw & 0x7FFF); bit15 = 0 -> negative (-raw)
//   All-zero 8-byte block = empty target slot.
// ============================================================================

#include <Arduino.h>
#include <math.h>

// ---- Pins / UART ----
static const int   RADAR_RX_PIN = 16;   // ESP RX  <- radar Tx
static const int   RADAR_TX_PIN = 15;   // ESP TX  -> radar Rx
static const long  RADAR_BAUD   = 256000;
HardwareSerial RadarSerial(2);          // use UART2

// ---- Frame parser ----
static const uint8_t  FRAME_LEN  = 30;
static const uint8_t  HEADER[4]  = {0xAA, 0xFF, 0x03, 0x00};
static const uint8_t  TAIL[2]    = {0x55, 0xCC};
static uint8_t        frameBuf[FRAME_LEN];
static uint8_t        framePos = 0;
static uint32_t       frameCount = 0;
static uint32_t       lastDataMs = 0;

struct Target {
  int16_t  x_mm;
  int16_t  y_mm;
  int16_t  speed_cms;
  uint16_t res_mm;
  float    dist_cm;     // radius = sqrt(x^2 + y^2)
  float    angle_deg;   // atan2(x, y): 0 = straight ahead, + = right, - = left
  bool     valid;
};

// Sign-magnitude decode: bit15 set -> positive, clear -> negative
static int16_t decodeSigned(uint16_t raw) {
  if (raw & 0x8000) return (int16_t)(raw & 0x7FFF);
  return (int16_t)(-(int32_t)raw);
}

static void parseFrame() {
  frameCount++;
  lastDataMs = millis();

  Serial.printf("\n--- frame #%lu (t=%lus) ---\n",
                (unsigned long)frameCount, (unsigned long)(millis() / 1000));

  uint8_t active = 0;
  for (uint8_t i = 0; i < 3; i++) {
    const uint8_t *p = &frameBuf[4 + i * 8];
    uint16_t rx  = p[0] | (p[1] << 8);
    uint16_t ry  = p[2] | (p[3] << 8);
    uint16_t rs  = p[4] | (p[5] << 8);
    uint16_t res = p[6] | (p[7] << 8);

    if (rx == 0 && ry == 0 && rs == 0 && res == 0) {
      Serial.printf("  target %u: --- (empty)\n", i + 1);
      continue;
    }

    Target t;
    t.x_mm      = decodeSigned(rx);
    t.y_mm      = decodeSigned(ry);
    t.speed_cms = decodeSigned(rs);
    t.res_mm    = res;
    t.dist_cm   = sqrtf((float)t.x_mm * t.x_mm + (float)t.y_mm * t.y_mm) / 10.0f;
    t.angle_deg = atan2f((float)t.x_mm, (float)t.y_mm) * 180.0f / PI;
    t.valid     = true;
    active++;

    Serial.printf("  target %u: X=%5d mm  Y=%5d mm  dist=%6.1f cm  angle=%+6.1f deg  speed=%+4d cm/s  res=%u mm\n",
                  i + 1, t.x_mm, t.y_mm, t.dist_cm, t.angle_deg, t.speed_cms, t.res_mm);
  }
  if (active == 0) Serial.println("  (no targets in view)");
}

static void feedByte(uint8_t b) {
  // Sync on 4-byte header, then collect to 30 bytes, verify tail.
  if (framePos < 4) {
    if (b == HEADER[framePos]) {
      frameBuf[framePos++] = b;
    } else if (b == HEADER[0]) {
      frameBuf[0] = b;
      framePos = 1;
    } else {
      framePos = 0;
    }
    return;
  }

  frameBuf[framePos++] = b;
  if (framePos == FRAME_LEN) {
    if (frameBuf[28] == TAIL[0] && frameBuf[29] == TAIL[1]) {
      parseFrame();
    }
    framePos = 0;
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 1500) delay(10);

  Serial.println(F("\n=== LD2450 ESP32 test ==="));
  Serial.printf("Radar UART2 @ %ld baud, RX=GPIO%d, TX=GPIO%d\n",
                RADAR_BAUD, RADAR_RX_PIN, RADAR_TX_PIN);
  Serial.println(F("Power: 5V. Signals: 3.3V. No OT2 pin."));

  RadarSerial.begin(RADAR_BAUD, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  delay(500);

  int pre = RadarSerial.available();
  Serial.printf("Pre-init bytes in RX buffer: %d\n", pre);
  if (pre == 0)
    Serial.println(F("WARNING: no data yet. Check 5V power, Tx/Rx swap, common GND, baud 256000."));
  lastDataMs = millis();
}

void loop() {
  while (RadarSerial.available()) {
    feedByte(RadarSerial.read());
  }

  // No-data watchdog
  static uint32_t lastWarn = 0;
  if (millis() - lastDataMs > 3000 && millis() - lastWarn > 3000) {
    lastWarn = millis();
    Serial.printf("[diag] no frames for %lus (frames=%lu, uart_avail=%d). Check wiring/baud.\n",
                  (unsigned long)((millis() - lastDataMs) / 1000),
                  (unsigned long)frameCount, RadarSerial.available());
  }
}
