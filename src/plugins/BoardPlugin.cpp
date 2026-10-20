#include "plugins/BoardPlugin.h"
#include "config.h"

namespace
{
struct Fit
{
  const PixelFont *font;
  int pitch;    // rows from one line to the next
  int maxLines; // lines that fit on the screen at that pitch
};

// largest first; two small lines share the screen with their descenders cut to 1 px
const Fit FITS[] = {
    {&fontLarge, 16, 1},
    {&fontMedium, 13, 1},
    {&fontSmall, 8, 2},
};

constexpr unsigned long READER_PAUSE = 2000;
// redraw now and then, a scrolling message clears the screen meanwhile
constexpr unsigned long REDRAW_INTERVAL = 250;

std::vector<GlyphLine> layout(const PixelFont &font, const String &text)
{
  std::vector<GlyphLine> lines = TextRender::wrap(font, TextRender::decode(font, text), COLS);
  // empty lines at the end only add blank space to the reader
  while (lines.size() > 1 && lines.back().empty())
  {
    lines.pop_back();
  }
  return lines;
}
} // namespace

void BoardPlugin::loadText()
{
  loadedVersion = config.getTextVersion();
  const String text = config.getBoardText();

  for (const Fit &fit : FITS)
  {
    std::vector<GlyphLine> candidate = layout(*fit.font, text);
    if ((int)candidate.size() <= fit.maxLines)
    {
      font = fit.font;
      lines = candidate;
      linePitch = fit.pitch;
      clipLines = candidate.size() > 1;
      phase = STATIC;
      draw();
      return;
    }
  }

  // too long for any size: read it line by line in the small font
  font = &fontSmall;
  lines = layout(fontSmall, text);
  linePitch = fontSmall.lineHeight;
  clipLines = false;
  offset = 0;
  maxOffset = lines.size() * linePitch - ROWS;
  phase = PAUSE_TOP;
  phaseStart = millis();
  draw();
}

void BoardPlugin::draw()
{
  Screen.clear();

  int y = -offset;
  if (phase == STATIC)
  {
    // centre the block; a single line keeps its full height
    const int height =
        lines.size() == 1 ? font->lineHeight : (int)lines.size() * linePitch;
    y = (ROWS - height) / 2;
  }

  for (const GlyphLine &line : lines)
  {
    if (y + linePitch > 0 && y < ROWS)
    {
      const int x = (COLS - TextRender::width(*font, line)) / 2;
      if (clipLines)
      {
        TextRender::draw(*font, line, x, y, y, y + linePitch);
      }
      else
      {
        TextRender::draw(*font, line, x, y);
      }
    }
    y += linePitch;
  }

  lastDraw = millis();
}

void BoardPlugin::setup()
{
  offset = 0;
  loadText();
}

void BoardPlugin::loop()
{
  if (loadedVersion != config.getTextVersion())
  {
    loadText();
    return;
  }

  const unsigned long now = millis();
  switch (phase)
  {
  case PAUSE_TOP:
    if (now - phaseStart >= READER_PAUSE)
    {
      phase = SCROLLING;
    }
    break;

  case SCROLLING:
    if (now - lastDraw >= config.getBoardSpeed())
    {
      offset++;
      draw();
      if (offset >= maxOffset)
      {
        phase = PAUSE_BOTTOM;
        phaseStart = now;
      }
    }
    return;

  case PAUSE_BOTTOM:
    if (now - phaseStart >= READER_PAUSE)
    {
      offset = 0;
      phase = PAUSE_TOP;
      phaseStart = now;
      draw();
    }
    break;

  case STATIC:
    break;
  }

  if (now - lastDraw >= REDRAW_INTERVAL)
  {
    draw();
  }
}

const char *BoardPlugin::getName() const
{
  return "Board";
}
