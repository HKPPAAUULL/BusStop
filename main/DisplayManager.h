#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "Display_ST7789.h"
#include "font5x7.h"
#include "font16x16_zh.h"

void onDisplay();
void offDisplay();
void initDisplay(bool rotateScreen);
void clearDisplay();
void simpleMsg(const String& displayText);
void simpleMsg(const String& displayText1, const String& displayText2);
void showBusInfo(const String& route, const String& stopName, const String& displayText, bool isFirst);
void showWeatherInfo(const String& maxTemp, const String& minTemp);
void printPreferences();
void rotationScreen180();

#endif
