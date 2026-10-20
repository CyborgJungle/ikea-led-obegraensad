#pragma once

#include "PluginManager.h"

// Scrolls the text from the config (set in the web app) over the screen, endlessly
class TextPlugin : public Plugin
{
private:
  std::vector<uint8_t> glyphs; // glyph index per character, decoded once per text change
  uint32_t loadedVersion = 0;
  int offset = 0;
  unsigned long lastStep = 0;

  void loadText();
  int textWidth() const;

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
