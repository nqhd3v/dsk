// LD2410S raw UART test for Arduino UNO R4 WiFi
// Purpose: verify LD2410S radar module works, independent of ESP32
//
// !! LD2410S is 3.3V (3.0-3.6V) — DO NOT connect to 5V !!
//
// Wiring (UNO R4 WiFi — 3.3V logic):
//   LD2410S J2 Pin1 (3V3)  → 3.3V  (NOT 5V!)
//   LD2410S J2 Pin2 (GND)  → GND
//   LD2410S J2 Pin3 (OT1/TX) → D0 (Serial1 RX)
//   LD2410S J2 Pin4 (RX)   → D1 (Serial1 TX)
//   LD2410S J2 Pin5 (OT2)  → not connected (optional GPIO status)
//
// Baud rate: 115200
// Upload, open Serial Monitor @ 115200.
//
// LD2410S default output: MINIMAL frame format
// (Protocol doc V1.00, §2.1):
//   6E                    — frame head (1 byte)
//   SS                    — target state: 0/1=no one, 2/3=someone
//   DL DH                 — object distance in cm (2 bytes, little-endian)
//   62                    — frame end (1 byte)
//   Total: 5 bytes per frame

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("=== LD2410S Raw UART Test (UNO R4 WiFi) ===");
    Serial.println("!! VCC must be 3.3V — NOT 5V !!");
    Serial.println("Wiring: J2-Pin1(3V3)->3.3V, Pin2(GND)->GND, Pin3(OT1/TX)->D0, Pin4(RX)->D1");
    Serial.println("Expecting minimal frames: 6E [state] [dist_lo] [dist_hi] 62");
    Serial.println("Waiting for data...\n");

    Serial1.begin(115200);

    delay(1000);  // Let radar boot
}

static uint32_t byteCount = 0;
static uint32_t frameCount = 0;
static uint32_t lastPrint = 0;

// Raw hex dump mode — first 30s, dump all bytes for debug
static bool dumpMode = true;
static uint32_t dumpStart = 0;
static uint16_t dumpLineBytes = 0;

// Minimal frame parser: 6E [state] [dist_lo] [dist_hi] 62
static enum { WAIT_HEAD, IN_BODY } parseState = WAIT_HEAD;
static uint8_t frameBuf[5];
static uint8_t framePos = 0;

void loop() {
    while (Serial1.available()) {
        uint8_t b = Serial1.read();
        byteCount++;

        // Raw hex dump for first 30 seconds
        if (dumpMode) {
            if (dumpStart == 0) { dumpStart = millis(); Serial.print("[HEX] "); }
            if (b < 0x10) Serial.print('0');
            Serial.print(b, HEX);
            Serial.print(' ');
            dumpLineBytes++;
            if (dumpLineBytes >= 20) {
                Serial.println();
                Serial.print("[HEX] ");
                dumpLineBytes = 0;
            }
            if (millis() - dumpStart > 30000) {
                dumpMode = false;
                Serial.println("\n[HEX] Dump done. Switching to frame parser only.");
            }
        }

        // Minimal frame parser
        switch (parseState) {
            case WAIT_HEAD:
                if (b == 0x6E) {
                    frameBuf[0] = b;
                    framePos = 1;
                    parseState = IN_BODY;
                }
                break;
            case IN_BODY:
                frameBuf[framePos++] = b;
                if (framePos == 5) {
                    if (frameBuf[4] == 0x62) {
                        // Valid minimal frame
                        frameCount++;

                        uint8_t  tgtState = frameBuf[1];
                        uint16_t distance = frameBuf[2] | (frameBuf[3] << 8);

                        const char* stateStr;
                        switch (tgtState) {
                            case 0: stateStr = "NO_ONE(0)"; break;
                            case 1: stateStr = "NO_ONE(1)"; break;
                            case 2: stateStr = "SOMEONE(2)"; break;
                            case 3: stateStr = "SOMEONE(3)"; break;
                            default: stateStr = "UNKNOWN"; break;
                        }

                        Serial.print("[FRAME #");
                        Serial.print(frameCount);
                        Serial.print("] state=");
                        Serial.print(stateStr);
                        Serial.print(" dist=");
                        Serial.print(distance);
                        Serial.print("cm raw: ");
                        for (uint8_t i = 0; i < 5; i++) {
                            if (frameBuf[i] < 0x10) Serial.print('0');
                            Serial.print(frameBuf[i], HEX);
                            Serial.print(' ');
                        }
                        Serial.println();
                    }
                    // Reset regardless
                    parseState = WAIT_HEAD;
                    framePos = 0;
                }
                break;
        }
    }

    // Status every 5s
    uint32_t now = millis();
    if (now - lastPrint >= 5000) {
        lastPrint = now;
        Serial.print("[STATUS] ");
        Serial.print(now / 1000);
        Serial.print("s  bytes=");
        Serial.print(byteCount);
        Serial.print("  frames=");
        Serial.print(frameCount);
        Serial.print("  uart_avail=");
        Serial.println(Serial1.available());

        if (byteCount == 0) {
            Serial.println("  !! No bytes received. Check:");
            Serial.println("  !! VCC=3.3V (NOT 5V!), GND, OT1(TX)->D0, RX->D1");
        }
    }
}
