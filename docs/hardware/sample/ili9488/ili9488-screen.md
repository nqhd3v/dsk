Sample code from provider:

````
/*
  Bàn phím số trên màn hình cảm ứng 480x320 với các khung nhỏ gọn hơn
*/

#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Đường dẫn lưu dữ liệu hiệu chỉnh cảm ứng
#define CALIBRATION_FILE "/TouchCalData2"
#define REPEAT_CAL false

// Tọa độ và kích thước bàn phím
#define KEY_X 60  // Tọa độ X bắt đầu
#define KEY_Y 50  // Tọa độ Y bắt đầu
#define KEY_W 70  // Chiều rộng nút
#define KEY_H 40  // Chiều cao nút
#define KEY_SPACING_X 15 // Khoảng cách ngang giữa các nút
#define KEY_SPACING_Y 15 // Khoảng cách dọc giữa các nút
#define KEY_TEXTSIZE 1   // Cỡ chữ trên nút

// Font chữ
#define LABEL1_FONT &FreeSansOblique9pt7b
#define LABEL2_FONT &FreeSansBold9pt7b

// Vùng hiển thị số (chỉnh DISP_X để khung nằm bên phải)
#define DISP_X 280  // 480 - DISP_W - 20
#define DISP_Y 10
#define DISP_W 180
#define DISP_H 60
#define DISP_TSIZE 2
#define DISP_TCOLOR TFT_CYAN


// Vị trí hiển thị trạng thái
#define STATUS_X 370
#define STATUS_Y 80

// Bộ đệm số nhập vào
#define NUM_LEN 12
char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

// Tạo nhãn cho các nút bấm
char keyLabel[15][5] = {"New", "Del", "Send", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "#"};
uint16_t keyColor[15] = {TFT_RED, TFT_ORANGE, TFT_DARKGREEN,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE};

// Tạo các nút bấm
TFT_eSPI_Button key[15];

// Khởi tạo chương trình
void setup() {
  Serial.begin(9600);
  tft.init();

  // Xoay màn hình ngang
  tft.setRotation(1);

  // Hiệu chỉnh cảm ứng
  touch_calibrate();

  // Vẽ giao diện
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 480, 320, TFT_DARKGREY);
  tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_BLACK);
  tft.drawRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);
  drawKeypad();
}

void loop() {
  uint16_t t_x = 0, t_y = 0;
  bool pressed = tft.getTouch(&t_x, &t_y);

  for (uint8_t b = 0; b < 15; b++) {
    if (pressed && key[b].contains(t_x, t_y)) {
      key[b].press(true);
    } else {
      key[b].press(false);
    }
  }

  for (uint8_t b = 0; b < 15; b++) {
    if (b < 3) tft.setFreeFont(LABEL1_FONT);
    else tft.setFreeFont(LABEL2_FONT);

    if (key[b].justReleased()) key[b].drawButton();
    if (key[b].justPressed()) {
      key[b].drawButton(true);
      if (b >= 3) {
        if (numberIndex < NUM_LEN) {
          numberBuffer[numberIndex] = keyLabel[b][0];
          numberIndex++;
          numberBuffer[numberIndex] = 0;
        }
        status("");
      }
      if (b == 1) { // Xóa ký tự
        if (numberIndex > 0) numberBuffer[--numberIndex] = 0;
        status("");
      }
      if (b == 2) { // Gửi số qua Serial
        status("Sent to Serial");
        Serial.println(numberBuffer);
      }
      if (b == 0) { // Xóa toàn bộ
        status("Cleared");
        numberIndex = 0;
        numberBuffer[numberIndex] = 0;
      }

// Hiển thị số

tft.setTextDatum(TL_DATUM);  // Canh trái chữ số
tft.setFreeFont(&FreeSans18pt7b);  // Dùng font chữ lớn
tft.setTextColor(DISP_TCOLOR);

// Tính toán chiều rộng của chuỗi số
int strWidth = tft.textWidth(numberBuffer);

// Giới hạn số ký tự hiển thị nếu chuỗi dài hơn chiều rộng khung
if (strWidth > DISP_W) {
  // Tính toán số ký tự có thể hiển thị trong khung
  int maxChars = DISP_W / (tft.textWidth("0") + 1);  // Tính số ký tự có thể hiển thị
  numberBuffer[maxChars] = '\0';  // Cắt chuỗi theo số ký tự có thể hiển thị
}

// Vẽ số tại vị trí đã tính toán
int xPos = DISP_X + 4;  // Vị trí X của chữ (canh trái)
int yPos = DISP_Y + 10;  // Điều chỉnh vị trí Y (giảm giá trị để đưa chữ lên)

int xwidth = tft.drawString(numberBuffer, xPos, yPos);

// Xóa phần dư nếu chuỗi số không đủ dài để lấp đầy khung
if (xwidth < DISP_W) {
  tft.fillRect(DISP_X + 4 + xwidth, DISP_Y + 1, DISP_W - xwidth - 5, DISP_H - 2, TFT_BLACK);
}
    }
  }
}

// Vẽ bàn phím
void drawKeypad() {
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t b = col + row * 3;
      if (b < 3) tft.setFreeFont(LABEL1_FONT);
      else tft.setFreeFont(LABEL2_FONT);

      key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
                        KEY_Y + row * (KEY_H + KEY_SPACING_Y),
                        KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
                        keyLabel[b], KEY_TEXTSIZE);
      key[b].drawButton();
    }
  }
}

// Hiệu chỉnh cảm ứng
void touch_calibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  if (!SPIFFS.begin()) {
    SPIFFS.format();
    SPIFFS.begin();
  }

  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (!REPEAT_CAL) {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f && f.readBytes((char *)calData, 14) == 14) calDataOK = 1;
      f.close()
      ;
    } else {
      SPIFFS.remove(CALIBRATION_FILE);
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    tft.setTouch(calData);
  } else {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Touch corners as indicated");
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

// Hiển thị trạng thái
void status(const char *msg) {
  tft.setTextPadding(200);
  tft.setTextColor(TFT_YELLOW, TFT_DARKGREY);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(msg, STATUS_X, STATUS_Y);
}```
````
