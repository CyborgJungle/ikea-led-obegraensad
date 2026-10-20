#include "plugins/TextPlugin.h"
#include "config.h"

static const font &textFont = fonts[0];
// 7 pixel high glyphs, vertically centred
static constexpr int TEXT_Y = 4;

void TextPlugin::loadText()
{
  loadedVersion = config.getScrollTextVersion();
  const String text = config.getScrollText();

  glyphs.clear();
  for (size_t i = 0; i < text.length(); i++)
  {
    const uint8_t c = text[i];
    // same as Screen.scrollText(): skip the UTF-8 lead byte of umlauts, the
    // font keeps those glyphs at the position of the second byte
    if (c == 195)
    {
      continue;
    }
    const bool known = c >= textFont.offset && c - textFont.offset < textFont.charCount;
    glyphs.push_back(known ? c - textFont.offset : 0);
  }

  // start just outside the right edge
  offset = -COLS;
}

int TextPlugin::textWidth() const
{
  return glyphs.size() * (textFont.sizeX + 1);
}

void TextPlugin::setup()
{
  Screen.clear();
  loadText();
  lastStep = 0;
}

void TextPlugin::loop()
{
  if (loadedVersion != config.getScrollTextVersion())
  {
    loadText();
  }

  const unsigned long now = millis();
  if (now - lastStep < config.getScrollSpeed())
  {
    return;
  }
  lastStep = now;

  Screen.clear();
  for (size_t i = 0; i < glyphs.size(); i++)
  {
    const int x = i * (textFont.sizeX + 1) - offset;
    if (x > -(textFont.sizeX + 1) && x < COLS)
    {
      Screen.drawBitmap(x, TEXT_Y, fontGlyph(textFont, glyphs[i]), textFont.sizeY, 8);
    }
  }

  if (++offset >= textWidth())
  {
    offset = -COLS;
  }
}

const char *TextPlugin::getName() const
{
  return "Text";
}
