#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "WeatherFetcher.h"

const char* weatherApiUrl = "https://data.weather.gov.hk/weatherAPI/opendata/weather.php?dataType=fnd&lang=en";

WeatherInfo fetchWeather() {
  Serial.println("WeatherFetcher: Fetching weather...");
  WeatherInfo result = {"--", "--"};

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // 忽略 SSL 憑證

    HTTPClient http;
    http.begin(client, weatherApiUrl);
    http.setTimeout(5000);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String response = http.getString();
      
      // 建立 JSON 過濾器
      StaticJsonDocument<512> filter;
      JsonObject forecast0 = filter["weatherForecast"][0].to<JsonObject>(); 
      forecast0["forecastMaxtemp"]["value"] = true;
      forecast0["forecastMintemp"]["value"] = true;

      // 解析 JSON
      DynamicJsonDocument doc(4096); // 根據實測大小調整
      DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));

      if (error) { //要留
        Serial.print("WeatherFetcher: JSON Error: ");
        Serial.println(error.c_str());
        result.maxTemp = "JSON Error";
      } else {
        JsonArray forecasts = doc["weatherForecast"]; 
        
        if (!forecasts.isNull() && forecasts.size() > 0) {
          JsonObject today = forecasts[0];
          Serial.println("WeatherFetcher: Today's forecast found.");
          
          // 取得 maxTemp
          if (today["forecastMaxtemp"]["value"].is<int>()) {
            result.maxTemp = String(today["forecastMaxtemp"]["value"].as<int>());
          } else {
            result.maxTemp = "--";
          }
          // 取得 minTemp
          if (today["forecastMintemp"]["value"].is<int>()) {
            result.minTemp = String(today["forecastMintemp"]["value"].as<int>());
          } else {
            result.minTemp = "--";
          }
        }
      }
    } else {
      Serial.printf("WeatherFetcher: HTTP Error: %d\n", httpCode);
      result.maxTemp = "HTTP Error";
    }
    http.end();
  } else {
    Serial.println("WeatherFetcher: WiFi not connected");
    result.maxTemp = "WiFi Error";
  }

  Serial.printf("WeatherFetcher: Max: %s, Min: %s\n", result.maxTemp.c_str(), result.minTemp.c_str());
  return result;
}
