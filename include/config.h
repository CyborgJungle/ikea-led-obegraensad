#pragma once

#include <Arduino.h>
#include <string>
#include "constants.h"

#ifdef ENABLE_STORAGE
#include <Preferences.h>
#endif

class Config
{
private:
#ifdef ENABLE_STORAGE
  Preferences preferences;
  bool storageAvailable;
#endif
  
  // Current values with safe defaults
  String weatherLocation;
  String ntpServer;
  String tzInfo;
  bool autoStartSchedule;
  String scrollText;
  uint16_t scrollSpeed;
  // bumped on every text/speed change, the Text plugin reloads when it differs
  volatile uint32_t scrollTextVersion;
  bool initialized;

public:
  Config();
  
  // Initialize configuration - always succeeds with defaults if storage fails
  void begin();
  
  // Load from storage (safe - uses defaults on failure)
  void load();
  
  // Save to storage (safe - fails silently)
  void save();
  
  // Reset to hardcoded defaults
  void setDefaults();
  
  // Getters (always return valid values)
  String getWeatherLocation() const;
  String getNtpServer() const;
  String getTzInfo() const;
  bool getAutoStartSchedule() const;
  String getScrollText() const;
  uint16_t getScrollSpeed() const;
  uint32_t getScrollTextVersion() const { return scrollTextVersion; }
  bool isInitialized() const { return initialized; }
  
  // Setters with validation
  void setWeatherLocation(const String& location);
  void setNtpServer(const String& server);
  void setTzInfo(const String& tz);
  void setAutoStartSchedule(bool autoStart);
  void setScrollText(const String& text);
  void setScrollSpeed(int speed);

  static constexpr size_t SCROLL_TEXT_MAX_LENGTH = 200;
  static constexpr uint16_t SCROLL_SPEED_MIN = 20;  // ms per pixel
  static constexpr uint16_t SCROLL_SPEED_MAX = 250;
  
  // Export to JSON
  String toJson() const;
  
  // Import from JSON (validates input)
  bool fromJson(const String& json);
};

extern Config config;
