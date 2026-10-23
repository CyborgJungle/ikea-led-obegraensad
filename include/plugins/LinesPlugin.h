#pragma once

#include "PluginManager.h"
#include "mode_clock.h"

class LinesPlugin : public Plugin
{
private:
  uint8_t count = 0;
  ModeTimer timer;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
