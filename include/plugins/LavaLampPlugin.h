#pragma once

#include "PluginManager.h"

// A lava lamp: wax resting on the bottom heats up, swells and rises slowly,
// cools near the top and sinks back down. Drawn like Blobs, as one field of
// soft blobs that melt together at their edges, but moved by temperature: a
// round trip takes about a minute, faster or slower with the speed setting.
class LavaLampPlugin : public Plugin
{
public:
  void setup() override;
  void loop() override;
  const char *getName() const override;

private:
  struct Wax
  {
    float home;   // column the wax drifts around
    float phase;  // of the sideways drift, radians
    float wobble; // drift speed, radians per second
    float size;   // radius when cold
    float heat;   // how fast this wax warms up on the heater
    float x, y;   // y counts down from the top, in ASPECT-scaled units
    float vy;
    float temperature; // 0 cold .. 1.2 hot
  };

  static constexpr uint8_t NUM_WAX = 4;

  Wax wax[NUM_WAX];
  unsigned long lastFrame = 0;
  uint8_t previousBrightness[ROWS * COLS];

  void step(Wax &w, float dt);
  void render();
};
