#ifndef WEATHER_FETCHER_H
#define WEATHER_FETCHER_H

#include <Arduino.h>

// 結構用來儲存天氣資訊
struct WeatherInfo {
  String maxTemp;
  String minTemp;
};

// 函式宣告：從天文台 API 擷取天氣資料
WeatherInfo fetchWeather();

#endif
