#pragma once

#include "PluginManager.h"
#include "text_render.h"

// Shows the board text from the config standing still, like a menu board.
// Picks the largest font the text fits in; if it does not fit even in the
// smallest, it becomes a reader that scrolls the lines up.
class BoardPlugin : public Plugin
{
private:
  enum Phase : uint8_t
  {
    STATIC,
    PAUSE_TOP,
    SCROLLING,
    PAUSE_BOTTOM
  };

  const PixelFont *font = &fontLarge;
  std::vector<GlyphLine> lines;
  int linePitch = 0;
  bool clipLines = false;
  uint32_t loadedVersion = 0;

  Phase phase = STATIC;
  int offset = 0;
  int maxOffset = 0;
  unsigned long phaseStart = 0;
  unsigned long lastDraw = 0;

  void loadText();
  void draw();

public:
  void setup() override;
  void loop() override;
  const char *getName() const override;
};
