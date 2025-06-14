// BusFetcher.cpp
#include <WiFi.h>
#include <time.h>
#include "BusFetcher.h"

// 宣告private static function
static BusTimes callApi_MTR(const String& busName, const String& stopID);
static BusTimes callApi_KMB(const String& busName, const String& stopID);
static time_t parseISOTime(const char* isoTime);

BusTimes fetchBusTime(const String& busCompany, const String& busName, const String& stopID){
  if (busCompany == "MTR") {
    return callApi_MTR(busName, stopID);
  } else if (busCompany == "KMB") {
    return callApi_KMB(busName, stopID);
  } else {
    Serial.println("BusFetcher: Incorrect bus company");
    return BusTimes{"--", "--"};
  }
}

// 港鐵巴士公司API
static BusTimes callApi_MTR(const String& busName, const String& stopID) {
  Serial.println("BusFetcher: Fetching MTR bus time...");
  const char* apiUrl = "https://rt.data.gov.hk/v1/transport/mtr/bus/getSchedule";
  String payload = "{\"language\":\"en\",\"routeName\":\"K66\"}";
  payload = "{\"language\":\"en\",\"routeName\":\"" + busName +"\"}";
  BusTimes result = {"--", "--"};

  // 確保 WiFi 已連線
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // 忽略 SSL 憑證驗證（政府 API 沒有正式憑證）

    HTTPClient http;
    http.begin(client, apiUrl); // 初始化 HTTPS 請求
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32Client");
    http.setTimeout(5000);
    int httpResponseCode = http.POST(payload); // 發送 POST 請求

    if (httpResponseCode == 200) {
      String response = http.getString(); // 取得回應字串

      if (response.length() == 0) {
        Serial.println("BusFetcher: Empty response from server.");
        result.first = "Empty Response";
        http.end();
        return result;
      }

      // 建立 JSON 過濾器，只保留需要的欄位
      StaticJsonDocument<512> filter;
      JsonObject busStopFilter = filter["busStop"].createNestedObject();
      busStopFilter["busStopId"] = true;
      busStopFilter["bus"] = true;

      // 解析 JSON 回應
      DynamicJsonDocument doc(24576); // 使用 24KB 空間
      DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(filter));

      if (error) {
        Serial.println("BusFetcher: JSON Decode Error");
        Serial.println(error.c_str());
        result.first = "JSON Error";
      } else {
        JsonArray busStops = doc["busStop"];
        bool found = false;
        // 遍歷所有站點
        for (JsonObject stop : busStops) {
          const char* stopId = stop["busStopId"];
          if (stopId && strcmp(stopId, stopID.c_str()) == 0) {
            found = true;
            JsonArray buses = stop["bus"];
            if(!buses.isNull() && buses.size() > 0) {    
              for (size_t i = 0; i < (buses.size() < 2 ? buses.size() : 2); i++) {
                const char* timeText = buses[i]["departureTimeText"];
                String timeStr = String(timeText); 

                // 處理時間格式
                if (timeStr.endsWith("minutes") || timeStr.endsWith("minute")) {
                  int spaceIndex = timeStr.indexOf(' ');
                  if (spaceIndex > 0) {
                    int minutes = timeStr.substring(0, spaceIndex).toInt();
                    minutes = max(0, minutes - 1); // 減 1 分鐘，避免顯示延遲
                    timeStr = (minutes == 0) ? "Departing" : String(minutes);
                  }
                } else if (timeStr.equalsIgnoreCase("Departing / Departed")) {
                  timeStr  = "Departing";
                }
                // 其他格式直接顯示
                if (i == 0) result.first = timeStr;
                else result.second = timeStr;
              } 
            } else {
              // 沒有班次資料
              result.first = "No Time Info";
            }
            break;
          }
        }
        if (!found) {
          // 沒有巴士站資料
          result.first = "No Stop";
        }
      }
    } else {
      // HTTP 錯誤
      Serial.print("BusFetcher: HTTP Error:" + httpResponseCode);
      result.first = "HTTP Error";
    }
    http.end(); // 關閉連線
  } else {
    // WiFi 未連線
    Serial.println("BusFetcher: WiFi Connection Loss");
    result.first = "WiFi Error";
  }
  Serial.printf("BusFetcher: Next buses: %s, %s\n", result.first.c_str(), result.second.c_str());
  return result;
}

// 九巴公司API
static BusTimes callApi_KMB(const String& busName, const String& stopID) {
  Serial.println("BusFetcher: Fetching KMB bus time...");
  String apiUrl = "http://data.etabus.gov.hk/v1/transport/kmb/eta/" + stopID + "/" + busName + "/1";
  BusTimes result = {"--", "--"};

  // 確保 WiFi 已連線
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl); // 初始化 HTTPS 請求
    http.setTimeout(5000);
    int httpResponseCode = http.GET(); // 發送 GET 請求

    if (httpResponseCode == 200) {
      String response = http.getString(); // 取得回應字串
      DynamicJsonDocument doc(8192);
      DeserializationError error = deserializeJson(doc, response);

      if (response.length() == 0) {
        Serial.println("BusFetcher: Empty response from server.");
        result.first = "Empty Response";
        http.end();
        return result;
      }

      if (error) {
        Serial.println("BusFetcher: JSON Decode Error");
        Serial.println(error.c_str());
        result.first = "JSON Error";
      } else {
        JsonArray data = doc["data"];
        int count = 0;
        // 遍歷所有站點
        for (JsonObject item : data) {
          if (count >= 2) break;

          const char* etaStr = item["eta"];
          if (etaStr && strlen(etaStr) > 0) {
            time_t etaTime = parseISOTime(etaStr);
            time_t now = time(nullptr);
            int diffMin = (etaTime - now) / 60;
            String timeStr;
            if (diffMin <= 0) timeStr = "Departing";
            else timeStr = String(diffMin);
            if (count == 0) result.first = timeStr;
            else result.second = timeStr;
            count++;
          }
        }
        if (count == 0) {
          // 沒有到站資料
          result.first = "No ETA";
        }
      }
    } else {
      // HTTP 錯誤
      Serial.print("BusFetcher: HTTP Error:" + httpResponseCode);
      result.first = "HTTP Error";
    }
    http.end(); // 關閉連線
  } else {
    // WiFi 未連線
    Serial.println("BusFetcher: WiFi Connection Loss");
    result.first = "WiFi Error";
  }
  Serial.printf("BusFetcher: Next buses: %s, %s\n", result.first.c_str(), result.second.c_str());
  return result;
}

static time_t parseISOTime(const char* isoTime) {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  strptime(isoTime, "%Y-%m-%dT%H:%M:%S", &tm);
  return mktime(&tm);
}

