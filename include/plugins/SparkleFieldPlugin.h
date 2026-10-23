#pragma once

#include "PluginManager.h"
#include "mode_clock.h"

class SparkleFieldPlugin : public Plugin
{
private:
  ModeTimer timer;
  uint8_t sparkles[16][16] = {};

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
