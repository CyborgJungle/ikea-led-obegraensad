#pragma once

#include "constants.h"
#include <stdint.h>

// the moon at a moment, for an observer at lat/lon (degrees); without a place,
// north is drawn up and the lit edge as seen from the Earth's centre
struct MoonState
{
  double fraction;    // 0 new .. 1 full
  double phaseAngle;  // sun-moon-observer angle, degrees
  bool waxing;
  double limbFromUp;  // where the lit edge points, degrees from up towards the left
  double northFromUp; // where the moon's north points, likewise
  double altitude;    // above the horizon, degrees (0 without a place)
};

MoonState moonState(double unixTime, bool hasPlace, double latitude, double longitude);

// the moon on the 16x16 LEDs, one brightness per LED, row by row; before the
// clock is set (clockSet false) a dim outline
void renderMoon(const MoonState &state, bool clockSet, uint8_t *out);
