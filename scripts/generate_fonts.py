#!/usr/bin/env python3
"""Generates src/pixel_fonts.cpp, the proportional fonts used by the Text and Board plugins.

Source fonts (download them into one folder and pass it as the only argument):

  spleen-8x16.bdf  https://github.com/fcambus/spleen (BSD 2-Clause)
  spleen-5x8.bdf   https://github.com/fcambus/spleen (BSD 2-Clause)
  6x13.bdf         https://gitlab.freedesktop.org/xorg/font/misc-misc (public domain)
  5x8.bdf          https://gitlab.freedesktop.org/xorg/font/misc-misc (public domain)

  python3 scripts/generate_fonts.py path/to/fonts

Spleen 5x8 only covers ASCII: its accented letters are composed here from the
base letter and an accent mark, other Latin-1 symbols and € come from 5x8.

Every glyph is trimmed to its inked columns, so the fonts become proportional,
and stored as one byte per row (most significant bit = leftmost pixel).
"""

import os
import sys
import unicodedata

# printable ASCII, Latin-1 and the euro sign
CODEPOINTS = list(range(32, 127)) + list(range(160, 256)) + [0x20AC]
# phones type typographic punctuation, draw it with the plain ASCII glyph
FALLBACKS = {0x2018: "'", 0x2019: "'", 0x201C: '"', 0x201D: '"', 0x2013: "-", 0x2014: "-"}

OUTPUT = os.path.join(os.path.dirname(__file__), "..", "src", "pixel_fonts.cpp")

SPLEEN_LICENSE = """Copyright (c) 2018-2026, Frederic Cambus
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE."""


class Glyph:
    def __init__(self, rows, top):
        self.rows = rows  # list of '#'/'.' strings, trimmed to inked columns
        self.top = top  # rows above the baseline of the first row


def trim_columns(rows):
    used = [c for c in range(len(rows[0])) if any(r[c] == "#" for r in rows)] if rows else []
    if not used:
        return []
    return [r[used[0] : used[-1] + 1] for r in rows]


def trim_rows(rows, top):
    while rows and "#" not in rows[0]:
        rows = rows[1:]
        top -= 1
    while rows and "#" not in rows[-1]:
        rows = rows[:-1]
    return rows, top


def load_bdf(path):
    glyphs, ascent, descent = {}, 0, 0
    with open(path, encoding="latin-1") as f:
        lines = f.read().splitlines()
    i = 0
    while i < len(lines):
        if lines[i].startswith("FONT_ASCENT"):
            ascent = int(lines[i].split()[1])
        elif lines[i].startswith("FONT_DESCENT"):
            descent = int(lines[i].split()[1])
        elif lines[i].startswith("STARTCHAR"):
            encoding, bbx = None, None
            while not lines[i].startswith("BITMAP"):
                parts = lines[i].split()
                if parts[0] == "ENCODING":
                    encoding = int(parts[1])
                elif parts[0] == "BBX":
                    bbx = [int(p) for p in parts[1:5]]
                i += 1
            i += 1
            width, height, _, yoffset = bbx
            rows = []
            while not lines[i].startswith("ENDCHAR"):
                bits, nbits = int(lines[i], 16), len(lines[i]) * 4
                rows.append("".join("#" if bits >> (nbits - 1 - c) & 1 else "." for c in range(width)))
                i += 1
            rows, top = trim_rows(trim_columns(rows), yoffset + height)
            glyphs[encoding] = Glyph(rows, top)
        i += 1
    return glyphs, ascent, descent


# accent marks: two rows above a lowercase letter, one row above a capital
ACCENTS = {
    0x0300: (["#.", ".#"], ["#."]),  # grave
    0x0301: ([".#", "#."], [".#"]),  # acute
    0x0302: ([".#.", "#.#"], [".#."]),  # circumflex
    0x0303: ([".#.#", "#.#."], ["#.#."]),  # tilde
    0x0308: (["....", "#..#"], ["#..#"]),  # diaeresis
    0x030A: ([".#.", "#.#"], [".#."]),  # ring
}
CEDILLA = 0x0327


