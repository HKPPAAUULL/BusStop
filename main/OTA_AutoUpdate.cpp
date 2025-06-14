//OTA_AutoUpdate.cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include "version.h"

bool checkUpdate() {
  HTTPClient http;
  http.begin("https://hkppaauull.github.io/BusStop/version.txt");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String latestVersion = http.getString();
    latestVersion.trim();
    if (latestVersion != CURRENT_VERSION) {
      Serial.println("OTA: New version available: " + latestVersion);
      http.end();
      return true;
    } else {
      Serial.println("Already at latest version: " + String(CURRENT_VERSION));
    }
  } else {
    Serial.printf("Failed to check version. HTTP code: %d\n", httpCode);
  }

  http.end();
  return false;
}

bool doUpdate() {
  Serial.println("OTA: Starting update via github");
  
  WiFiClientSecure client;
  client.setInsecure();  // 忽略憑證驗證（僅限測試）

Serial.println("Client created, starting HTTP...");

  HTTPClient http;
  http.begin(client, "https://hkppaauull.github.io/BusStop/main.ino.bin");

Serial.println("HTTP begin done");

  int httpCode = http.GET();
  Serial.printf("HTTP code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    if (contentLength <= 0) {
      Serial.println("Invalid content length");
      http.end();
      return false;
    }

    if (!Update.begin(contentLength)) {
      Serial.println("Not enough space for OTA");
      http.end();
      return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);

    if (Update.end() && Update.isFinished()) {
      Serial.println("OTA: Update successful, restarting...");
      delay(2000);
      return true;
    } else {
      Serial.printf("OTA: Update failed. Error #: %d\n", Update.getError());
    }
  } else {
    Serial.printf("OTA: HTTP error: %d\n", httpCode);
  }

  http.end();
  return false;
}


