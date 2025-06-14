#include <Arduino.h>
#include <ESPAsyncWebServer.h> 
#include <Preferences.h>
#include "AsyncWebPortal.h"
#include "BusData.h"
#include <Ticker.h>

AsyncWebServer server(80);
Preferences preferences;
Ticker restartTimer;

String getBusData(const std::vector<RouteInfo>& routes) {
  String jsBusStops = "{\n";
  for (size_t i = 0; i < routes.size(); ++i) {
    const auto& route = routes[i];
    jsBusStops += "  \"" + route.code + "\": {\n";
    for (size_t d = 0; d < route.directions.size(); ++d) {
      const auto& dir = route.directions[d];
      jsBusStops += "    \"" + dir.direction + "\": {\n";
      jsBusStops += "      \"stops\": {\n";
      for (size_t j = 0; j < dir.stops.size(); ++j) {
        jsBusStops += "        \"" + dir.stops[j].code + "\": \"" + dir.stops[j].alias + "\"";
        if (j < dir.stops.size() - 1) jsBusStops += ",\n";
      }
      jsBusStops += "\n      }\n    }";
      if (d < route.directions.size() - 1) jsBusStops += ",\n";
    }
    jsBusStops += "\n  }";
    if (i < routes.size() - 1) jsBusStops += ",\n";
  }
  jsBusStops += "\n}";
  return jsBusStops;
}


