#include "text_render.h"
#include "screen.h"

namespace
{
// pixels between two glyphs
constexpr int LETTER_SPACING = 1;

const PixelGlyph *findGlyph(const PixelFont &font, uint16_t codepoint)
{
  int low = 0;
  int high = font.glyphCount - 1;
  while (low <= high)
  {
    const int middle = (low + high) / 2;
    const uint16_t found = font.glyphs[middle].codepoint;
    if (found == codepoint)
    {
      return &font.glyphs[middle];
    }
    if (found < codepoint)
    {
      low = middle + 1;
    }
    else
    {
      high = middle - 1;
    }
  }
  return codepoint == '?' ? nullptr : findGlyph(font, '?');
}

int advance(const PixelFont &font, const PixelGlyph *glyph)
{
  return glyph ? glyph->width : font.spaceWidth;
}

// pixels the glyphs [from, to) take, without spacing after the last one
int span(const PixelFont &font, const GlyphLine &line, size_t from, size_t to)
{
  int total = 0;
  for (size_t i = from; i < to; i++)
  {
    total += advance(font, line[i]) + (i + 1 < to ? LETTER_SPACING : 0);
  }
  return total;
}
} // namespace

std::vector<GlyphLine> TextRender::decode(const PixelFont &font, const String &text)
{
  std::vector<GlyphLine> lines(1);
  const size_t length = text.length();
  for (size_t i = 0; i < length;)
  {
    const uint8_t lead = text[i];
    uint32_t codepoint = lead;
    int extra = 0;
    if (lead >= 0xF0)
    {
      codepoint = lead & 0x07;
      extra = 3;
    }
    else if (lead >= 0xE0)
    {
      codepoint = lead & 0x0F;
      extra = 2;
    }
    else if (lead >= 0xC0)
    {
      codepoint = lead & 0x1F;
      extra = 1;
    }
    i++;
    for (; extra > 0 && i < length && (text[i] & 0xC0) == 0x80; extra--, i++)
    {
      codepoint = (codepoint << 6) | (text[i] & 0x3F);
    }

    if (codepoint == '\n')
    {
      lines.emplace_back();
    }
    else if (codepoint == ' ' || codepoint == '\t')
    {
      lines.back().push_back(nullptr);
    }
    else if (codepoint >= ' ' && codepoint != 0x7F)
    {
      lines.back().push_back(findGlyph(font, codepoint > 0xFFFF ? '?' : codepoint));
    }
  }
  return lines;
}

int TextRender::width(const PixelFont &font, const GlyphLine &line)
{
  return span(font, line, 0, line.size());
}

std::vector<GlyphLine> TextRender::wrap(const PixelFont &font,
                                        const std::vector<GlyphLine> &lines,
                                        int maxWidth)
{
  std::vector<GlyphLine> wrapped;
  for (const GlyphLine &line : lines)
  {
    GlyphLine current;
    size_t i = 0;
    while (i < line.size())
    {
      if (line[i] == nullptr)
      {
        i++;
        continue;
      }
      size_t end = i;
      while (end < line.size() && line[end] != nullptr)
      {
        end++;
      }

      GlyphLine token(line.begin() + i, line.begin() + end);
      GlyphLine candidate = current;
      if (!candidate.empty())
      {
        candidate.push_back(nullptr);
      }
      candidate.insert(candidate.end(), token.begin(), token.end());

      if (width(font, candidate) <= maxWidth)
      {
        current = candidate;
      }
      else
      {
        if (!current.empty())
        {
          wrapped.push_back(current);
          current.clear();
        }
        // a word that is too wide on its own gets split over lines
        size_t start = 0;
        while (start < token.size())
        {
          size_t stop = start + 1;
          while (stop < token.size() && span(font, token, start, stop + 1) <= maxWidth)
          {
            stop++;
          }
          GlyphLine part(token.begin() + start, token.begin() + stop);
          if (stop < token.size())
          {
            wrapped.push_back(part);
          }
          else
          {
            current = part;
          }
          start = stop;
        }
      }
      i = end;
    }
    wrapped.push_back(current);
  }
  return wrapped;
}

void TextRender::draw(const PixelFont &font,
                      const GlyphLine &line,
                      int x,
                      int y,
                      int clipTop,
                      int clipBottom)
{
  clipTop = max(clipTop, 0);
  clipBottom = min(clipBottom, (int)ROWS);
  for (const PixelGlyph *glyph : line)
  {
    if (x >= COLS)
    {
      return;
    }
    if (glyph && x + glyph->width > 0)
    {
      for (int row = 0; row < glyph->height; row++)
      {
        const int screenY = y + glyph->y + row;
        if (screenY < clipTop || screenY >= clipBottom)
        {
          continue;
        }
        const uint8_t bits = font.data[glyph->offset + row];
        for (int column = 0; column < glyph->width; column++)
        {
          const int screenX = x + column;
          if (screenX >= 0 && screenX < COLS && (bits & (0x80 >> column)))
          {
            Screen.setPixel(screenX, screenY, 1);
          }
        }
      }
    }
    x += advance(font, glyph) + LETTER_SPACING;
  }
}
