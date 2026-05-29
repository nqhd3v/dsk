// LD2410C raw UART test for Arduino UNO R4 WiFi
// Purpose: verify radar module sends data, independent of ESP32 + ld2410 library
//
// Wiring (UNO R4 WiFi — 3.3V logic, no level shifter needed):
//   Radar VCC  → 5V
//   Radar GND  → GND
//   Radar TX   → D0 (Serial1 RX)
//   Radar RX   → D1 (Serial1 TX)
//
// Upload, open Serial Monitor @ 115200.
// If radar works: you'll see hex frames starting with F4 F3 F2 F1
// If nothing: radar not transmitting (power issue or dead module)

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("=== LD2410C Raw UART Test (UNO R4 WiFi) ===");
    Serial.println("Wiring: Radar TX → D0, Radar RX → D1, VCC → 5V, GND → GND");
    Serial.println("Expecting frames: F4 F3 F2 F1 ... F8 F7 F6 F5");
    Serial.println("Waiting for data...\n");

    Serial1.begin(256000);

    delay(1000);  // Let radar boot
}

static uint32_t byteCount = 0;
static uint32_t frameCount = 0;
static uint32_t lastPrint = 0;

// Simple frame detector state machine
static enum { WAIT_F4, WAIT_F3, WAIT_F2, WAIT_F1, IN_FRAME } state = WAIT_F4;
static uint8_t frameBuf[64];
static uint8_t framePos = 0;

void loop() {
    while (Serial1.available()) {
        uint8_t b = Serial1.read();
        byteCount++;

        // Detect frame header F4 F3 F2 F1
        switch (state) {
            case WAIT_F4:
                if (b == 0xF4) { state = WAIT_F3; frameBuf[0] = b; framePos = 1; }
                break;
            case WAIT_F3:
                if (b == 0xF3) { state = WAIT_F2; frameBuf[1] = b; framePos = 2; }
                else { state = WAIT_F4; }
                break;
            case WAIT_F2:
                if (b == 0xF2) { state = WAIT_F1; frameBuf[2] = b; framePos = 3; }
                else { state = WAIT_F4; }
                break;
            case WAIT_F1:
                if (b == 0xF1) { state = IN_FRAME; frameBuf[3] = b; framePos = 4; }
                else { state = WAIT_F4; }
                break;
            case IN_FRAME:
                if (framePos < sizeof(frameBuf)) {
                    frameBuf[framePos++] = b;
                }
                // Check for end marker F8 F7 F6 F5
                if (framePos >= 8 &&
                    frameBuf[framePos-4] == 0xF8 &&
                    frameBuf[framePos-3] == 0xF7 &&
                    frameBuf[framePos-2] == 0xF6 &&
                    frameBuf[framePos-1] == 0xF5) {

                    frameCount++;

                    // Parse basic target data (byte offsets from frame start)
                    // [4-5] intra-frame length
                    // [6] data type (0x02 = target)
                    // [7] head (0xAA)
                    // [8] target_type: 0=none, 1=moving, 2=stationary, 3=both
                    // [9-10] moving distance cm
                    // [11] moving energy
                    // [12-13] stationary distance cm
                    // [14] stationary energy
                    // [15-16] detection distance cm

                    if (framePos >= 17 && frameBuf[6] == 0x02 && frameBuf[7] == 0xAA) {
                        uint8_t  tgtType  = frameBuf[8];
                        uint16_t movDist  = frameBuf[9] | (frameBuf[10] << 8);
                        uint8_t  movEng   = frameBuf[11];
                        uint16_t statDist = frameBuf[12] | (frameBuf[13] << 8);
                        uint8_t  statEng  = frameBuf[14];
                        uint16_t detDist  = frameBuf[15] | (frameBuf[16] << 8);

                        const char* typeStr[] = {"NONE", "MOVING", "STATIONARY", "BOTH"};
                        uint8_t ti = (tgtType <= 3) ? tgtType : 0;

                        Serial.print("[FRAME #");
                        Serial.print(frameCount);
                        Serial.print("] type=");
                        Serial.print(typeStr[ti]);
                        Serial.print(" mov_d=");
                        Serial.print(movDist);
                        Serial.print("cm mov_e=");
                        Serial.print(movEng);
                        Serial.print(" stat_d=");
                        Serial.print(statDist);
                        Serial.print("cm stat_e=");
                        Serial.print(statEng);
                        Serial.print(" det_d=");
                        Serial.print(detDist);
                        Serial.println("cm");
                    } else {
                        // Dump raw hex for non-target frames
                        Serial.print("[FRAME #");
                        Serial.print(frameCount);
                        Serial.print("] raw: ");
                        for (uint8_t i = 0; i < framePos && i < 32; i++) {
                            if (frameBuf[i] < 0x10) Serial.print('0');
                            Serial.print(frameBuf[i], HEX);
                            Serial.print(' ');
                        }
                        Serial.println();
                    }

                    state = WAIT_F4;
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
            Serial.println("  !! No bytes received. Check: VCC=5V, GND, TX→D0, RX→D1");
        }
    }
}
