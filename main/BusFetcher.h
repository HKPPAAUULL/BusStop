#ifndef BUS_FETCHER_H
#define BUS_FETCHER_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

struct BusTimes {
  String first;
  String second;
};

BusTimes fetchBusTime(const String& busCompany, const String& busName, const String& stopName);

#endif
