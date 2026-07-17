#!/usr/bin/env python3
"""
Build a deduplicated Atari 2600 character set from one or more images.

Each image is sliced into WIDTH x DEPTH pixel blocks ("characters"). Identical
blocks are deduplicated into a shared charset; every image also gets a grid
mapping its blocks to character IDs. Each character's rows are packed into
three bit planes (R/G/B) suitable for the TIA. Output is a matching .c/.h pair:

  - The .c file's per-character comment lists every (x, y) source grid
    position that character came from, e.g. `//     20  (3,2), (7,4)`.
  - The .h file gets a `#define CHAR_MAP_<x>_<y> <id>` for every source grid
    cell, giving each occupied grid position a compile-time name for the
    character ID it became. Character 0 is always reserved for blank (by
    convention, not by inspecting pixels), so cells that dedupe to id 0 are
    skipped -- no CHAR_MAP define and no position list in the .c comment.
"""

#from __future__ import annotations

import argparse
import glob
import re
from dataclasses import dataclass, field
from pathlib import Path

from PIL import Image

BIT_PLANES = 3  # rows are packed into R, G and B planes

# Coloured-block emoji for each combination of a pixel's R/G/B plane bits,
# indexed by (r | g << 1 | b << 2). One-to-one assignment (no repeats)
# against the current gfx/characterset.png palette colours, with index 4
# pinned to brown (its natural match) and everything else re-solved around
# that to minimize total RGB Euclidean distance. 🟥 red goes unused -- once
# brown is reserved for index 4, nothing else in the palette is close to
# red. This is a slightly worse global fit than letting the optimizer pick
# index 4 freely (it bumped index 1 to brown instead), but keeps brown where
# you'd expect it for a brown-ish colour.
PIXEL_EMOJI = ["⬛", "🟩", "🟦", "🟨", "🟫", "🟧", "🟪", "⬜"]


@dataclass
class CharSet:
    """Deduplicated pixel blocks, plus each source image's block grid."""

    width: int
    depth: int
    ids: dict[tuple, int] = field(default_factory=dict)
    grids: dict[str, list[list[int]]] = field(default_factory=dict)

    def __len__(self) -> int:
        return len(self.ids)

    def by_id(self):
        """Characters in assignment order, as (pixels, id) pairs."""
        return sorted(self.ids.items(), key=lambda kv: kv[1])

    def add_image(self, filename: str) -> None:
        """Slice an image into blocks, deduplicating into the shared charset."""
        img = Image.open(filename)
        w, h = img.size

        grid = []
        for y in range(0, h, self.depth):
            row = []
            for x in range(0, w, self.width):
                block = img.crop((x, y, x + self.width, y + self.depth))
                pixels = tuple(block.getdata())
                if pixels not in self.ids:
                    self.ids[pixels] = len(self.ids)
                row.append(self.ids[pixels])
            grid.append(row)

        self.grids[filename] = grid

    def occurrences(self) -> dict[int, list[tuple[str, int, int]]]:
        """id -> ordered list of (filename, x, y) source grid positions."""
        occ: dict[int, list[tuple[str, int, int]]] = {}
        for filename, grid in self.grids.items():
            for y, row in enumerate(grid):
                for x, idx in enumerate(row):
                    occ.setdefault(idx, []).append((filename, x, y))
        return occ


BLANK_ID = 0  # character 0 is always reserved for blank, by convention


def sanitize_ident(text: str) -> str:
    """Turn arbitrary text into a valid fragment of a C identifier."""
    return re.sub(r"\W", "_", text)


def pack_bits(values: list[int], bit_index: int) -> int:
    """Pack one bit from each value into a single integer, first value highest."""
    result = 0
    for i, value in enumerate(values):
        bit = (value >> bit_index) & 1
        result |= bit << (len(values) - 1 - i)
    return result


def character_rows(pixels: tuple, width: int, depth: int) -> list[tuple[list[str], str]]:
    """A character's rows: packed R/G/B fields, paired with an emoji preview of the row."""
    rows = []
    for y in range(depth):
        line = list(pixels[y * width:(y + 1) * width])
        fields = [f"0b{pack_bits(line, plane):0{width}b}" for plane in range(BIT_PLANES)]
        preview = "".join(PIXEL_EMOJI[value & 0b111] for value in line)
        rows.append((fields, preview))
    return rows


def map_varname(filename: str) -> str:
    """C identifier for a source image's block-grid array."""
    return filename.replace(".", "_").replace("/", "_") + "_map"


