#pragma once

#include "PluginManager.h"
#ifdef ESP32
#include <mutex>
#endif

class TextPlugin : public Plugin
{
private:
#ifdef ESP32
  std::mutex settingsMutex;
#endif
  std::string text = "Hello!";
  int scale = 1;
  int frameDelay = 60;
  bool playing = false;
  bool resetFrame = true;
  int offset = -COLS;
  unsigned long lastFrame = 0;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override { return "Text"; }
  bool configure(const std::string &value, int size, int delay);
  void stop();
  void writeToJson(JsonObject object);
};

// One instance is registered at the end of the plugin list to preserve all
// existing plugin IDs in saved settings and schedules.
extern TextPlugin scrollingText;
