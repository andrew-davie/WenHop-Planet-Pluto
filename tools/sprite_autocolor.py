#!/usr/bin/env python3
"""
sprite_autocolor.py - Auto-colour flat-placeholder shape_* arrays in
main/sprites.c using a colour-by-height template learned from the frames
that already have real (hand-authored) shading.

Every properly-shaded humanoid pose in sprites.c (FRAME_STAND, the WALK/
WALKUP/WALKDOWN cycles, HUNCH, PUSH/PUSH2, ARMS_IN_AIR, MINE_UP_0/1) follows
the same head-to-toe colour banding regardless of pose: a 4-tone HMT0..HMT3
gradient down the rounded helmet, a 2-tone BDY1/BDY2 band across the boxy
torso, then the palette repeats (HMT1/HMT2/HMT0) down the legs/boots. This
script measures those bands (as a fraction of frame height) from the
existing shaded frames and applies the same bands to every row of every
"flat" frame (one that currently uses a single placeholder colour, e.g.
HMT0 throughout) by row position.

Frames are skipped (left untouched) if:
  - every "on" pixel already uses more than one colour (already shaded), or
  - the only colour used is BONE (bare-bone skeleton frames are correctly
    monochrome - not a placeholder awaiting a suit colour), or
  - the frame has fewer than MIN_ROWS rows (too short to be a full-body
    pose - these are small icons/fragments, not humanoid figures).

Usage:
    python3 tools/sprite_autocolor.py [main/sprites.c] [--dry-run]
"""

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_SPRITES_C = REPO_ROOT / "main" / "sprites.c"

MIN_ROWS = 13

ARRAY_START_RE = re.compile(r'const\s+unsigned\s+char\s+shape_(\w+)\[\]\s*=\s*\{')
ONE_RE = re.compile(r'(ONE\(\s*[X_]{8}\s*,\s*)(\w+)(\s*\))')
TWO_RE = re.compile(r'(TWO\(\s*[X_]{8}\s*,\s*[X_]{8}\s*,\s*)(\w+)(\s*,\s*)(\w+)(\s*\))')

# Learned from the 19 hand-shaded humanoid template poses in sprites.c
# (see conversation - aggregated colour-by-height-decile analysis).
BANDS = [
    (0.00, 0.12, 'HMT0'),
    (0.12, 0.28, 'HMT1'),
    (0.28, 0.40, 'HMT2'),
    (0.40, 0.52, 'HMT3'),
    (0.52, 0.64, 'BDY1'),
    (0.64, 0.80, 'BDY2'),
    (0.80, 0.84, 'HMT1'),
    (0.84, 0.96, 'HMT2'),
    (0.96, 1.01, 'HMT0'),
]


def band_color(frac):
    for lo, hi, col in BANDS:
        if lo <= frac < hi:
            return col
    return BANDS[-1][2]


def parse_shapes(lines):
    shapes = []
    i = 0
    while i < len(lines):
        m = ARRAY_START_RE.search(lines[i])
        if m:
            name = m.group(1)
            decl_line = i
            j = i + 1
            row_line_idxs = []
            colors = set()
            while j < len(lines) and '};' not in lines[j]:
                if TWO_RE.search(lines[j]) or ONE_RE.search(lines[j]):
                    row_line_idxs.append(j)
                    for mt in TWO_RE.finditer(lines[j]):
                        colors.add(mt.group(2))
                        colors.add(mt.group(4))
                    for mo in ONE_RE.finditer(lines[j]):
                        colors.add(mo.group(2))
                j += 1
            shapes.append({
                'name': name,
                'decl_line': decl_line,
                'row_line_idxs': row_line_idxs,
                'colors': colors,
            })
            i = j + 1
        else:
            i += 1
    return shapes


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('sprites_c', nargs='?', default=str(DEFAULT_SPRITES_C))
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    path = Path(args.sprites_c)
    lines = path.read_text().splitlines(keepends=True)
    shapes = parse_shapes(lines)

    recoloured, skipped = [], []
    for s in shapes:
        n_rows = len(s['row_line_idxs'])
        if n_rows == 0:
            continue
        if len(s['colors']) > 1:
            continue  # already shaded
        if s['colors'] == {'BONE'}:
            skipped.append((s['name'], 'bone-only (skeleton)'))
            continue
        if n_rows < MIN_ROWS:
            skipped.append((s['name'], f'only {n_rows} rows (< {MIN_ROWS}, likely not a full pose)'))
            continue
        recoloured.append(s['name'])
        for row_idx, line_idx in enumerate(s['row_line_idxs']):
            frac = row_idx / (n_rows - 1) if n_rows > 1 else 0.0
            col = band_color(frac)
            line = lines[line_idx]
            mt = TWO_RE.search(line)
            if mt:
                line = TWO_RE.sub(lambda m: f'{m.group(1)}{col}{m.group(3)}{col}{m.group(5)}', line)
            else:
                line = ONE_RE.sub(lambda m: f'{m.group(1)}{col}{m.group(3)}', line)
            lines[line_idx] = line

    print(f"Recoloured: {len(recoloured)}")
    for n in recoloured:
        print(f"  {n}")
    print(f"\nSkipped: {len(skipped)}")
    for n, reason in skipped:
        print(f"  {n} - {reason}")

    if not args.dry_run:
        path.write_text(''.join(lines))
        print(f"\nWritten to {path}")


if __name__ == '__main__':
    main()
