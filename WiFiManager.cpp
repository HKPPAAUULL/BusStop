#include <WiFi.h>
#include "WiFiManager.h"
#include "AsyncWebPortal.h"
#include "BusFetcher.h"

bool connectToWiFi(const char* ssid, const char* password, unsigned long timeoutMs) {
  Serial.printf("WiFiManager: Connecting to WiFi SSID: %s\n", ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeoutMs) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFiManager: WiFi connected");
      return true;
    } else {
      Serial.println("\nWiFiManager: WiFi connection failed");
      return false;
    }
  }

void startAPMode() {
  WiFi.softAP("BusStop_Config", "");
  Serial.println("WiFiManager: AP Mode started. Connect to WiFi 'BusStop_Config' and go to http://192.168.4.1");
  startWebServer();
}
