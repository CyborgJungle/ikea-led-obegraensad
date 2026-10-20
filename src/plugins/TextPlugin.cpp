#include "plugins/TextPlugin.h"
#include "config.h"

void TextPlugin::loadText()
{
  loadedVersion = config.getTextVersion();

  switch (config.getScrollSize())
  {
  case Config::SIZE_SMALL:
    font = &fontSmall;
    break;
  case Config::SIZE_MEDIUM:
    font = &fontMedium;
    break;
  default:
    font = &fontLarge;
  }

  // one line: line breaks scroll by as spaces
  glyphs.clear();
  for (const GlyphLine &line : TextRender::decode(*font, config.getScrollText()))
  {
    if (!glyphs.empty())
    {
      glyphs.push_back(nullptr);
    }
    glyphs.insert(glyphs.end(), line.begin(), line.end());
  }
  textWidth = TextRender::width(*font, glyphs);

  // start just outside the right edge
  offset = -COLS;
}

void TextPlugin::setup()
{
  Screen.clear();
  loadText();
  lastStep = 0;
}

void TextPlugin::loop()
{
  if (loadedVersion != config.getTextVersion())
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
  TextRender::draw(*font, glyphs, -offset, (ROWS - font->lineHeight) / 2);

  if (++offset >= textWidth)
  {
    offset = -COLS;
  }
}

const char *TextPlugin::getName() const
{
  return "Text";
}
