#pragma once

#include "constants.h"

#ifdef ENABLE_SERVER

#include <ESPAsyncWebServer.h>

#include "PluginManager.h"
#include "screen.h"
#include "secrets.h"
#include "signs.h"

// Wireless firmware updates without a page: POST the firmware as a multipart
// file to /api/firmware with the OTA_USERNAME / OTA_PASSWORD credentials
// (digest auth), see upload.py. GET /api/firmware checks the credentials.
void initOTA(AsyncWebServer &server);
// restarts into the new firmware shortly after a successful update
void otaLoop();

#endif
