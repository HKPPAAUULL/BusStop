#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>

// 初始化 NTP 時間（香港時區）
void initTime();

// 判斷是否為服務日(星期幾)
bool isServiceDay(const std::vector<int>& allowedDays);

// 判斷是否為服務時間
bool isServiceTime(int startHour, int startMinute, int endHour, int endMinute);

#endif
