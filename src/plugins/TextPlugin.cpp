#include "plugins/TextPlugin.h"
#include "scrolling_text.h"

#ifdef ESP32
#define TEXT_GUARD std::lock_guard<std::mutex> guard(settingsMutex)
#else
#define TEXT_GUARD
#endif

TextPlugin scrollingText;

void TextPlugin::setup()
{
  TEXT_GUARD;
  resetFrame = true;
}

bool TextPlugin::configure(const std::string &value, int size, int delay)
{
  if (!scrolling_text::valid(value, size, delay))
    return false;
  TEXT_GUARD;
  text = value;
  scale = size;
  frameDelay = delay;
  playing = true;
  resetFrame = true;
  return true;
}

void TextPlugin::stop()
{
  TEXT_GUARD;
  playing = false;
  resetFrame = true;
}

void TextPlugin::writeToJson(JsonObject object)
{
  TEXT_GUARD;
  object["text"] = text;
  object["size"] = scale;
  object["delay"] = frameDelay;
  object["playing"] = playing && pluginManager.getActivePlugin() == this;
}

void TextPlugin::loop()
{
  TEXT_GUARD;
  const unsigned long now = millis();
  if (resetFrame)
  {
    offset = -COLS;
    resetFrame = false;
  }
  else if (now - lastFrame < static_cast<unsigned long>(frameDelay))
  {
    return;
  }
  lastFrame = now;
  Screen.clear();
  if (!playing)
    return;

  const font &textFont = fonts[0];
  const int y = (ROWS - 7 * scale) / 2;
  for (size_t i = 0; i < text.size(); ++i)
  {
    const int x = static_cast<int>(i) * 6 * scale - offset;
    if (x >= COLS || x + 5 * scale <= 0)
      continue;
    const uint8_t index = static_cast<uint8_t>(text[i]) - textFont.offset;
    scrolling_text::drawGlyph(fontGlyph(textFont, index), x, y, scale, COLS, ROWS,
                              [](const uint8_t *address) { return pgm_read_byte(address); },
                              [](int px, int py) { Screen.setPixel(px, py, 1); });
  }
  offset = scrolling_text::nextOffset(offset, text.size(), scale, COLS);
}
