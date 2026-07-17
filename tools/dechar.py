#!/usr/bin/env python3
"""
dechar.py - Render Atari 2600 character-set bitplane data (characterset.c)
into a single indexed-palette PNG "character sheet".

Format assumptions (WenHop-Planet-Pluto characterset.c):
  - A "character" is a `const unsigned char _CHAR_NAME[CHAR_Y] = { ... };`
    array definition (arrays declared with a bare `[]`, e.g.
    _CHAR_WATERFLOW_0, are animation-frame data in a different layout and
    are skipped).
  - Each character is stored as 10 rows, each row being 3 bytes, 5 pixels
    wide, one bit per pixel per byte. The three bytes are additive R/G/B
    light primaries -- which source byte drives which channel is set by
    BYTE_ORDER below (a 3-letter permutation of "RGB", e.g. "RGB" means
    byte 1 = red, byte 2 = green, byte 3 = blue; "BGR" reverses it). Each
    channel bit is either fully off (0) or fully on (255), so combining the
    three bits per pixel gives one of the 8 RGB-cube corner colours (black,
    blue, green/red/etc. depending on BYTE_ORDER, up to white).

    Colours are computed directly from the bits, not guessed from the
    emoji-art comments in the source (those are hand-drawn approximations,
    not a reliable colour reference).

Usage:
    python3 dechar.py [input.c] [output.png]

Defaults to the WenHop-Planet-Pluto characterset.c and writes
characterset.png next to this script if no arguments are given.
"""

import re
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("This tool requires Pillow. Install with: pip install Pillow")

CHAR_W = 5
CHAR_H = 10
CHARS_PER_ROW = 16
LEADING_BLANK_CHARS = 2  # charSet[] reserves slots 0/1 (CH_BLANK, CH_PLACEHOLDER) as blank

# Which channel each of the 3 source bytes (in file order) drives.
# byte 1 = red, byte 2 = green, byte 3 = blue.
# Previously tried: "GRB", "RBG".
BYTE_ORDER = "RGB"

# 3-bit (R,G,B) value -> RGB colour, computed directly as additive primary
# mixing (each bit is a channel fully on or fully off) -- NOT derived from
# the emoji-art comments, which are approximate and not a colour reference.
PALETTE = {idx: ((idx >> 2 & 1) * 255, (idx >> 1 & 1) * 255, (idx & 1) * 255) for idx in range(8)}

CHAR_DEF_RE = re.compile(
    r"const\s+unsigned\s+char\s+(_CHAR_\w+)\s*\[\s*CHAR_Y\s*\]\s*=\s*\{(.*?)\};",
    re.DOTALL,
)
BIN_LITERAL_RE = re.compile(r"0b([01]{5})")


def find_characters(src_text):
    """Return an ordered list of (name, [row0_indices, row1_indices, ...])."""
    characters = []
    for match in CHAR_DEF_RE.finditer(src_text):
        # Skip definitions that are commented out (e.g. `// const unsigned
        # char _CHAR_LAVA_2[CHAR_Y] = { ... };`), which show up in the
        # source as a disabled/incomplete character stub.
        line_start = src_text.rfind("\n", 0, match.start()) + 1
        line = src_text[line_start:match.start()]
        if line.strip().startswith("//"):
            continue

        name = match.group(1)
        body = match.group(2)
        bits = BIN_LITERAL_RE.findall(body)
        if len(bits) != CHAR_H * 3:
            raise ValueError(
                f"{name}: expected {CHAR_H * 3} binary literals (10 rows x R/G/B), "
                f"found {len(bits)}"
            )
        rows = []
        for row in range(CHAR_H):
            byte_bits = (bits[row * 3], bits[row * 3 + 1], bits[row * 3 + 2])
            row_indices = []
            for col in range(CHAR_W):
                channels = {ch: int(byte_bits[pos][col]) for pos, ch in enumerate(BYTE_ORDER)}
                r, g, b = channels["R"], channels["G"], channels["B"]
                row_indices.append((r << 2) | (g << 1) | b)
            rows.append(row_indices)
        characters.append((name, rows))
    return characters


def build_image(characters, chars_per_row=CHARS_PER_ROW, leading_blanks=LEADING_BLANK_CHARS):
    blank_rows = [[0] * CHAR_W for _ in range(CHAR_H)]
    tiles = [("_BLANK_", blank_rows) for _ in range(leading_blanks)] + characters

    total = len(tiles)
    grid_rows = (total + chars_per_row - 1) // chars_per_row
    width = chars_per_row * CHAR_W
    height = grid_rows * CHAR_H

    img = Image.new("P", (width, height), 0)
    palette_bytes = []
    for i in range(8):
        palette_bytes.extend(PALETTE[i])
    palette_bytes.extend([0, 0, 0] * (256 - 8))
    img.putpalette(palette_bytes)

    pixels = img.load()
    for i, (name, rows) in enumerate(tiles):
        tile_x = (i % chars_per_row) * CHAR_W
        tile_y = (i // chars_per_row) * CHAR_H
        for row_i, row in enumerate(rows):
            for col_i, idx in enumerate(row):
                pixels[tile_x + col_i, tile_y + row_i] = idx

    return img, tiles


def main():
    script_dir = Path(__file__).resolve().parent
    default_input = script_dir.parent / "main" / "characterset.c"
    default_output = script_dir / "characterset.png"

    input_path = Path(sys.argv[1]) if len(sys.argv) > 1 else default_input
    output_path = Path(sys.argv[2]) if len(sys.argv) > 2 else default_output

    if not input_path.exists():
        sys.exit(f"Input file not found: {input_path}")

    src_text = input_path.read_text()
    characters = find_characters(src_text)

    if not characters:
        sys.exit("No _CHAR_NAME[CHAR_Y] definitions found in input file.")

    img, tiles = build_image(characters)
    img.save(output_path)

    print(f"Parsed {len(characters)} characters from {input_path.name}")
    print(f"Wrote {img.width}x{img.height} indexed PNG ({len(tiles)} tiles, "
          f"{CHARS_PER_ROW} across) to {output_path}")
    for i, (name, _) in enumerate(tiles):
        print(f"  [{i:03d}] {name}")


if __name__ == "__main__":
    main()
