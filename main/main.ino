#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "WiFiManager.h"
#include "AsyncWebPortal.h"
#include "BusFetcher.h"
#include "WeatherFetcher.h"
#include "DisplayManager.h"
#include "TimeUtils.h"
#include "ButtonController.h"
#include "version.h"
#include "OTA_AutoUpdate.h"

#define BOOT_BUTTON_PIN 9
#define LONG_PRESS_DURATION 3000
ButtonController buttonController(BOOT_BUTTON_PIN, LONG_PRESS_DURATION);
unsigned long buttonPressStart = 0;
bool buttonWasPressed = false;
bool wifiSetted = false;
bool wifiConnected = false;
bool displayOn = false;
bool manualDisplayOff = false;

// 用戶自定義項目
String ssid = "";
String password = "";
String company = ""; //巴士公司
String busName = ""; // 站名
String stopID = ""; // 站名
std::vector<int> serviceDays = {}; // 顯示巴士資訊日
std::vector<int> weatherDays = {}; // 顯示天氣日
bool rotateScreen = false; //螢幕是否旋轉180度
// 巴士資訊顯示時間
int serviceStartHour = 0;
int serviceStartMinute = 1;
int serviceEndHour = 23;
int serviceEndMinute = 59;

// 天氣資訊顯示時間
int wStartHour = 0;
int wStartMinute = 1;
int wEndHour = 23;
int wEndMinute = 59;

// 班次更新控制項
unsigned long lastBusUpdate = 0;
unsigned long lastWeatherUpdate = 0;
const unsigned long busUpdateInterval = 60000;       // 1 分鐘
const unsigned long weatherUpdateInterval = 10800000; // 3 小時

// 輪流顯示控制項
unsigned long lastSwitch = 0;
const unsigned long switchInterval = 8000;
const unsigned long weatherBlankDuration = 2000; // 清屏時間
unsigned long lastWeatherCycleTime = 0;
bool weatherDisplayOn = true; // 控制顯示或清屏
String lastDisplayedText = "";
BusTimes currentTimes = {"--", "--"};
WeatherInfo currentWeather = {"--", "--"};
int displayStage = 0; // 0: 第一班, 1: 第二班, 2: 天氣

void setup() {
  Serial.begin(115200);
 
Serial.printf("Free heap before OTA: %d\n", ESP.getFreeHeap());
Serial.printf("Sketch size: %d\n", ESP.getSketchSize());
Serial.printf("Free sketch space: %d\n", ESP.getFreeSketchSpace());

  loadPreferences();
  initDisplay(rotateScreen);
  Serial.println("Main: version " + String(CURRENT_VERSION));
  printPreferences();

  if (ssid == "" || password == "") {
    // No WiFi credentials saved, start in AP mode
    wifiSetted = false;
    Serial.println("Main: No WiFi credentials saved");
    startAPMode();
    simpleMsg("WiFi SSID: Bus_Config", "Go to http://192.168.4.1");
  } else {
    // WiFi credentials found, connect to WiFi
    wifiSetted = true;
    // 嘗試連接WiFi
    simpleMsg("Connecting WiFi...");
    wifiConnected = connectToWiFi(ssid.c_str(), password.c_str());
    if (wifiConnected){
      simpleMsg("WiFi Connected.");
      delay(2000);
      simpleMsg("Current Version: " + String(CURRENT_VERSION), "Checking update...");
      if(checkUpdate()){
        delay(2000);
        simpleMsg("New version detected.", "Update via OTA...");
        delay(2000);
        simpleMsg("Updating via OTA... ", "Please do NOT power off!");
        if(doUpdate()){
          simpleMsg("Update successfully!", "Auto rebooting...");
          ESP.restart();
        } else {
          simpleMsg("Update failed.", "Reboot to try again.");
        }
      } else {
        simpleMsg("Already in latest version.", "Loading info...");
        delay(2000);
      }
      initTime();
      initButton();
      if (!isShowBusInfo() && !isShowWeatherInfo()){
        outOfServiceAlert();
        offDisplay();
        displayOn = false;
      }
    } else {
      // No WiFi credentials saved, start in AP mode
      startAPMode();
      simpleMsg("WiFi Connection failed", "Setup WiFi in AP mode");
      delay(5000);
      simpleMsg("WiFi SSID: Bus_Config", "Go to http://192.168.4.1");
    }
  }
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  handleClient();

  buttonController.update();

  if (wifiConnected) {
    bool showBus = isShowBusInfo();
    bool showWeather = isShowWeatherInfo();

    if ((showBus || showWeather) && !manualDisplayOff) {
      if (!displayOn) {
        onDisplay();
        displayOn = true;
      }

      // 更新班次資料
      if (showBus && (millis() - lastBusUpdate > busUpdateInterval || lastBusUpdate == 0)) {
        currentTimes = fetchBusTime(company, busName, stopID);
        lastBusUpdate = millis();
      }

      // 更新天氣資料
      if (showWeather && (millis() - lastWeatherUpdate > weatherUpdateInterval || lastWeatherUpdate == 0)) {
        currentWeather = fetchWeather();
        lastWeatherUpdate = millis();
      }

      // 控制顯示輪替
      int maxStage = (showBus && showWeather) ? 3 : (showBus ? 2 : 1);
      if (millis() - lastSwitch > switchInterval) {
        displayStage = (displayStage + 1) % maxStage;
        lastSwitch = millis();
      }

      // 顯示內容
      switch (displayStage) {
        case 0: // 顯示第一班巴士時間（或只有天氣資訊時顯示天氣）
          if (showBus) {
            String displayText = currentTimes.first;
            if (displayText != lastDisplayedText) {
              showBusInfo(busName, stopID, displayText, true);
              lastDisplayedText = displayText;
            }
          } else if (showWeather) { 
            // Referesh 防止烙印
            unsigned long currentMillis = millis();
            if (weatherDisplayOn && currentMillis - lastWeatherCycleTime >= switchInterval) {
              // 顯示完 8 秒，清除畫面
              clearDisplay();
              weatherDisplayOn = false;
              lastWeatherCycleTime = currentMillis;
              lastDisplayedText = ""; // 重設，確保下次會重新顯示
            } else if (!weatherDisplayOn && currentMillis - lastWeatherCycleTime >= weatherBlankDuration) {
              // 清屏完 2 秒，重新顯示天氣
              weatherDisplayOn = true;
              lastWeatherCycleTime = currentMillis;
            }

            if (weatherDisplayOn) {
              String weatherText = currentWeather.maxTemp + "C / " + currentWeather.minTemp + "C";
              if (weatherText != lastDisplayedText) {
                showWeatherInfo(currentWeather.maxTemp, currentWeather.minTemp);
                lastDisplayedText = weatherText;
              }
            }
          }
          break;
        case 1: // 顯示第二班巴士時間（只有在 showBus 為 true 時才會執行）
          if (showBus) {
            String displayText = currentTimes.second;
            if (displayText != lastDisplayedText) {
              showBusInfo(busName, stopID, displayText, false);
              lastDisplayedText = displayText;
            }
          }
          break;
        case 2: // 顯示天氣資訊（只有在 showWeather 為 true 且 showBus 也為 true 時才會執行）
          if (showWeather) {
            String weatherText = currentWeather.maxTemp + "C / " + currentWeather.minTemp + "C";
            if (weatherText != lastDisplayedText) {
              showWeatherInfo(currentWeather.maxTemp, currentWeather.minTemp);
              lastDisplayedText = weatherText;
            }
          }
          break;
      }
    } else {
      //關閉屏幕
      if (displayOn) {
        if (!manualDisplayOff) {
          outOfServiceAlert();
        }
        offDisplay();
        displayOn = false;
      }
    }
  }

  delay(10);
}

