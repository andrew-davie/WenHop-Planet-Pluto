#!/usr/bin/env python3
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

def char_to_c(pix, x_start, x_end, height, ascii_code):
    """Return C declaration string for one character."""
    name = c_array_name(ascii_code)
    char_width = x_end - x_start + 1
    n_bytes = (char_width + 7) // 8

    printable = chr(ascii_code)
    safe = printable if printable.isprintable() and printable not in ('"', '\\', '/') else f"0x{ascii_code:02X}"

    lines = []
    lines.append(f"/* '{safe}'  ASCII {ascii_code}  width={char_width}px  {n_bytes} byte(s)/row */")
    lines.append(f"static const unsigned char {name}[] = {{")

    row_strings = []
    for y in range(height):
        row_bytes = row_to_bytes(pix, x_start, x_end, y)
        hex_vals = ", ".join(f"0x{b:02X}" for b in row_bytes)
        row_strings.append(f"    {hex_vals}")

    lines.append(",\n".join(row_strings))
    lines.append("};")
    return "\n".join(lines)


def widths_array(spans, start_ascii):
    """Return a C array of per-character pixel widths."""
    lines = ["static const unsigned char charWidths[] = {"]
    entries = []
    for i, (xs, xe) in enumerate(spans):
        w = xe - xs + 1
        code = start_ascii + i
        safe = chr(code) if chr(code).isprintable() and chr(code) not in ('"', '\\') else f"0x{code:02X}"
        entries.append(f"    {w:3d}  /* '{safe}' ASCII {code} */")
    lines.append(",\n".join(entries))
    lines.append("};")
    return "\n".join(lines)


def master_table(spans, start_ascii):
    """Return the asciiTable[] pointer array."""
    n = len(spans)
    lines = [
        f"/* asciiTable[i] → character for ASCII (i + {start_ascii}) */",
        f"static const unsigned char *asciiTable[{n}] = {{",
    ]
    entries = []
    for i, _ in enumerate(spans):
        code = start_ascii + i
        safe = chr(code) if chr(code).isprintable() and chr(code) not in ('"', '\\') else f"0x{code:02X}"
        entries.append(f"    {c_array_name(code)}  /* '{safe}' */")
    lines.append(",\n".join(entries))
    lines.append("};")
    return "\n".join(lines)


# ── main ───────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Convert charset image to C arrays.")
    parser.add_argument("image", help="Path to the charset image file")
    parser.add_argument("--start-ascii", type=int, default=32,
                        help="ASCII code of the first character in the image (default: 32 = space)")
    parser.add_argument("--output", default=None,
                        help="Output file (default: stdout)")
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

    # Build output
    sections = []

    sections.append(f"/* Auto-generated by charset_to_c.py */")
    sections.append(f"/* Source: {args.image}  size: {w}x{h}  chars: {len(spans)} */")
    sections.append(f"/* First char ASCII {args.start_ascii} ('{chr(args.start_ascii)}') */")
    sections.append("")
    sections.append("#pragma once")
    sections.append("#include <stdint.h>")
    sections.append("")
    sections.append(f"#define FONT_HEIGHT  {h}")
    sections.append(f"#define FONT_FIRST_CHAR  {args.start_ascii}")
    sections.append(f"#define FONT_CHAR_COUNT  {len(spans)}")
    sections.append("")

    # Individual character arrays
    for i, (xs, xe) in enumerate(spans):
        code = args.start_ascii + i
        sections.append(char_to_c(pix, xs, xe, h, code))
        sections.append("")

    # Width table
    sections.append(widths_array(spans, args.start_ascii))
    sections.append("")

    # Master pointer table
    sections.append(master_table(spans, args.start_ascii))
    sections.append("")

    output = "\n".join(sections)

    if args.output:
        with open(args.output, "w") as f:
            f.write(output)
        print(f"Written to {args.output}", file=sys.stderr)
    else:
        print(output)


if __name__ == "__main__":
    main()