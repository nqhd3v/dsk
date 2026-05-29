#pragma once

// --- Driver ---
#define ILI9488_DRIVER

// --- Resolution ---
#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// --- ESP32-S3 SPI pins (HSPI) ---
#define TFT_MOSI  11
#define TFT_MISO  13
#define TFT_SCLK  12
#define TFT_CS    21
#define TFT_DC    10
#define TFT_RST   47
#define TFT_BL    14    // Backlight enable (active HIGH assumed)

// --- SPI frequency ---
// Start conservative at 27 MHz. Raise to 40 MHz if stable (step 1.4).
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  16000000
#define SPI_TOUCH_FREQUENCY  2500000  // XPT2046, Phase 5

// --- Touch (Phase 5 — wired later) ---
// Uncomment when XPT2046 wired:
// #define TOUCH_CS  42

// --- Misc ---
#define LOAD_GLCD    // 8px font
#define LOAD_FONT2   // 16px
#define LOAD_FONT4   // 26px
#define LOAD_FONT6   // 48px digit font
#define LOAD_FONT7   // 7-seg 48px
#define LOAD_FONT8   // 75px digit font
#define LOAD_GFXFF   // FreeFonts

// Use HSPI (SPI2) on ESP32-S3 — SPI3 also works but HSPI is default.
#define USE_HSPI_PORT
