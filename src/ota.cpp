#include "ota.h"

#ifdef ENABLE_SERVER

#ifdef ESP32
#include <Update.h>
#else
#include <Updater.h>
#endif

namespace
{
// one update at a time; set by the upload handler, read by the request handler
bool updateStarted = false;
bool updateSucceeded = false;
bool unauthorized = false;
String updateError;
unsigned long progressMillis = 0;
unsigned long restartAt = 0;

bool isAuthorized(AsyncWebServerRequest *request)
{
  return request->authenticate(OTA_USERNAME, OTA_PASSWORD);
}

void showUpdating()
{
  currentStatus = UPDATE;
  Screen.clear();
  Screen.drawBitmap(0, 0, letterU, sizeof(letterU), COLS, MAX_BRIGHTNESS);
}

void showRestarting()
{
  Screen.drawBitmap(0, 0, letterR, sizeof(letterR), COLS);
}

void fail(const char *reason)
{
  if (updateError.isEmpty())
  {
    updateError = reason;
  }
#ifdef ESP32
  Update.abort();
#endif
}

void handleChunk(AsyncWebServerRequest *request,
                 const String &filename,
                 size_t index,
                 uint8_t *data,
                 size_t length,
                 bool final)
{
  if (index == 0)
  {
    updateStarted = false;
    updateSucceeded = false;
    updateError = "";
    unauthorized = !isAuthorized(request);
    if (unauthorized)
    {
      return;
    }

    Serial.println("OTA update started");
    showUpdating();
    updateStarted = true;
#ifdef ESP32
    const bool begun = Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
#else
    Update.runAsync(true);
    const bool begun = Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
#endif
    if (!begun)
    {
      fail("Could not start the update");
      return;
    }
    if (request->hasHeader("X-MD5"))
    {
      Update.setMD5(request->getHeader("X-MD5")->value().c_str());
    }
  }

  if (unauthorized || !updateError.isEmpty())
  {
    return;
  }

  if (Update.write(data, length) != length)
  {
    fail("Writing the firmware failed");
    return;
  }

  if (millis() - progressMillis > 1000)
  {
    progressMillis = millis();
    Serial.printf("OTA progress: %u bytes\n", index + length);
  }

  if (final)
  {
    if (Update.end(true))
    {
      updateSucceeded = true;
    }
    else
    {
      fail("Firmware rejected (size or checksum)");
    }
  }
}

void handleUploadDone(AsyncWebServerRequest *request)
{
  if (unauthorized)
  {
    unauthorized = false;
    request->requestAuthentication();
    return;
  }

  if (updateSucceeded)
  {
    Serial.println("OTA update finished, restarting");
    showRestarting();
    request->send(200, "text/plain", "OK");
    // let the response go out before restarting
    restartAt = millis() + 1000;
    return;
  }

  Serial.printf("OTA update failed: %s\n", updateError.c_str());
  request->send(500, "text/plain", updateError.isEmpty() ? "No firmware received" : updateError);
  if (updateStarted)
  {
    currentStatus = NONE;
    Screen.loadFromStorage();
  }
}
} // namespace

void initOTA(AsyncWebServer &server)
{
  server.on("/api/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isAuthorized(request))
    {
      request->requestAuthentication();
      return;
    }
    request->send(200, "application/json", "{\"ready\":true}");
  });
  server.on("/api/firmware", HTTP_POST, handleUploadDone, handleChunk);
}

void otaLoop()
{
  if (restartAt && (long)(millis() - restartAt) >= 0)
  {
    ESP.restart();
  }
}

#endif