// 讀取Preferences中的用戶自定義項目
void loadPreferences() {
  Serial.println("Loading Preferences...");
  preferences.begin("user-config", false);

  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  busName = preferences.getString("route", "");
  stopID = preferences.getString("stop", "");
  company = preferences.getString("company", "");
  rotateScreen = preferences.getBool("rotateScreen", false);

  // 讀取服務時間
  serviceStartHour = preferences.getInt("startHour", 0);
  serviceStartMinute = preferences.getInt("startMinute", 1);
  serviceEndHour = preferences.getInt("endHour", 23);
  serviceEndMinute = preferences.getInt("endMinute", 59);
  
  wStartHour = preferences.getInt("wStartHour", 0);
  wStartMinute = preferences.getInt("wStartMinute", 1);
  wEndHour = preferences.getInt("wEndHour", 23);
  wEndMinute = preferences.getInt("wEndMinute", 59);

  // 讀取服務日
  String daysStr = preferences.getString("serviceDays", "");
  serviceDays.clear();
  int index = 0;
  while (index < daysStr.length()) {
    int commaIndex = daysStr.indexOf(',', index);
    if (commaIndex == -1) commaIndex = daysStr.length();
    int day = daysStr.substring(index, commaIndex).toInt();
    serviceDays.push_back(day);
    index = commaIndex + 1;
  }

  // 讀取天氣顯示日
  String weatherStr = preferences.getString("weatherDays", "");
  weatherDays.clear();
  index = 0;
  while (index < weatherStr.length()) {
    int commaIndex = weatherStr.indexOf(',', index);
    if (commaIndex == -1) commaIndex = weatherStr.length();
    int day = weatherStr.substring(index, commaIndex).toInt();
    weatherDays.push_back(day);
    index = commaIndex + 1;
  }
  
  preferences.end();
}

bool isShowBusInfo() {
  return isServiceDay(serviceDays) && 
         isServiceTime(serviceStartHour, serviceStartMinute, serviceEndHour, serviceEndMinute);
}

bool isShowWeatherInfo() {
  return isServiceDay(weatherDays) &&
         isServiceTime(wStartHour, wStartMinute, wEndHour, wEndMinute);
}

void outOfServiceAlert() {
  Serial.println("Main: Out of service");
  simpleMsg("Out of service time", "Screen off in 5 seconds");
  delay(6000);
  clearDisplay();
}

void initButton() {
  buttonController.begin();

  buttonController.onShortPress([]() {
    if (displayOn) {
        Serial.println("Main: Turn off display (manual)");
        manualDisplayOff = true;
        offDisplay();
        displayOn = false;
      } else {
        Serial.println("Main: Turn on display (manual)");
        manualDisplayOff = false;
        onDisplay();
        displayOn = true;
      }
  });

  buttonController.onLongPress([]() {
    Serial.println("Main: Clearing Preferences and reboot");
    preferences.begin("user-config", false);
    preferences.clear();
    preferences.end();
    delay(500);
    esp_restart();
  });
}

//【開發注意事項】
//開發版選擇ESP32 C6 Dev Module
//工具設定：NO OTA 2MB / 2MB
//Enable USB CDC On Boot

