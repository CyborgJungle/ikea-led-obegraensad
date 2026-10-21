#pragma once

#include "PluginManager.h"

// Shows the frames the camera page (served over HTTPS, see secure_server.h)
// sends from a phone: 16x16 grayscale, one byte per pixel, row by row
class CameraPlugin : public Plugin
{
private:
  uint32_t shownFrame = 0;
  bool idleShown = false;

  void drawIdle();

public:
  // called from the HTTPS server task
  static void receiveFrame(const uint8_t *pixels);

  void setup() override;
  void loop() override;
  const char *getName() const override;
};
