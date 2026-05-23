// dsk-guard — Phase 0 skeleton
// Target: MKE-K01 (ESP32-S3-WROOM-1 N16R8)
// Goal: smoke-test toolchain + flash + PSRAM + on-board RGB.
//
// LED: WS2812 on GPIO 48 (single pixel). Cycles R → G → B every 1 s.
// Serial: 115200 baud via CH343P. Prints chip info + free heap/PSRAM.

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#ifndef DG_FW_VERSION
#define DG_FW_VERSION "0.0.0"
#endif

static constexpr uint8_t  RGB_PIN    = 48;
static constexpr uint16_t RGB_COUNT  = 1;
static constexpr uint32_t TICK_MS    = 1000;

Adafruit_NeoPixel rgb(RGB_COUNT, RGB_PIN, NEO_GRB + NEO_KHZ800);

static const uint32_t COLORS[] = {
  0x200000,  // dim red
  0x002000,  // dim green
  0x000020,  // dim blue
};
static constexpr size_t COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);

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

void setup() {
  Serial.begin(115200);
  // Wait briefly for serial monitor to attach; do not block boot forever.
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 1500) {
    delay(10);
  }

  printChipInfo();

  if (ESP.getPsramSize() == 0) {
    Serial.println(F("[WARN] PSRAM not detected — check platformio.ini psram flags."));
  }

  rgb.begin();
  rgb.setBrightness(64);
  rgb.show();  // off
  Serial.println(F("[OK] Phase 0 smoke test running. RGB cycle R→G→B."));
}

void loop() {
  static size_t   idx          = 0;
  static uint32_t lastTick     = 0;
  static uint32_t heartbeatTick = 0;

  uint32_t now = millis();

  if (now - lastTick >= TICK_MS) {
    lastTick = now;
    rgb.setPixelColor(0, COLORS[idx]);
    rgb.show();
    idx = (idx + 1) % COLOR_COUNT;
  }

  // Heartbeat log every 10 s.
  if (now - heartbeatTick >= 10000) {
    heartbeatTick = now;
    Serial.printf("[hb] up=%lus heap=%lu psram_free=%lu\n",
                  (unsigned long)(now / 1000),
                  (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)ESP.getFreePsram());
  }
}
