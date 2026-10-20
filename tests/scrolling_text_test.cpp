#include "scrolling_text.h"
#include <array>
#include <cassert>
#include <iostream>

int main()
{
  using namespace scrolling_text;
  assert(valid("Hello, world!", 1, 60));
  assert(valid(std::string(120, 'A'), 2, 250));
  assert(!valid("", 1, 60));
  assert(!valid("   ", 1, 60));
  assert(!valid(std::string(121, 'A'), 1, 60));
  assert(!valid("Hello\n", 1, 60));
  assert(!valid("caf\xc3\xa9", 1, 60));
  assert(!valid(std::string("A\0B", 3), 1, 60));
  assert(!valid("A", 3, 60));
  assert(!valid("A", 1, 0));
  assert(!valid("A", 1, 251));
  assert(width(120, 2) == 1440);
  assert(nextOffset(12, 1, 2, 16) == -16);
  assert(nextOffset(-16, 1, 1, 16) == -15);

  // A from the bundled 5x7 system font. Each lit source pixel must become an
  // exact 2x2 block at large size, without writing outside the 16x16 display.
  const uint8_t glyph[] = {0x70, 0x88, 0x88, 0x88, 0xf8, 0x88, 0x88};
  std::array<std::array<bool, 16>, 16> small{};
  std::array<std::array<bool, 16>, 16> large{};
  auto readByte = [](const uint8_t *p) { return *p; };
  drawGlyph(glyph, 0, 0, 1, 16, 16, readByte, [&](int x, int y) { small[y][x] = true; });
  drawGlyph(glyph, 0, 1, 2, 16, 16, readByte, [&](int x, int y) { large[y][x] = true; });
  assert(small[0][1] && !small[0][0] && small[4][4]);
  int smallCount = 0, largeCount = 0;
  for (int y = 0; y < 16; ++y)
    for (int x = 0; x < 16; ++x)
    {
      smallCount += small[y][x];
      largeCount += large[y][x];
      if (y >= 1 && y < 15 && x < 10)
        assert(large[y][x] == small[(y - 1) / 2][x / 2]);
      else
        assert(!large[y][x]);
    }
  assert(largeCount == 4 * smallCount);
  int clippedCount = 0;
  for (int origin : {-10, -1, 15, 16})
    drawGlyph(glyph, origin, 1, 2, 16, 16, readByte, [&](int x, int y) {
      assert(x >= 0 && x < 16 && y >= 0 && y < 16);
      ++clippedCount;
    });
  assert(clippedCount > 0);
  std::cout << "Text validation, scaled glyph pixels, clipping and scrolling bounds passed.\n";
}
