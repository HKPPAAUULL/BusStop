#include "TimeUtils.h"
#include <time.h>
#include <vector>


// 初始化 NTP 時間，同步至香港時區（UTC+8）
void initTime() {
  configTime(28800, 0, "pool.ntp.org"); // 28800 秒 = 8 小時
}


// 判斷是否為服務日（星期幾）
bool isServiceDay(const std::vector<int>& allowedDays) {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("TimeUtils: Failed to obtain time");
    return true; // 若無法取得時間，預設繼續顯示
  }

  int weekday = timeinfo.tm_wday; // 星期日=0，星期一=1，...，星期六=6
  
  // 檢查今天是否在允許的星期列表中
  for (int day : allowedDays) {
    if (weekday == day) {
      return true;
    }
  }

  return false;
}


// 判斷是否為服務時間
bool isServiceTime(int startHour, int startMinute, int endHour, int endMinute) {
  struct tm timeinfo;
  
  // 嘗試取得本地時間
  if (!getLocalTime(&timeinfo)) {
    Serial.println("TimeUtils: Failed to obtain time");
    return true; // 若無法取得時間，預設繼續顯示
  }
  // 取得目前小時與分鐘
  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  
  // 計算用戶自定義時間
  int startMinutes = startHour * 60 + startMinute;
  int endMinutes = endHour * 60 + endMinute;

  // 判斷是否在服務時間之間
  return (currentMinutes >= startMinutes && currentMinutes <= endMinutes);

}