//Seting 頁面
void handleRoot(AsyncWebServerRequest *request) {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <title>巴士資訊牌設定</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">  
    <style>
      body {
        font-family: Arial, sans-serif;
        padding: 20px;
        font-size: 18px;
        background-color: #f9f9f9;
      }
      h1 {
        text-align: center;
        color: #333;
      }
      form {
        max-width: 400px;
        margin: auto;
      }
      select, input[type='submit'] {
        width: 100%;
        padding: 12px;
        margin: 8px 0;
        box-sizing: border-box;
        font-size: 16px;
      }
      input[type='submit'] {
        background-color: #4CAF50;
        color: white;
        border: none;
        cursor: pointer;
      }
      input[type='submit']:hover {
        background-color: #45a049;
      }
    </style>
  </head>
  <body>
    <h1>巴士資訊牌設定</h1>
    <form action='/save' method='POST'>
      <p>
        <label for="ssid">WiFi SSID:</label>
        <input type='text' id="ssid" name='ssid' required><br>
      </p>
      <p>
        <label for="password">WiFi 密碼:</label>
        <input type='password' id="password" name='password' required><br>
      </p>

      <select id="company" name="company" onchange="updateRoutes()">
        <option value="KMB">九巴</option>
        <option value="Citybus">城巴</option>
        <option value="MTR">港鐵巴士</option>
      </select>

      <label for="route">巴士路線:</label>
      <select id="route" name="route" onchange="updateDirections()">
        <!-- 將由 JavaScript 動態填入 -->
      </select>

      <label for="direction">方向:</label>
      <select id="direction" name="direction" onchange="updateStops()">
        <!-- 將由 JavaScript 動態填入 -->
      </select>

      <label for="stop">巴士站:</label>
      <select id="stop" name="stop">
        <!-- 將由 JavaScript 動態填入 -->
      </select>

      <label for="serviceDays">巴士資訊顯示日（可多選）:</label>
      <select id="serviceDays" name="serviceDays" multiple>
        <option value="1">星期一</option>
        <option value="2">星期二</option>
        <option value="3">星期三</option>
        <option value="4">星期四</option>
        <option value="5">星期五</option>
        <option value="6">星期六</option>
        <option value="7">星期日</option>
      </select>

      <p>
        <label for="startTime">巴士資訊開始時間:</label>
        <input type="time" id="startTime" name="startTime">
      </p>

      <p>
        <label for="endTime">巴士資訊結束時間:</label>
        <input type="time" id="endTime" name="endTime">
      </p>

      <p>氣溫預告於中午12點前顯示今天資訊, 12點後顯示明天資訊</p>

      <label for="weatherDays">氣溫顯示日（可多選）:</label>
      <select id="weatherDays" name="weatherDays" multiple>
        <option value="1">星期一</option>
        <option value="2">星期二</option>
        <option value="3">星期三</option>
        <option value="4">星期四</option>
        <option value="5">星期五</option>
        <option value="6">星期六</option>
        <option value="7">星期日</option>
      </select>

      <p>
        <label for="weatherStartTime">氣溫預告開始時間:</label>
        <input type="time" id="weatherStartTime" name="weatherStartTime">
      </p>

      <p>
        <label for="weatherEndTime">氣溫預告結束時間:</label>
        <input type="time" id="weatherEndTime" name="weatherEndTime">
      </p>
      
      <p>
        <label for="rotateScreen">180度反轉螢幕:</label>
        <input type="checkbox" id="rotateScreen" name="rotateScreen" value="true">
      </p>

      <input type='submit' value='儲存'>
    </form>

    <script> 
  )rawliteral";

  // 插入BusData.h 的巴士班次資料
  // html += getBusData(); 
  html += "const allBusData = {\n";
  html += "  \"KMB\": " + getBusData(KMB_Routes) + ",\n";
  html += "  \"Citybus\": " + getBusData(Citybus_Routes) + ",\n";
  html += "  \"MTR\": " + getBusData(MTR_Routes) + "\n";
  html += "};\n";

  html += R"rawliteral(

    console.log("allBusData:", allBusData);

    // 更新路線選單
    function updateRoutes() {
      const company = document.getElementById("company").value;
      const routeSelect = document.getElementById("route");
      const directionSelect = document.getElementById("direction");
      const stopSelect = document.getElementById("stop");
      routeSelect.innerHTML = "";
      directionSelect.innerHTML = "";
      stopSelect.innerHTML = "";

      const routes = allBusData[company];
      for (const routeCode in routes) {
        const option = document.createElement("option");
        option.value = routeCode;
        option.text = routeCode;
        routeSelect.appendChild(option);
      }

      updateDirections();
    }

    function updateDirections() {
      const company = document.getElementById("company").value;
      const routeCode = document.getElementById("route").value;
      const directionSelect = document.getElementById("direction");
      const stopSelect = document.getElementById("stop");

      directionSelect.innerHTML = "";
      stopSelect.innerHTML = "";

      const directions = allBusData[company][routeCode];
      for (const directionName in directions) {
        const option = document.createElement("option");
        option.value = directionName;
        option.text = directionName;
        directionSelect.appendChild(option);
      }

      updateStops();
    }

    function updateStops() {
      const company = document.getElementById("company").value;
      const routeCode = document.getElementById("route").value;
      const directionName = document.getElementById("direction").value;
      const stopSelect = document.getElementById("stop");
      stopSelect.innerHTML = "";

      const stops = allBusData[company][routeCode][directionName].stops;
      for (const stopCode in stops) {
        const option = document.createElement("option");
        option.value = stopCode;
        option.text = stops[stopCode];
        stopSelect.appendChild(option);
      }
    }

    // 頁面載入時初始化
    window.onload = () => {
      updateRoutes();
      document.getElementById("route").addEventListener("change", updateDirections);
      document.getElementById("direction").addEventListener("change", updateStops);
    };
  </script>
  </body>
  </html>
  )rawliteral";

  request->send(200, "text/html", html);
}

void restartESP() {
  ESP.restart();
}

