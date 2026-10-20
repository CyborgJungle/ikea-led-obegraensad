#pragma once

#include <Arduino.h>
#include <vector>

#include "constants.h"
#include "pixel_fonts.h"

// Text as glyphs of one font, decoded once so drawing every frame stays cheap.
// nullptr stands for a space.
using GlyphLine = std::vector<const PixelGlyph *>;

namespace TextRender
{
// Decodes UTF-8 into glyphs; '\n' starts a new line, unknown characters become '?'
std::vector<GlyphLine> decode(const PixelFont &font, const String &text);

int width(const PixelFont &font, const GlyphLine &line);

// Word-wraps every line to maxWidth, words wider than that are split
std::vector<GlyphLine> wrap(const PixelFont &font,
                            const std::vector<GlyphLine> &lines,
                            int maxWidth);

// Draws a line with the top of its line box at y, only rows in [clipTop, clipBottom)
void draw(const PixelFont &font,
          const GlyphLine &line,
          int x,
          int y,
          int clipTop = 0,
          int clipBottom = ROWS);
} // namespace TextRender
