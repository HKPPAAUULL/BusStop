#pragma once
#include <map>
#include <vector>
#include <Arduino.h>

struct StopInfo {
  String code;
  String alias;
};

struct DirectionInfo {
  String direction; // 方向名稱，例如 "往天水圍方向"
  std::vector<StopInfo> stops;
};

struct RouteInfo {
  String code;  // 路線號碼，例如 "69"
  std::vector<DirectionInfo> directions;
};

extern std::vector<RouteInfo> KMB_Routes;
extern std::vector<RouteInfo> Citybus_Routes;
extern std::vector<RouteInfo> MTR_Routes;

