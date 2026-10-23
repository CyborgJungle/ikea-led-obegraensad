#pragma once

#include "PluginManager.h"
#include "mode_clock.h"

class MeteorShowerPlugin : public Plugin
{
private:
  struct Meteor
  {
    float x;
    float y;
    float vx;
    float vy;
  };

  ModeTimer timer;
  static constexpr uint8_t kMeteorCount = 6;
  Meteor meteors[kMeteorCount];

  void resetMeteor(Meteor &meteor);

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
