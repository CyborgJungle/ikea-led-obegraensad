#include "config.h"
#include <ArduinoJson.h>
#include "brightness_schedule.h"

Config config;


Config::Config()
{
  initialized = false;
  textVersion = 0;
  countdownRun = 0;
  countdownReset = 0;
#ifdef ENABLE_STORAGE
  storageAvailable = false;
#endif
  setDefaults();
}


void Config::begin()
{
  Serial.println("[Config] Initializing configuration system...");
  
  // Set safe defaults first
  setDefaults();
  
#ifdef ENABLE_STORAGE
  // Try to initialize storage
  try {
    if (preferences.begin("config", false)) {
      storageAvailable = true;
      Serial.println("[Config] Storage initialized successfully");
      
      // Try to load saved configuration
      load();
    } else {
      Serial.println("[Config] Warning: Could not initialize storage, using defaults");
      storageAvailable = false;
    }
  } catch (...) {
    Serial.println("[Config] Error: Exception during storage init, using defaults");
    storageAvailable = false;
  }
#else
  Serial.println("[Config] Storage disabled, using defaults");
#endif
  
  initialized = true;
  
  Serial.println("[Config] ============================================");
  Serial.println("[Config] Current Configuration:");
  Serial.print("[Config] Weather Location: ");
  Serial.println(weatherLocation);
  Serial.print("[Config] NTP Server: ");
  Serial.println(ntpServer);
  Serial.print("[Config] Timezone: ");
  Serial.println(tzInfo);
  Serial.print("[Config] Auto-Start Schedule: ");
  Serial.println(autoStartSchedule ? "enabled" : "disabled");
  Serial.print("[Config] Scroll Text: ");
  Serial.println(scrollText);
  Serial.println("[Config] ============================================");
}

void Config::setDefaults()
{
  // Use constants from constants.h as defaults
  weatherLocation = String(WEATHER_LOCATION);
  ntpServer = String(NTP_SERVER);
  tzInfo = String(TZ_INFO);
  autoStartSchedule = false;
  scrollText = "Hello!";
  scrollSpeed = 60;
  scrollSize = SIZE_LARGE;
  boardText = "Menu\nCoffee 3\nTea 2,50";
  boardSpeed = 120;
  lavaSpeed = 100;
  countdownSeconds = 60;
  scheduleHoursEnabled = false;
  scheduleFrom = "08:00";
  scheduleTo = "23:00";
  textVersion++;
}

void Config::load()
{
#ifdef ENABLE_STORAGE
  if (!storageAvailable) {
    Serial.println("[Config] Storage not available, keeping defaults");
    return;
  }
  
  try {
    // Load with fallback to defaults
    weatherLocation = preferences.getString("weatherLoc", String(WEATHER_LOCATION));
    ntpServer = preferences.getString("ntpServer", String(NTP_SERVER));
    tzInfo = preferences.getString("tzInfo", String(TZ_INFO));
    autoStartSchedule = preferences.getBool("autoSchedule", false);
    setScrollText(preferences.getString("scrollText", scrollText));
    setScrollSpeed(preferences.getUShort("scrollSpeed", scrollSpeed));
    setScrollSize(preferences.getUChar("scrollSize", scrollSize));
    setBoardText(preferences.getString("boardText", boardText));
    setBoardSpeed(preferences.getUShort("boardSpeed", boardSpeed));
    setLavaSpeed(preferences.getUShort("lavaSpeed", lavaSpeed));
    countdownSeconds = constrain(preferences.getUShort("countdown", countdownSeconds), 1, COUNTDOWN_MAX_SECONDS);
    setScheduleHours(preferences.getBool("schedHours", scheduleHoursEnabled),
                     preferences.getString("schedFrom", scheduleFrom),
                     preferences.getString("schedTo", scheduleTo));
    
    Serial.println("[Config] Configuration loaded from storage");
  } catch (...) {
    Serial.println("[Config] Error loading config, using defaults");
    setDefaults();
  }
#endif
}