def compose(base, mark, ascent, lowercase):
    """Puts an accent on a letter of a font whose capitals start one row below the
    top of the cell and whose lowercase starts two rows below it."""
    width = len(base.rows[0])
    cell = ["." * width] * (ascent + 1)
    for i, row in enumerate(base.rows):
        cell[ascent - base.top + i] = row
    if mark == CEDILLA:
        cell[ascent] = ".#".ljust(width, ".")
    else:
        pattern = ACCENTS[mark][0 if lowercase else 1]
        if lowercase:
            cell[0] = cell[1] = "." * width  # drops the dot of i
        for i, row in enumerate(pattern):
            if row.startswith("#") and row.endswith("#") and len(row) > 3:
                row = "#" + "." * (width - 2) + "#"  # diaeresis spans the letter
            left = (width - len(row)) // 2
            cell[i] = ("." * left + row).ljust(width, ".")[:width]
    rows, top = trim_rows(cell, ascent)
    return Glyph(rows, top)


def load_small(source):
    glyphs, ascent, _ = load_bdf(os.path.join(source, "spleen-5x8.bdf"))
    symbols = load_bdf(os.path.join(source, "5x8.bdf"))[0]
    for cp in CODEPOINTS:
        if cp in glyphs and glyphs[cp].rows or cp == 32:
            continue
        parts = unicodedata.decomposition(chr(cp)).split()
        if len(parts) == 2 and not parts[0].startswith("<"):
            base, mark = int(parts[0], 16), int(parts[1], 16)
            if base in glyphs and (mark in ACCENTS or mark == CEDILLA):
                glyphs[cp] = compose(glyphs[base], mark, ascent, chr(base).islower())
                continue
        if cp in symbols:
            glyphs[cp] = symbols[cp]
    return glyphs


def emit_font(out, name, glyphs, ascent, descent, space_width):
    for cp, char in FALLBACKS.items():
        glyphs.setdefault(cp, glyphs[ord(char)])
    data, entries = [], []
    for cp in sorted(c for c in glyphs if c in CODEPOINTS or c in FALLBACKS):
        glyph = glyphs[cp]
        width = len(glyph.rows[0]) if glyph.rows else 0
        assert width <= 8, f"{name} U+{cp:04X} is {width} px wide"
        entries.append(
            f"    {{0x{cp:04X}, {width}, {ascent - glyph.top if glyph.rows else 0}, "
            f"{len(glyph.rows)}, {len(data)}}},"
        )
        for row in glyph.rows:
            data.append(int(row.replace("#", "1").replace(".", "0").ljust(8, "0"), 2))

    out.append(f"static const uint8_t {name}Data[] = {{")
    for i in range(0, len(data), 16):
        out.append("    " + ", ".join(f"0x{b:02X}" for b in data[i : i + 16]) + ",")
    out.append("};\n")
    out.append(f"static const PixelGlyph {name}Glyphs[] = {{")
    out.extend(entries)
    out.append("};\n")
    out.append(
        f"const PixelFont {name} = {{{ascent + descent}, {ascent}, {space_width}, "
        f"{len(entries)}, {name}Glyphs, {name}Data}};\n"
    )


def main():
    source = sys.argv[1]
    large, large_ascent, large_descent = load_bdf(os.path.join(source, "spleen-8x16.bdf"))
    medium, medium_ascent, medium_descent = load_bdf(os.path.join(source, "6x13.bdf"))
    small = load_small(source)

    out = [
        "// Generated by scripts/generate_fonts.py, do not edit.",
        "//",
        "// Large and Small: Spleen 8x16 and 5x8 by Frederic Cambus, BSD 2-Clause license:",
        *("//   " + line for line in SPLEEN_LICENSE.splitlines()),
        "// Medium: X11 misc-fixed 6x13, public domain.",
        "// Small accented letters are composed from Spleen 5x8, symbols and € from X11 5x8 (public domain).",
        "",
        '#include "pixel_fonts.h"',
        "",
    ]
    emit_font(out, "fontLarge", large, large_ascent, large_descent, 4)
    emit_font(out, "fontMedium", medium, medium_ascent, medium_descent, 3)
    emit_font(out, "fontSmall", small, 7, 1, 2)

    with open(OUTPUT, "w") as f:
        f.write("\n".join(out))
    print(f"wrote {os.path.normpath(OUTPUT)}")


if __name__ == "__main__":
    main()
