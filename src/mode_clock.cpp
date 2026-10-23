#include "mode_clock.h"

namespace
{
// written by the screen task only, read by any task: 32 bits read whole
volatile uint32_t modeNow = 0;
uint32_t lastReal = 0;
// hundredths of a millisecond not yet added, so slow speeds do not lose time
uint32_t leftover = 0;
bool started = false;
// a longer gap (an update, a long message on screen) counts as this much,
// so the mode carries on instead of jumping ahead
constexpr uint32_t MAX_STEP_MS = 1000;
} // namespace

uint32_t modeMillis()
{
  return modeNow;
}

void advanceModeClock(uint16_t speedPercent)
{
  const uint32_t real = millis();
  if (!started)
  {
    lastReal = real;
    started = true;
    return;
  }
  uint32_t elapsed = real - lastReal;
  lastReal = real;
  if (elapsed > MAX_STEP_MS)
  {
    elapsed = MAX_STEP_MS;
  }
  const uint32_t scaled = elapsed * speedPercent + leftover;
  modeNow = modeNow + scaled / 100;
  leftover = scaled % 100;
}
