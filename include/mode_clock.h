#pragma once

#include <Arduino.h>

// The active mode's own clock: like millis(), but running at the mode's speed
// setting (Config::getModeSpeed), so at 200% a second of it passes in half a
// real second. Modes that keep time with modeMillis() and ModeTimer run
// faster or slower with their speed setting, without changing their code
// otherwise. The plugin manager advances it before each loop of the mode.
uint32_t modeMillis();

// moves the clock on by the real time since the last call, at speedPercent
void advanceModeClock(uint16_t speedPercent);

// NonBlockingDelay (timing.h) on the mode's clock
class ModeTimer
{
private:
  uint32_t previous = 0;
  bool firstRun = true;

public:
  // true once interval ms of mode time have passed since the last true
  bool isReady(uint32_t interval)
  {
    const uint32_t now = modeMillis();
    // allow the first run to execute immediately
    if (firstRun)
    {
      previous = now;
      firstRun = false;
      return true;
    }
    if (now - previous >= interval)
    {
      previous = now;
      return true;
    }
    return false;
  }

  void reset()
  {
    previous = modeMillis();
    firstRun = false;
  }

  // the next isReady() returns true
  void forceReady()
  {
    firstRun = true;
  }
};