void Config::save()
{
#ifdef ENABLE_STORAGE
  if (!storageAvailable) {
    Serial.println("[Config] Storage not available, cannot save");
    return;
  }
  
  try {
    preferences.putString("weatherLoc", weatherLocation);
    preferences.putString("ntpServer", ntpServer);
    preferences.putString("tzInfo", tzInfo);
    preferences.putBool("autoSchedule", autoStartSchedule);
    preferences.putString("scrollText", scrollText);
    preferences.putUShort("scrollSpeed", scrollSpeed);
    preferences.putUChar("scrollSize", scrollSize);
    preferences.putString("boardText", boardText);
    preferences.putUShort("boardSpeed", boardSpeed);
    preferences.putUShort("lavaSpeed", lavaSpeed);
    preferences.putUShort("countdown", countdownSeconds);
    preferences.putBool("schedHours", scheduleHoursEnabled);
    preferences.putString("schedFrom", scheduleFrom);
    preferences.putString("schedTo", scheduleTo);
    
    Serial.println("[Config] Configuration saved");
  } catch (...) {
    Serial.println("[Config] Error: Could not save configuration");
  }
#else
  Serial.println("[Config] Storage disabled, cannot save");
#endif
}


String Config::getWeatherLocation() const
{
  return weatherLocation.length() > 0 ? weatherLocation : String(WEATHER_LOCATION);
}

String Config::getNtpServer() const
{
  return ntpServer.length() > 0 ? ntpServer : String(NTP_SERVER);
}

String Config::getTzInfo() const
{
  return tzInfo.length() > 0 ? tzInfo : String(TZ_INFO);
}

bool Config::getAutoStartSchedule() const
{
  return autoStartSchedule;
}

void Config::setWeatherLocation(const String& location)
{
  if (location.length() > 0 && location.length() < 100) {
    weatherLocation = location;
  }
}


void Config::setNtpServer(const String& server)
{
  if (server.length() > 0 && server.length() < 100) {
    ntpServer = server;
  }
}

void Config::setTzInfo(const String& tz)
{
  if (tz.length() > 0 && tz.length() < 100) {
    tzInfo = tz;
  }
}

void Config::setAutoStartSchedule(bool autoStart)
{
  autoStartSchedule = autoStart;
}

String Config::getScrollText() const
{
  return scrollText;
}

uint16_t Config::getScrollSpeed() const
{
  return scrollSpeed;
}

// an empty text is allowed, it blanks the screen
void Config::setScrollText(const String& text)
{
  if (text.length() <= SCROLL_TEXT_MAX_LENGTH && text != scrollText) {
    scrollText = text;
    textVersion++;
  }
}

void Config::setScrollSpeed(int speed)
{
  scrollSpeed = constrain(speed, SCROLL_SPEED_MIN, SCROLL_SPEED_MAX);
}

uint8_t Config::getScrollSize() const
{
  return scrollSize;
}

void Config::setScrollSize(int size)
{
  const uint8_t valid = constrain(size, SIZE_SMALL, SIZE_LARGE);
  if (valid != scrollSize) {
    scrollSize = valid;
    textVersion++;
  }
}

String Config::getBoardText() const
{
  return boardText;
}

void Config::setBoardText(const String& text)
{
  if (text.length() <= BOARD_TEXT_MAX_LENGTH && text != boardText) {
    boardText = text;
    textVersion++;
  }
}

uint16_t Config::getBoardSpeed() const
{
  return boardSpeed;
}

void Config::setBoardSpeed(int speed)
{
  boardSpeed = constrain(speed, SCROLL_SPEED_MIN, SCROLL_SPEED_MAX);
}

void Config::setLavaSpeed(int percent)
{
  lavaSpeed = constrain(percent, LAVA_SPEED_MIN, LAVA_SPEED_MAX);
}

void Config::setCountdownSeconds(int seconds)
{
  countdownSeconds = constrain(seconds, 1, COUNTDOWN_MAX_SECONDS);
  countdownReset++;
}

