#include "DisplayManager.h"
#include <Preferences.h>


// 畫單一像素
void LCD_addPixel(uint16_t x, uint16_t y, uint16_t color) {
  LCD_SetCursor(x, y, x, y);
  LCD_WriteData_Word(color);
}

// 填滿矩形區域
void LCD_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  for (uint16_t i = 0; i < h; i++) {
    for (uint16_t j = 0; j < w; j++) {
      LCD_addPixel(x + j, y + i, color);
    }
  }
}

// 畫英文字元（5x7）
void DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size) {
  if (c < 32 || c > 126) return;

  for (uint8_t i = 0; i < 5; i++) {
    uint8_t line = font5x7[c - 32][i];
    for (uint8_t j = 0; j < 8; j++) {
      uint16_t pixelColor = (line & 0x01) ? color : bg;
      if (size == 1) {
        LCD_addPixel(x + (7 - j), y + i, pixelColor);
      } else {
        LCD_fillRect(x + (7 - j) * size, y + i * size, size, size, pixelColor);
      }
      line >>= 1;
    }
  }
}

// 畫英文字串
void DrawString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg, uint8_t size) {
  while (*str) {
    DrawChar(x, y, *str, color, bg, size);
    y += 6 * size;
    str++;
  }
}

// 畫中文字元（16x16）
void DrawChineseChar16x16(uint16_t x, uint16_t y, const uint8_t* charData, uint16_t color, uint16_t bg, uint8_t size) {
  for (uint8_t row = 0; row < 16; row++) {
    uint8_t byte1 = charData[row * 2];
    uint8_t byte2 = charData[row * 2 + 1];

    for (uint8_t col = 0; col < 8; col++) {
      uint16_t pixelColor = (byte1 & (0x80 >> col)) ? color : bg;
      if (size == 1) {
        LCD_addPixel(x + (15 - row), y + col, pixelColor);
      } else {
        LCD_fillRect(x + (15 - row) * size, y + col * size, size, size, pixelColor);
      }
    }

    for (uint8_t col = 0; col < 8; col++) {
      uint16_t pixelColor = (byte2 & (0x80 >> col)) ? color : bg;
      if (size == 1) {
        LCD_addPixel(x + (15 - row), y + 8 + col, pixelColor);
      } else {
        LCD_fillRect(x + (15 - row) * size, y + (8 + col) * size, size, size, pixelColor);
      }
    }
  }
}

// 開啟 LCD 背光
void onDisplay() {
  Set_Backlight(10);
}

// 關閉 LCD 背光
void offDisplay() {
  Set_Backlight(0);
}

// 初始化 LCD
void initDisplay(bool rotateScreen) {
  LCD_Init();
  if (rotateScreen) {
    rotationScreen180();
  }
  clearDisplay();
}

// 清除畫面
void clearDisplay() {
  //LCD_fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, 0x0000);  
  static uint16_t lineBuffer[LCD_WIDTH] = {0};
  for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
    LCD_addWindow(0, y, LCD_WIDTH - 1, y, lineBuffer);
  }
}

// 顯示一行簡單資訊
void simpleMsg(const String& displayText) {
  clearDisplay();
  DrawString(130, 10, displayText.c_str(), 0xFFFF, 0x0000, 2);
}

// 顯示兩行簡單資訊
void simpleMsg(const String& displayText1, const String& displayText2) {
  clearDisplay();
  DrawString(130, 10, displayText1.c_str(), 0xFFFF, 0x0000, 2);
  DrawString(90, 10, displayText2.c_str(), 0xFFFF, 0x0000, 2);
}

