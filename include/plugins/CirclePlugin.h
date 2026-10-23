#pragma once

#include "PluginManager.h"
#include "mode_clock.h"

class CirclePlugin : public Plugin
{
private:
  uint8_t circleStep = 0;
  ModeTimer timer;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
