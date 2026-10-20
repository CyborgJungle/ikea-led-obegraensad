#pragma once

#include "PluginManager.h"

// Counts down from the duration in the config as soon as the mode starts,
// then shows rings rippling out from the centre until the mode changes
class CountdownPlugin : public Plugin
{
private:
  uint32_t loadedRun = 0;
  unsigned long startMillis = 0;
  unsigned long durationMillis = 0;
  int shownSeconds = -1;
  bool finished = false;
  unsigned long finishedMillis = 0;
  unsigned long lastFrame = 0;

  void restart();
  void drawRemaining(int seconds);
  void drawFinished(unsigned long elapsed);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
