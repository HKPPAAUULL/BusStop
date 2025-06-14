#ifndef ASYNC_WEB_PORTAL_H
#define ASYNC_WEB_PORTAL_H
#include <Preferences.h>

extern Preferences preferences;

void startWebServer();
void handleClient();
void setupRootHandler();
void setupSaveHandler();

#endif
