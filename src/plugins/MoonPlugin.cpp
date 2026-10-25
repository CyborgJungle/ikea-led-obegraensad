#include "plugins/MoonPlugin.h"

// the place lookup needs WiFi and a second core for its task
#ifdef ESP32

#include "config.h"
#include "constants.h"
#include "moon_math.h"
#include "screen.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <memory>
#include <time.h>

namespace
{
// Open-Meteo's free geocoding (no key): a city name to coordinates
const char *const GEOCODE_URL = "https://geocoding-api.open-meteo.com/v1/search?count=1&language=en&format=json&name=";
constexpr unsigned long RETRY_AFTER_MS = 60UL * 1000;
// the moon moves slowly: once a minute is plenty
constexpr unsigned long DRAW_EVERY_MS = 60UL * 1000;

String urlEncode(const String &text)
{
  String out;
  const char *hex = "0123456789ABCDEF";
  for (size_t i = 0; i < text.length(); i++)
  {
    const uint8_t c = text[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      out += static_cast<char>(c);
    }
    else
    {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}
} // namespace

void MoonPlugin::setup()
{
  Screen.clear();
  memset(previousBrightness, 0, sizeof(previousBrightness));
  drawn = false;
  lastDraw = millis();
  // check the city on the first loop
  lastCityCheck = millis() - 5000;
  draw();
}

void MoonPlugin::startLookup(const String &city)
{
  if (lookupRunning.exchange(true))
  {
    return;
  }
  lookupCity = city;
  lastLookup = millis();
  lookupFailed = false;
  if (xTaskCreatePinnedToCore(geocodeTask, "moonPlace", 12288, this, 1, NULL, 1) != pdPASS)
  {
    lookupFailed = true;
    lookupRunning = false;
  }
}

void MoonPlugin::geocodeTask(void *parameter)
{
  MoonPlugin *plugin = static_cast<MoonPlugin *>(parameter);
  const bool ok = plugin->lookup();
  plugin->lookupFailed = !ok;
  plugin->lookupRunning = false;
  vTaskDelete(NULL);
}

// Runs in the lookup task: the coordinates of lookupCity
bool MoonPlugin::lookup()
{
  if (WiFi.status() != WL_CONNECTED || lookupCity.isEmpty())
  {
    return false;
  }
  // a public lookup of a city name; certificates are not checked, as for the weather
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure());
  client->setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(*client, String(GEOCODE_URL) + urlEncode(lookupCity)))
  {
    return false;
  }
  const int code = http.GET();
  if (code != HTTP_CODE_OK)
  {
    Serial.printf("[Moon] Place lookup answered %d\n", code);
    http.end();
    return false;
  }
  const String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body) || !doc["results"][0]["latitude"].is<double>() ||
      !doc["results"][0]["longitude"].is<double>())
  {
    Serial.println("[Moon] No place found for the weather location");
    return false;
  }
  foundLatitude = doc["results"][0]["latitude"].as<double>();
  foundLongitude = doc["results"][0]["longitude"].as<double>();
  placeReady = true;
  return true;
}

void MoonPlugin::loop()
{
  // a place came in: draw with it now
  if (placeReady)
  {
    latitude = foundLatitude;
    longitude = foundLongitude;
    placeReady = false;
    hasPlace = true;
    placeFor = lookupCity;
    draw();
  }
  // look the city up when it is new, or again a minute after a failure;
  // checked every few seconds, a change in Settings shows soon enough
  if (!lookupRunning && millis() - lastCityCheck >= 5000)
  {
    lastCityCheck = millis();
    const String city = config.getWeatherLocation();
    if (!city.isEmpty() && city != placeFor &&
        (!lookupFailed || lookupCity != city || millis() - lastLookup >= RETRY_AFTER_MS))
    {
      startLookup(city);
    }
  }
  if (millis() - lastDraw >= DRAW_EVERY_MS)
  {
    lastDraw = millis();
    draw();
  }
}

void MoonPlugin::draw()
{
  const time_t now = time(nullptr);
  // the clock is set once it is past 2023; before that, a dim outline
  const bool clockSet = now > 1700000000;
  const MoonState s = moonState(static_cast<double>(now), hasPlace, latitude, longitude);

  uint8_t frame[ROWS * COLS];
  renderMoon(s, clockSet, frame);
  for (uint16_t index = 0; index < ROWS * COLS; index++)
  {
    const uint8_t brightness = frame[index];
    if (!drawn || previousBrightness[index] != brightness)
    {
      Screen.setPixel(index % COLS, index / COLS, brightness > 0, brightness);
      previousBrightness[index] = brightness;
    }
  }
  drawn = true;
}

const char *MoonPlugin::getName() const
{
  return "Moon";
}

#endif
