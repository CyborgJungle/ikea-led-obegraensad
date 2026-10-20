#pragma once

#include "PluginManager.h"
#include "text_render.h"

// Scrolls the text from the config (set in the web app) over the screen, endlessly
class TextPlugin : public Plugin
{
private:
  const PixelFont *font = &fontLarge;
  GlyphLine glyphs;
  int textWidth = 0;
  uint32_t loadedVersion = 0;
  int offset = 0;
  unsigned long lastStep = 0;

  void loadText();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
