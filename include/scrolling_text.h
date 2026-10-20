#pragma once

#include <cstdint>
#include <string>

// Kept independent of Arduino so the bounds and pixel scaling can be tested
// on a host computer. The bundled system font has five columns and seven rows.
namespace scrolling_text
{
constexpr size_t MAX_LENGTH = 120;
constexpr int MIN_DELAY = 20;
constexpr int MAX_DELAY = 250;

inline bool valid(const std::string &text, int scale, int delay)
{
  if (text.empty() || text.size() > MAX_LENGTH || (scale != 1 && scale != 2) ||
      delay < MIN_DELAY || delay > MAX_DELAY)
    return false;
  bool hasVisibleCharacter = false;
  for (unsigned char c : text)
  {
    if (c < 32 || c > 126)
      return false;
    hasVisibleCharacter = hasVisibleCharacter || c != ' ';
  }
  return hasVisibleCharacter;
}

inline int width(size_t length, int scale)
{
  return static_cast<int>(length) * 6 * scale;
}

inline int nextOffset(int offset, size_t length, int scale, int columns)
{
  return offset + 1 > width(length, scale) ? -columns : offset + 1;
}

template <typename ReadByte, typename SetPixel>
void drawGlyph(const uint8_t *glyph, int x, int y, int scale, int columns, int rows,
               ReadByte readByte, SetPixel setPixel)
{
  for (int row = 0; row < 7; ++row)
  {
    const uint8_t bits = readByte(glyph + row);
    for (int col = 0; col < 5; ++col)
    {
      if (!(bits & (0x80 >> col)))
        continue;
      for (int dy = 0; dy < scale; ++dy)
      {
        for (int dx = 0; dx < scale; ++dx)
        {
          const int px = x + col * scale + dx;
          const int py = y + row * scale + dy;
          if (px >= 0 && px < columns && py >= 0 && py < rows)
            setPixel(px, py);
        }
      }
    }
  }
}
}
