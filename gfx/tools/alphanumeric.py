#!/usr/bin/env python3
# Written by Claude, Anthropic's AI assistant
"""
charset_to_c.py - Convert a charset bitmap image to C byte arrays.

Usage:
    python3 charset_to_c.py <image_file> [--start-ascii N] [--output file.h]

Rules:
  - Characters are separated by one or more fully-black vertical columns.
  - Red pixels (R>150, G<80, B<80) are segment-join markers and are treated
    as BLACK in the output (bit = 0).
  - White pixels (R>150, G>150, B>150) → bit = 1.
  - All other pixels → bit = 0.
  - Within each row the leftmost pixel is the most-significant bit,
    the rightmost pixel is the least-significant bit (LSB).
  - Each character array has one entry per image row (height bytes/words).
  - If a character is wider than 8 pixels the row is stored as multiple
    bytes, most-significant byte first.
  - asciiTable[] is a const unsigned char* array indexed from start_ascii.
"""

import sys
import argparse
from PIL import Image


# ── pixel classifiers ──────────────────────────────────────────────────────

def is_white(r, g, b):
    return r > 150 and g > 150 and b > 150

def is_red(r, g, b):
    return r > 150 and g < 80 and b < 80

def pixel_bit(r, g, b):
    """Return 1 if pixel represents a lit (foreground) pixel, else 0."""
    return 1 if is_white(r, g, b) else 0

def is_separator_col(pix, x, height):
    """True only when every pixel in column x is pure black (no white, no red)."""
    for y in range(height):
        rgb = pix[x, y]
        r, g, b = rgb[0], rgb[1], rgb[2]
        if is_white(r, g, b) or is_red(r, g, b):
            return False
    return True


# ── character segmentation ─────────────────────────────────────────────────

def find_char_spans(img):
    """Return list of (x_start, x_end) inclusive spans, one per character."""
    w, h = img.size
    pix = img.load()

    spans = []
    in_char = False
    x_start = 0

    for x in range(w):
        sep = is_separator_col(pix, x, h)
        if not sep and not in_char:
            x_start = x
            in_char = True
        elif sep and in_char:
            spans.append((x_start, x - 1))
            in_char = False

    if in_char:
        spans.append((x_start, w - 1))

    return spans


# ── row → bytes ────────────────────────────────────────────────────────────

def row_to_bytes(pix, x_start, x_end, y):
    """
    Pack one row of a character into bytes.
    Leftmost pixel = MSB of the entire value; rightmost = LSB.
    Returns list of ints (big-endian, MSB byte first).
    """
    char_width = x_end - x_start + 1

    # Build integer value: leftmost pixel → highest bit
    value = 0
    for i, x in enumerate(range(x_start, x_end + 1)):
        rgb = pix[x, y]
        r, g, b = rgb[0], rgb[1], rgb[2]
        bit = pixel_bit(r, g, b)
        bit_pos = char_width - 1 - i   # leftmost → MSB
        value |= (bit << bit_pos)

    # Split into bytes, MSB byte first
    n_bytes = (char_width + 7) // 8
    result = []
    for i in range(n_bytes - 1, -1, -1):
        result.append((value >> (i * 8)) & 0xFF)
    return result


# ── C output helpers ───────────────────────────────────────────────────────

def c_array_name(ascii_code):
    return f"char_{ascii_code:03d}"

def char_to_c(pix, x_start, x_end, height, ascii_code, prefix):
    """Return C declaration string for one character."""
    name = c_array_name(ascii_code)
    char_width = x_end - x_start + 1
    n_bytes = (char_width + 7) // 8
    height_macro = f"{prefix.upper()}_FONT_HEIGHT"

    printable = chr(ascii_code)
    safe = printable if printable.isprintable() and printable not in ('"', '\\', '/') else f"0x{ascii_code:02X}"

    lines = []
    lines.append(f"/* '{safe}'  ASCII {ascii_code}  width={char_width}px  {n_bytes} byte(s)/row */")
    size_expr = height_macro if n_bytes == 1 else f"{height_macro} * {n_bytes}"
    lines.append(f"static const unsigned char {name}[{size_expr}] = {{")

    row_strings = []
    for y in range(height):
        row_bytes = row_to_bytes(pix, x_start, x_end, y)
        bin_vals = ", ".join(f"0b{b:08b}" for b in row_bytes)
        emoji = "".join(
            "🟨" if pixel_bit(*pix[x, y][:3]) else "🟦"
            for x in range(x_start, x_end + 1)
        )
        comma = "," if y < height - 1 else " "
        row_strings.append(f"    {bin_vals}{comma}  // {emoji}")

    lines.append("\n".join(row_strings))
    lines.append("};")
    return "\n".join(lines)


