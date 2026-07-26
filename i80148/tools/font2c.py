# ------------------------------------------------------------------------------
#          font2c.py - CP437 font to C header converter
#
#  Copyright (C) 2026  TheCawa <vos80584@gmail.com>
# ------------------------------------------------------------------------------
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <https://gnu.org>.
# ------------------------------------------------------------------------------

#!/usr/bin/env python3
"""Convert a CP437 8x8 hex font into a C header for the video cards.

Input format (cp4378x8.txt):
  128 lines, each line has 16 space-separated hex bytes.
  Line N (0-based) encodes glyphs 2*N and 2*N+1:
    bytes[0..7]  -> glyph 2*N
    bytes[8..15] -> glyph 2*N+1

Each glyph is 8 rows, bottom to top (bytes[0] = row 7, bytes[7] = row 0).
The resulting uint64_t stores row 0 in the least-significant byte, so the
renderer's (glyph >> (bit_y*8)) & 0xFF produces the rows in top-to-bottom order.
"""

import sys
from pathlib import Path


def parse_font(path):
    glyphs = [None] * 256
    with open(path, 'r', encoding='utf-8') as f:
        for line_no, raw in enumerate(f):
            line = raw.strip()
            if not line:
                continue
            if line_no >= 128:
                break
            nums = [int(t, 16) for t in line.split()]
            if len(nums) != 16:
                raise ValueError(f"Line {line_no + 1}: expected 16 bytes, got {len(nums)}")
            even_idx = line_no * 2
            odd_idx = even_idx + 1
            glyphs[even_idx] = nums[0:8]
            glyphs[odd_idx] = nums[8:16]
    if any(g is None for g in glyphs):
        raise ValueError("Font file did not contain enough data for 256 glyphs")
    return glyphs


def glyph_to_u64(rows):
    # The source file stores rows in bottom-to-top order:
    # rows[0] = row 7, rows[7] = row 0.  The renderer expects the least
    # significant byte to be the top row, so we reverse the order here.
    val = 0
    for i, b in enumerate(reversed(rows)):
        val |= (b & 0xFF) << (i * 8)
    return val


def emit_header(glyphs, out_path):
    lines = [
        "#ifndef FONT8X8_H",
        "#define FONT8X8_H",
        "",
        "#include <stdint.h>",
        "",
        "// CP437 8x8 bitmap font (256 glyphs).",
        "// Generated from cp4378x8.txt by tools/font2c.py",
        "// Each glyph is 8 bytes; byte 0 is the top row, bit 7 is the leftmost pixel.",
        "// This header is private to the video cards and is not used by the emulator core.",
        "",
        "static const uint64_t font8x8[] = {",
    ]
    for i in range(0, 256, 4):
        vals = [glyph_to_u64(glyphs[j]) for j in range(i, min(i + 4, 256))]
        line = "    " + ", ".join(f"0x{v:016X}" for v in vals)
        if i + 4 < 256:
            line += ","
        lines.append(line)
    lines.extend([
        "};",
        "",
        "#define FONT8X8_WIDTH  8",
        "#define FONT8X8_HEIGHT 8",
        "#define FONT8X8_COUNT  (sizeof(font8x8) / sizeof(font8x8[0]))",
        "",
        "#endif // FONT8X8_H",
    ])
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    root = Path(__file__).resolve().parent.parent
    src = root / "cp4378x8.txt"
    dst = root / "src" / "videocards" / "font8x8.h"
    if len(sys.argv) > 1:
        src = Path(sys.argv[1])
    if len(sys.argv) > 2:
        dst = Path(sys.argv[2])
    glyphs = parse_font(src)
    emit_header(glyphs, dst)
    print(f"Generated {dst} with {len(glyphs)} glyphs from {src}")


if __name__ == "__main__":
    main()