// 處理表單提交的結果
void handleSave(AsyncWebServerRequest *request) {
  String ssid = request->arg("ssid");
  String password = request->arg("password"); 
  String route = request->arg("route");
  String stop = request->arg("stop");
  String company = request->arg("company");
  String startTime = request->arg("startTime"); // 格式 "HH:MM"
  String endTime = request->arg("endTime");
  String weatherStartTime = request->arg("weatherStartTime"); // 格式 "HH:MM"
  String weatherEndTime = request->arg("weatherEndTime");
  bool rotateScreen = (request->arg("rotateScreen") == "true");

  int startHour = startTime.substring(0, 2).toInt();
  int startMinute = startTime.substring(3, 5).toInt();
  int endHour = endTime.substring(0, 2).toInt();
  int endMinute = endTime.substring(3, 5).toInt();
  
  int wStartHour = weatherStartTime.substring(0, 2).toInt();
  int wStartMinute = weatherStartTime.substring(3, 5).toInt();
  int wEndHour = weatherEndTime.substring(0, 2).toInt();
  int wEndMinute = weatherEndTime.substring(3, 5).toInt();

  preferences.begin("user-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("company", company);
  preferences.putString("route", route);
  preferences.putString("stop", stop); 
  preferences.putInt("startHour", startHour);
  preferences.putInt("startMinute", startMinute);
  preferences.putInt("endHour", endHour);
  preferences.putInt("endMinute", endMinute);
  preferences.putInt("wStartHour", wStartHour);
  preferences.putInt("wStartMinute", wStartMinute);
  preferences.putInt("wEndHour", wEndHour);
  preferences.putInt("wEndMinute", wEndMinute);
  preferences.putBool("rotateScreen", rotateScreen);

  // 處理 serviceDays 多選 
  String serviceDaysStr = "";
  for (int i = 0; i < request->params(); i++) {
    if (request->getParam(i)->name() == "serviceDays") {
      serviceDaysStr += request->getParam(i)->value() + ",";
    }
  }
  if (serviceDaysStr.endsWith(",")) {
    serviceDaysStr.remove(serviceDaysStr.length() - 1);
  }
  preferences.putString("serviceDays", serviceDaysStr);

  // 儲存 weatherDays
  String weatherDaysStr = "";
  for (int i = 0; i < request->params(); i++) {
    if (request->getParam(i)->name() == "weatherDays") {
      weatherDaysStr += request->getParam(i)->value() + ",";
    }
  }
  if (weatherDaysStr.endsWith(",")) {
    weatherDaysStr.remove(weatherDaysStr.length() - 1);
  }
  preferences.putString("weatherDays", weatherDaysStr);

  preferences.end();


  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <title>Settings Saved</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">  
      <style>
        body {
          font-family: Arial, sans-serif;
          padding: 20px;
          font-size: 18px;
          background-color: #f9f9f9;
        }
        h1 {
          text-align: center;
          color: #333;
        }
        p {
          text-align: center;
        }
        form {
          max-width: 400px;
          margin: auto;
        }
        input[type='submit'] {
          width: 100%;
          padding: 12px;
          margin: 8px 0;
          box-sizing: border-box;
          font-size: 16px;
          background-color: #4CAF50;
          color: white;
          border: none;
          cursor: pointer;
        }
        input[type='submit']:hover {
          background-color: #45a049;
        }
      </style>
    </head>
    <body>
      <h1>Settings Saved</h1>
      <p>Rebooting...</p>
      <form action="/" method="GET">
        <input type="submit" value="返回首頁">
      </form>
    </body>
    </html>
  )rawliteral";

  request->send(200, "text/html", html);
  restartTimer.once(5, restartESP);
}

void startWebServer() {
  setupRootHandler();
  setupSaveHandler();
  server.begin();
  Serial.println("AsyncWebPortal: Web server started");
}

void setupRootHandler() {
  server.on("/", HTTP_GET, handleRoot);
}


void setupSaveHandler() {
  // 正常程序save (POST)
  server.on("/save", HTTP_POST, handleSave);

  // 不正常程序進入/save (GET)
  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
      <meta charset="UTF-8">
      <title>無效訪問</title>
      </head>
      <body>
        <h1>無效訪問</h1>
        <p>此頁面僅可透過表單提交時存取。</p>
        <form action="/" method="GET">
          <input type="submit" value="返回首頁">
        </form>
      </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });
}


void handleClient() {
  // AsyncWebServer handles clients in the background
}