def write_header(path: Path, name: str, charset: CharSet) -> None:
    lines = [
        "#pragma once\n",
        f"#define CARS_CHAR_WIDTH {charset.width}",
        f"#define CARS_CHAR_HEIGHT {charset.depth}\n",
        "typedef struct {",
        "    unsigned char data[CARS_CHAR_HEIGHT * 3]; // RGB",
        "} character;\n",
        f"extern const character {name}[{len(charset)}];\n",
    ]

    for filename, grid in charset.grids.items():
        varname = map_varname(filename)
        lines.append(f"extern const unsigned char {varname}[{len(grid)}][{len(grid[0])}];")
    if charset.grids:
        lines.append("")

    # One #define per non-blank source grid cell, naming the character ID it
    # became. Character 0 is always reserved for blank, so other cells that
    # deduplicated to id 0 are skipped as pointless repeats -- except (0,0)
    # itself, which always gets its define since it's the canonical blank
    # reference for that image.
    multi_file = len(charset.grids) > 1
    define_lines = []
    for filename, grid in charset.grids.items():
        stem = sanitize_ident(Path(filename).stem)
        for y, row in enumerate(grid):
            for x, idx in enumerate(row):
                if idx == BLANK_ID and (x, y) != (0, 0):
                    continue
                macro = f"CHAR_MAP_{stem}_{x}_{y}" if multi_file else f"CHAR_MAP_{x}_{y}"
                define_lines.append(f"#define {macro} {idx}")
    if define_lines:
        lines.extend(define_lines)
        lines.append("")

    lines.append("//EOF")
    path.write_text("\n".join(lines) + "\n")


def write_source(path: Path, include_stem: str, name: str, charset: CharSet) -> None:
    lines = [f'#include "{include_stem}.h"', ""]

    occurrences = charset.occurrences()
    multi_file = len(charset.grids) > 1

    lines.append(f"const character {name}[{len(charset)}] = {{")
    for pixels, idx in charset.by_id():
        rows = character_rows(pixels, charset.width, charset.depth)
        row_prefixes = ["        " + ", ".join(fields) + ", " for fields, _ in rows]
        comment_col = len(row_prefixes[0])

        # Character 0 (blank) tends to occur everywhere, so its position
        # list is just noise -- skip it, same as it skips a CHAR_MAP define.
        positions = [] if idx == BLANK_ID else occurrences.get(idx, [])
        if multi_file:
            pos_text = ", ".join(f"{fn}:({x},{y})" for fn, x, y in positions)
        else:
            pos_text = ", ".join(f"({x},{y})" for _, x, y in positions)
        header_comment = f"//     {idx}"
        if pos_text:
            header_comment += f"  {pos_text}"

        lines.append("    { {".ljust(comment_col) + header_comment)
        for line_no, (prefix, (_, preview)) in enumerate(zip(row_prefixes, rows)):
            lines.append(prefix + f"// {line_no:2d}  {preview}")
        lines.append("    } },")
    lines.append("};")
    lines.append("")

    for filename, grid in charset.grids.items():
        varname = map_varname(filename)
        lines.append(
            f"__attribute__((used)) const unsigned char "
            f"{varname}[{len(grid)}][{len(grid[0])}] = {{"
        )
        for row in grid:
            lines.append("    { " + ", ".join(str(n) for n in row) + " },")
        lines.append("};")
        lines.append("")

    lines.append("//EOF")
    path.write_text("\n".join(lines) + "\n")


def resolve_files(patterns: list[str]) -> list[str]:
    """Expand glob patterns, warning about any that match nothing."""
    files = []
    for pattern in patterns:
        matched = glob.glob(pattern)
        if not matched:
            print(f"Warning: pattern '{pattern}' did not match any files")
        files.extend(matched)
    return files


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build a deduplicated Atari 2600 character set from one or more images."
    )
    parser.add_argument("-w", "--width", type=int, required=True, help="Character width in pixels")
    parser.add_argument("-d", "--depth", type=int, required=True, help="Character height in pixels")
    parser.add_argument("-o", "--output", required=True, help="Output filename (without extension)")
    parser.add_argument("-n", "--name", required=True, help="C array/struct variable name")
    parser.add_argument("files", nargs="+", help="Input filenames or glob patterns")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    charset = CharSet(args.width, args.depth)
    for filename in resolve_files(args.files):
        charset.add_image(filename)

    write_header(Path(f"{args.output}.h"), args.name, charset)
    write_source(Path(f"{args.output}.c"), args.output, args.name, charset)
    print(f"C and H output written to {args.output} ({len(charset)} unique characters)")


if __name__ == "__main__":
    main()
