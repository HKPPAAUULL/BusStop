
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

bool connectToWiFi(const char* ssid, const char* password, unsigned long timeoutMs = 15000);
void startAPMode();

#endif
