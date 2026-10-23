#pragma once

#include "PluginManager.h"
#include "mode_clock.h"

class WaveBarsPlugin : public Plugin
{
private:
  ModeTimer timer;
  float phase = 0.0f;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