def widths_array(spans, empty_flags, start_ascii, prefix):
    """Return a C definition of the <prefix>_charWidths[] array."""
    name = f"{prefix}_charWidths"
    lines = [f"const unsigned char {name}[] = {{"]
    entries = []
    for i, ((xs, xe), empty) in enumerate(zip(spans, empty_flags)):
        code = start_ascii + i
        w = (xe - xs + 1) if (not empty or code == 32) else 0
        safe = chr(code) if chr(code).isprintable() and chr(code) not in ('"', '\\') else f"0x{code:02X}"
        entries.append(f"    {w:3d}  /* '{safe}' ASCII {code} */")
    lines.append(",\n".join(entries))
    lines.append("};")
    return "\n".join(lines)


def is_empty_char(pix, x_start, x_end, height):
    """True if the character has no white (visible) pixels at all."""
    for y in range(height):
        for x in range(x_start, x_end + 1):
            if pixel_bit(*pix[x, y][:3]):
                return False
    return True


def master_table(spans, empty_flags, start_ascii, prefix):
    """Return the <prefix>_asciiTable[] pointer array definition.
    Empty characters get a 0 (NULL) entry instead of an array pointer."""
    n = len(spans)
    name = f"{prefix}_asciiTable"
    lines = [
        f"/* {name}[i] → character for ASCII (i + {start_ascii}), 0 if no glyph */",
        f"const unsigned char *const {name}[{n}] = {{",
    ]
    entries = []
    for i, (_, empty) in enumerate(zip(spans, empty_flags)):
        code = start_ascii + i
        safe = chr(code) if chr(code).isprintable() and chr(code) not in ('"', '\\') else f"0x{code:02X}"
        ptr = "0" if empty else c_array_name(code)
        entries.append(f"    {ptr}  /* '{safe}' ASCII {code} */")
    lines.append(",\n".join(entries))
    lines.append("};")
    return "\n".join(lines)


def build_header(prefix, basename, image_path, w, h, spans, start_ascii):
    n = len(spans)
    lines = [
        f"/* Auto-generated by charset_to_c.py  —  DO NOT EDIT */",
        f"/* Source: {image_path}  size: {w}x{h}  chars: {n} */",
        f"",
        f"#pragma once",
        f"",
        f"#include <stdint.h>",
        f"",
        f"#define {prefix.upper()}_FONT_HEIGHT      {h}",
        f"#define {prefix.upper()}_FONT_FIRST_CHAR  {start_ascii}",
        f"#define {prefix.upper()}_FONT_CHAR_COUNT  {n}",
        f"",
        f"extern const unsigned char *const {prefix}_asciiTable[{n}];",
        f"extern const unsigned char  {prefix}_charWidths[{n}];",
        f"",
    ]
    return "\n".join(lines)


def build_source(prefix, basename, image_path, w, h, spans, start_ascii, pix):
    lines = [
        f"/* Auto-generated by charset_to_c.py  —  DO NOT EDIT */",
        f"/* Source: {image_path}  size: {w}x{h}  chars: {len(spans)} */",
        f"",
        f'#include "{basename}.h"',
        f"",
    ]

    empty_flags = []
    for i, (xs, xe) in enumerate(spans):
        code = start_ascii + i
        empty = is_empty_char(pix, xs, xe, h)
        empty_flags.append(empty)
        if not empty:
            lines.append(char_to_c(pix, xs, xe, h, code, prefix))
            lines.append("")

    lines.append(widths_array(spans, empty_flags, start_ascii, prefix))
    lines.append("")
    lines.append(master_table(spans, empty_flags, start_ascii, prefix))
    lines.append("")

    return "\n".join(lines)


# ── main ───────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Convert charset image to C arrays.")
    parser.add_argument("image", help="Path to the charset image file")
    parser.add_argument("--output", required=True,
                        help="Output prefix (generates <prefix>.c and <prefix>.h)")
    parser.add_argument("--start-ascii", type=int, default=32,
                        help="ASCII code of the first character in the image (default: 32 = space)")
    args = parser.parse_args()

    img = Image.open(args.image).convert("RGB")
    w, h = img.size
    pix = img.load()

    spans = find_char_spans(img)

    if not spans:
        print("ERROR: no characters found — check that separator columns are pure black.", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(spans)} character(s), image height = {h}px", file=sys.stderr)
    for i, (xs, xe) in enumerate(spans):
        code = args.start_ascii + i
        print(f"  [{i:3d}] ASCII {code:3d} '{chr(code) if chr(code).isprintable() else '?'}'"
              f"  x={xs}..{xe}  width={xe-xs+1}", file=sys.stderr)

    import os
    basename = os.path.basename(args.output)   # filename only, no path
    h_path = f"{args.output}.h"
    c_path = f"{args.output}.c"

    with open(h_path, "w") as f:
        f.write(build_header(basename, basename, args.image, w, h, spans, args.start_ascii))
    print(f"Written {h_path}", file=sys.stderr)

    with open(c_path, "w") as f:
        f.write(build_source(basename, basename, args.image, w, h, spans, args.start_ascii, pix))
    print(f"Written {c_path}", file=sys.stderr)


if __name__ == "__main__":
    main()