#pragma once

#include "PluginManager.h"

// Shows the duration from the config and waits; a start (web app) counts it
// down, at zero rings ripple out from the centre for a moment, then it shows
// the duration again and waits for the next start
class CountdownPlugin : public Plugin
{
private:
  enum Phase : uint8_t
  {
    WAITING,
    RUNNING,
    FINISHED
  };

  Phase phase = WAITING;
  uint32_t loadedRun = 0;
  uint32_t loadedReset = 0;
  unsigned long startMillis = 0;
  unsigned long durationMillis = 0;
  int shownSeconds = -1;
  unsigned long finishedMillis = 0;
  unsigned long lastFrame = 0;

  void wait();
  void start();
  void drawRemaining(int seconds);
  void drawFinished(unsigned long elapsed);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