void Config::startCountdown()
{
  countdownRun++;
}

void Config::resetCountdown()
{
  countdownReset++;
}

// ignored when a time is malformed or both are the same (an empty window)
void Config::setScheduleHours(bool enabled, const String& from, const String& to)
{
  uint16_t fromMinute, toMinute;
  if (!BrightnessSchedule::parseTime(from, fromMinute) ||
      !BrightnessSchedule::parseTime(to, toMinute) || fromMinute == toMinute) {
    return;
  }
  scheduleHoursEnabled = enabled;
  scheduleFrom = from;
  scheduleTo = to;
}


String Config::toJson() const
{
  JsonDocument doc;
  doc["weatherLocation"] = weatherLocation;
  doc["ntpServer"] = ntpServer;
  doc["tzInfo"] = tzInfo;
  doc["autoStartSchedule"] = autoStartSchedule;
  doc["scrollText"] = scrollText;
  doc["scrollSpeed"] = scrollSpeed;
  doc["scrollSize"] = scrollSize;
  doc["boardText"] = boardText;
  doc["boardSpeed"] = boardSpeed;
  doc["lavaSpeed"] = lavaSpeed;
  doc["countdownSeconds"] = countdownSeconds;
  doc["scheduleHoursEnabled"] = scheduleHoursEnabled;
  doc["scheduleFrom"] = scheduleFrom;
  doc["scheduleTo"] = scheduleTo;
  
  String output;
  serializeJson(doc, output);
  return output;
}


bool Config::fromJson(const String& json)
{
  if (json.length() == 0) {
    Serial.println("[Config] Empty JSON");
    return false;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.print("[Config] JSON parse error: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Validate and set each field
  if (doc["weatherLocation"].is<String>()) {
    String loc = doc["weatherLocation"].as<String>();
    if (loc.length() > 0 && loc.length() < 100) {
      weatherLocation = loc;
    }
  }
  
  if (doc["ntpServer"].is<String>()) {
    String ntp = doc["ntpServer"].as<String>();
    if (ntp.length() > 0 && ntp.length() < 100) {
      ntpServer = ntp;
    }
  }
  
  if (doc["tzInfo"].is<String>()) {
    String tz = doc["tzInfo"].as<String>();
    if (tz.length() > 0 && tz.length() < 100) {
      tzInfo = tz;
    }
  }
  
  if (doc["autoStartSchedule"].is<bool>()) {
    autoStartSchedule = doc["autoStartSchedule"].as<bool>();
  }

  if (doc["scrollText"].is<String>()) {
    setScrollText(doc["scrollText"].as<String>());
  }

  if (doc["scrollSpeed"].is<int>()) {
    setScrollSpeed(doc["scrollSpeed"].as<int>());
  }

  if (doc["scrollSize"].is<int>()) {
    setScrollSize(doc["scrollSize"].as<int>());
  }

  if (doc["boardText"].is<String>()) {
    setBoardText(doc["boardText"].as<String>());
  }

  if (doc["boardSpeed"].is<int>()) {
    setBoardSpeed(doc["boardSpeed"].as<int>());
  }

  if (doc["lavaSpeed"].is<int>()) {
    setLavaSpeed(doc["lavaSpeed"].as<int>());
  }

  // duration first, so {"countdownSeconds": 30, "countdownStart": true} starts 30 s
  if (doc["countdownSeconds"].is<int>()) {
    setCountdownSeconds(doc["countdownSeconds"].as<int>());
  }

  if (doc["scheduleHoursEnabled"].is<bool>() || doc["scheduleFrom"].is<String>() ||
      doc["scheduleTo"].is<String>()) {
    setScheduleHours(doc["scheduleHoursEnabled"] | scheduleHoursEnabled,
                     doc["scheduleFrom"] | scheduleFrom,
                     doc["scheduleTo"] | scheduleTo);
  }

  if (doc["countdownStart"] | false) {
    startCountdown();
  }

  if (doc["countdownReset"] | false) {
    resetCountdown();
  }
  
  return true;
}