// 顯示巴士班次資訊
void showBusInfo(const String& route, const String& stopName, const String& displayText, bool isFirst) {
  clearDisplay();

  if(isFirst){
    //文字：[路線編號] 到站時間
    DrawString(90, 10, route.c_str(), 0xFFFF, 0x0000, 5.5);
    DrawChineseChar16x16(90, 110, chineseChar_1, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(90, 160, chineseChar_2, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(90, 210, chineseChar_3, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(90, 260, chineseChar_4, 0xFFFF, 0x0000, 3);
  } else {
    //文字：再下一班
    DrawChineseChar16x16(90, 10, chineseChar_8, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(90, 70, chineseChar_9, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(90, 130, chineseChar_10, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(90, 190, chineseChar_11, 0xFFFF, 0x0000, 3);
  }
  
  if (displayText == "Departing") {
    //文字：已到站
    DrawChineseChar16x16(30, 10, chineseChar_7, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(30, 70, chineseChar_1, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(30, 130, chineseChar_2, 0xFFFF, 0x0000, 3);
  } else if (displayText.length() == 1 && displayText.toInt() > 0) {
    //文字：[個位數值] 分鐘
    DrawString(30, 10, displayText.c_str(), 0xFFFF, 0x0000, 5);
    DrawChineseChar16x16(30 , 40, chineseChar_5, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(30 , 100, chineseChar_6, 0xFFFF, 0x0000, 3);
  } else if (displayText.length() == 2 && displayText.toInt() > 0) {
    //文字：[雙位數值] 分鐘
    DrawString(30, 10, displayText.c_str(), 0xFFFF, 0x0000, 5);
    DrawChineseChar16x16(30 , 90, chineseChar_5, 0xFFFF, 0x0000, 3);
    DrawChineseChar16x16(30 , 150, chineseChar_6, 0xFFFF, 0x0000, 3);
  } else {
    //Error Msg
    DrawString(30, 10, displayText.c_str(), 0x07E0, 0x0000, 3);
  }
}

// 顯示天氣資訊
void showWeatherInfo(const String& maxTemp, const String& minTemp) {
  clearDisplay();

  //文字：最高溫度 [maxTemp]
  DrawChineseChar16x16(90, 10, chineseChar_12, 0xFFFF, 0x0000, 3);
  DrawChineseChar16x16(90, 70, chineseChar_15, 0xFFFF, 0x0000, 3);
  DrawChineseChar16x16(90, 130, chineseChar_13, 0xFFFF, 0x0000, 3);
  DrawChineseChar16x16(90, 190, chineseChar_14, 0xFFFF, 0x0000, 3);
  DrawString(90, 260, maxTemp.c_str(), 0xFFFF, 0x0000, 5.5);

  //文字：最低溫度 [maxTemp]
  DrawChineseChar16x16(30, 10, chineseChar_12, 0xFFFF, 0x0000, 3);
  DrawChineseChar16x16(30, 70, chineseChar_16, 0xFFFF, 0x0000, 3);
  DrawChineseChar16x16(30, 130, chineseChar_13, 0xFFFF, 0x0000, 3);
  DrawChineseChar16x16(30, 190, chineseChar_14, 0xFFFF, 0x0000, 3);
  DrawString(30, 260, minTemp.c_str(), 0xFFFF, 0x0000, 5.5);
}

// For Debug
void printPreferences() {
  Preferences prefs;
  prefs.begin("user-config", true); // read-only

  Serial.println("=== Preferences Dump ===");
  Serial.println("WiFi SSID: " + prefs.getString("ssid", "(not set)"));
  Serial.println("WiFi Password: " + prefs.getString("password", "(not set)"));
  Serial.println("Company: " + prefs.getString("company", "(not set)"));
  Serial.println("Route: " + prefs.getString("route", "(not set)"));
  Serial.println("Stop: " + prefs.getString("stop", "(not set)"));
  Serial.println("Service Days: " + prefs.getString("serviceDays", "(not set)"));
  Serial.printf("Service Start Time: %02d:%02d\n", prefs.getInt("startHour", -1), prefs.getInt("startMinute", -1));
  Serial.printf("Service End Time: %02d:%02d\n", prefs.getInt("endHour", -1), prefs.getInt("endMinute", -1));
  Serial.println("Weather Days: " + prefs.getString("weatherDays", "(not set)"));
  Serial.printf("Weather Start Time: %02d:%02d\n", prefs.getInt("wStartHour", -1), prefs.getInt("wStartMinute", -1));
  Serial.printf("Weather End Time: %02d:%02d\n", prefs.getInt("wEndHour", -1), prefs.getInt("wEndMinute", -1));
  Serial.println("Rotate Screen: " + prefs.getBool("rotateScreen", "(not set)"));
  Serial.println("========================");

  prefs.end();
}

void rotationScreen180(){
  LCD_SetRotation(180);
}



