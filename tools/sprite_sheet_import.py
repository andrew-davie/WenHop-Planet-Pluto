#!/usr/bin/env python3
"""
sprite_sheet_import.py - Import new frames from a "sprite sheet" PNG into sprites.c

Given a sprite sheet image containing many frames (black=off, white=on,
red=connectivity marker used only to visually join isolated pixel groups
that belong to the same frame - discarded from the actual pixel data),
this tool:

  1. Segments the sheet into individual frames using 8-connected pixel
     grouping, with a proximity+size heuristic to fold small disconnected
     fragments (eye dots, an outstretched hand, etc.) into their parent
     frame.
  2. Drops any frame that is 100% red pixels (artist's info/annotation
     marks, e.g. arrows - not real sprite data).
  3. Compares every sheet frame (ignoring colour) against every shape_*
     array already in sprites.c.
       - If it matches an existing shape exactly, that shape gets a
         "// sheet: <file> @ (x,y)" comment recording where it lives on
         this sheet. Nothing else about it is touched.
       - If it doesn't match anything, it's a new frame: a new shape_*
         array is appended (colour placeholder HMT0 throughout, centre
         point set to bottom-row/middle), and it is wired into
         spriteShape[] (sprites.c) and enum FRAME (sprites.h).
  4. Runs sprite_vis.py over sprites.c to regenerate the emoji
     visualisation comments / byte-0 header for anything touched.

Frames already in sprites.c that are NOT found on the given sheet are
left completely alone. The tool is safe to run repeatedly (also across
multiple different sheets over time) - already-imported frames will be
recognised as matches on subsequent runs, not re-added.

Usage:
    python3 tools/sprite_sheet_import.py gfx/TIX/some-sheet.png
    python3 tools/sprite_sheet_import.py sheet.png --dry-run
"""

import argparse
import re
import subprocess
import sys
from collections import deque
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("This tool requires Pillow: pip install pillow", file=sys.stderr)
    sys.exit(1)

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_SPRITES_C = REPO_ROOT / "main" / "sprites.c"
DEFAULT_SPRITES_H = REPO_ROOT / "main" / "sprites.h"
SPRITE_VIS = Path(__file__).resolve().parent / "sprite_vis.py"

PLACEHOLDER_COLOR = "HMT0"

# --- merge heuristic tuning -------------------------------------------------
# A component smaller than MIN_HOST pixels is a candidate "fragment" that
# may belong to a nearby larger frame rather than being its own frame.
MIN_HOST = 50
# A fragment may only merge into a host that is at least this much bigger
# (fragment/host area ratio).
RATIO_THRESH = 0.5
# Max gap (Chebyshev distance between bounding boxes) for a fragment to be
# considered part of a host. Real inter-frame gutters on this sheet run
# ~5-9px; genuine stray fragments (eye dots, reaching arms) sit within 0-2px
# of their parent, so this stays comfortably below the gutter width.
GAP_THRESH = 4


# ============================================================================
# Image segmentation
# ============================================================================

def classify(px):
    r, g, b = int(px[0]), int(px[1]), int(px[2])
    if r > 200 and g > 200 and b > 200:
        return 'W'
    if r < 60 and g < 60 and b < 60:
        return 'K'
    if r > 150 and g < 120 and b < 120:
        return 'R'
    # anything else (anti-aliasing, stray colour) - treat as off
    return 'K'


def load_grid(path):
    im = Image.open(path).convert('RGB')
    w, h = im.size
    arr = im.load()
    grid = [[classify(arr[x, y]) for x in range(w)] for y in range(h)]
    return grid, w, h


def find_components(grid, w, h):
    visited = [[False] * w for _ in range(h)]
    comps = []
    for y in range(h):
        for x in range(w):
            if grid[y][x] != 'K' and not visited[y][x]:
                q = deque([(y, x)])
                visited[y][x] = True
                pixels = []
                while q:
                    cy, cx = q.popleft()
                    pixels.append((cy, cx))
                    for dy in (-1, 0, 1):
                        for dx in (-1, 0, 1):
                            if dy == 0 and dx == 0:
                                continue
                            ny, nx = cy + dy, cx + dx
                            if (0 <= ny < h and 0 <= nx < w
                                    and not visited[ny][nx] and grid[ny][nx] != 'K'):
                                visited[ny][nx] = True
                                q.append((ny, nx))
                comps.append(pixels)
    return comps


def bbox_of(pixels):
    ys = [a for a, b in pixels]
    xs = [b for a, b in pixels]
    return min(xs), min(ys), max(xs), max(ys)


def bbox_gap(b1, b2):
    x0, y0, x1, y1 = b1
    X0, Y0, X1, Y1 = b2
    dx = max(X0 - x1, x0 - X1, 0)
    dy = max(Y0 - y1, y0 - Y1, 0)
    return max(dx, dy)


def segment_frames(grid, w, h):
    """Return a list of dicts: bbox, is_double, n_rows, rows (X/_ strings),
    plus counts of dropped all-red info marks for reporting."""
    comps = find_components(grid, w, h)

    items = []
    dropped_all_red = 0
    for p in comps:
        wcount = sum(1 for a, b in p if grid[a][b] == 'W')
        if wcount == 0:
            dropped_all_red += 1
            continue
        items.append({'pixels': set(p), 'bbox': bbox_of(p), 'w': wcount})

    # Fold small stray fragments into their nearest, sufficiently-larger host.
    changed = True
    while changed:
        changed = False
        for i, frag in enumerate(items):
            if frag is None or frag['w'] >= MIN_HOST:
                continue
            best, best_gap = None, None
            for j, host in enumerate(items):
                if i == j or host is None or host['w'] < MIN_HOST:
                    continue
                if frag['w'] / host['w'] > RATIO_THRESH:
                    continue
                g = bbox_gap(frag['bbox'], host['bbox'])
                if g > GAP_THRESH:
                    continue
                if best is None or g < best_gap or (g == best_gap and host['w'] > best['w']):
                    best, best_gap = host, g
            if best is not None:
                best['pixels'] |= frag['pixels']
                xs = [b for a, b in best['pixels']]
                ys = [a for a, b in best['pixels']]
                best['bbox'] = (min(xs), min(ys), max(xs), max(ys))
                best['w'] += frag['w']
                items[i] = None
                changed = True

    frames = [it for it in items if it is not None]
    frames.sort(key=lambda f: (f['bbox'][1], f['bbox'][0]))

    for f in frames:
        x0, y0, x1, y1 = f['bbox']
        width_used = x1 - x0 + 1
        is_double = width_used > 8
        local_width = 16 if is_double else 8
        rows = []
        for y in range(y0, y1 + 1):
            row = []
            for c in range(local_width):
                x = x0 + c
                on = x <= x1 and x < w and grid[y][x] == 'W'
                row.append('X' if on else '_')
            rows.append(''.join(row))
        f['is_double'] = is_double
        f['local_width'] = local_width
        f['width_used'] = width_used
        f['n_rows'] = len(rows)
        f['rows'] = rows
        if local_width > 16:
            f['warning'] = f'width {width_used}px exceeds 16px double-sprite limit'

    return frames, dropped_all_red


# ============================================================================
# Parsing the existing sprites.c
# ============================================================================

ARRAY_START_RE = re.compile(r'const\s+unsigned\s+char\s+shape_(\w+)\[\]\s*=\s*\{')
ONE_RE = re.compile(r'ONE\(\s*([X_]{8})\s*,')
TWO_RE = re.compile(r'TWO\(\s*([X_]{8})\s*,\s*([X_]{8})\s*,')
SHEET_COMMENT_RE = re.compile(r'^\s*//\s*sheet:\s*(\S+)\s*@\s*\((\d+),\s*(\d+)\)\s*$')


def parse_existing_shapes(lines):
    """Return list of dicts describing each shape_* array: name, is_double,
    n_rows, rows (tuple of bit strings), decl_line (index of the `const...{`
    line), end_line (index of the matching `};`)."""
    shapes = []
    i = 0
    while i < len(lines):
        m = ARRAY_START_RE.search(lines[i])
        if m:
            name = m.group(1)
            decl_line = i
            j = i + 1
            rows = []
            is_double = None
            while j < len(lines) and '};' not in lines[j]:
                mt = TWO_RE.search(lines[j])
                mo = ONE_RE.search(lines[j])
                if mt:
                    rows.append(mt.group(1) + mt.group(2))
                    is_double = True
                elif mo:
                    rows.append(mo.group(1))
                    is_double = False
                j += 1
            shapes.append({
                'name': name,
                'is_double': bool(is_double),
                'n_rows': len(rows),
                'rows': tuple(rows),
                'decl_line': decl_line,
                'end_line': j,
            })
            i = j + 1
        else:
            i += 1
    return shapes


def find_preceding_sheet_comments(lines, decl_line):
    """Scan upward from decl_line over blank lines and '// sheet: ...'
    comment lines. Return (first_comment_line_index_or_decl_line,
    {sheet_filename: line_index})."""
    idx = decl_line - 1
    comment_lines = {}
    top = decl_line
    while idx >= 0:
        stripped = lines[idx].strip()
        if stripped == '':
            idx -= 1
            continue
        m = SHEET_COMMENT_RE.match(lines[idx])
        if m:
            comment_lines[m.group(1)] = idx
            top = idx
            idx -= 1
            continue
        break
    return top, comment_lines


# ============================================================================
# Code generation
# ============================================================================

def frame_name(x0, y0):
    return f"FRAME_NEW_X{x0:03d}_Y{y0:03d}"


def format_new_shape_block(name, frame):
    is_double = frame['is_double']
    n_rows = frame['n_rows']
    width_used = frame['width_used']
    cx = width_used // 2
    cy = 0  # bottom row, per convention: centre_row_from_top = n_rows-1 - cy

    lines = []
    lines.append(f"const unsigned char shape_{name}[] = {{\n")
    lines.append("\n")
    byte0 = f"SPRITE_DOUBLE | {n_rows}" if is_double else f"{n_rows}"
    lines.append(f"    {byte0},\n")
    lines.append(f"    {cx},\n")
    lines.append(f"    {cy},\n")
    lines.append("\n")
    for row in frame['rows']:
        if is_double:
            a, b = row[:8], row[8:]
            lines.append(f"    TWO( {a}, {b}, {PLACEHOLDER_COLOR}, {PLACEHOLDER_COLOR} ),\n")
        else:
            lines.append(f"    ONE( {row}, {PLACEHOLDER_COLOR} ),\n")
    lines.append("};\n")
    lines.append("\n")
    return lines


def sheet_comment_line(sheet_name, x0, y0):
    return f"// sheet: {sheet_name} @ ({x0},{y0})\n"


# ============================================================================
# Main
# ============================================================================

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('sheet', help='Path to the sprite sheet PNG')
    ap.add_argument('--sprites-c', default=str(DEFAULT_SPRITES_C))
    ap.add_argument('--sprites-h', default=str(DEFAULT_SPRITES_H))
    ap.add_argument('--dry-run', action='store_true', help="Report only, don't modify files")
    ap.add_argument('--no-vis', action='store_true', help="Don't run sprite_vis.py afterwards")
    args = ap.parse_args()

    sheet_path = Path(args.sheet)
    sheet_name = sheet_path.name
    sprites_c_path = Path(args.sprites_c)
    sprites_h_path = Path(args.sprites_h)

    grid, w, h = load_grid(sheet_path)
    frames, dropped_all_red = segment_frames(grid, w, h)

    for f in frames:
        if f.get('warning'):
            print(f"WARNING: frame @ {f['bbox'][:2]}: {f['warning']}", file=sys.stderr)

    sprites_c_lines = sprites_c_path.read_text().splitlines(keepends=True)
    existing = parse_existing_shapes(sprites_c_lines)

    by_sig = {}
    for e in existing:
        by_sig.setdefault((e['is_double'], e['n_rows'], e['rows']), e['name'])

    matched = []   # (frame, existing_shape_name)
    new_frames = []
    for f in frames:
        sig = (f['is_double'], f['n_rows'], tuple(f['rows']))
        name = by_sig.get(sig)
        if name:
            matched.append((f, name))
        else:
            new_frames.append(f)

    print(f"Sheet: {sheet_name}  ({w}x{h})")
    print(f"Segmented frames: {len(frames)}  (dropped {dropped_all_red} all-red info marks)")
    print(f"Matched existing: {len(matched)}   New: {len(new_frames)}")
    print()

    if args.dry_run:
        for f, name in matched:
            x0, y0 = f['bbox'][0], f['bbox'][1]
            print(f"  MATCH  {name:30s} @ ({x0},{y0})")
        for f in new_frames:
            x0, y0 = f['bbox'][0], f['bbox'][1]
            print(f"  NEW    {frame_name(x0, y0):30s} @ ({x0},{y0})  "
                  f"{'double' if f['is_double'] else 'single'}  {f['n_rows']} rows")
        print("\n(dry run - no files modified)")
        return

    # --- Update comments on matched existing shapes -------------------------
    name_to_shape = {e['name']: e for e in existing}
    # Collect edits keyed by decl_line so we can apply them without
    # invalidating other line indices (we insert/replace comment lines
    # directly above each declaration, processing bottom-to-top).
    edits = []  # (top_line_idx, decl_line_idx, new_comment_lines)
    for f, name in matched:
        shape = name_to_shape[name]
        x0, y0 = f['bbox'][0], f['bbox'][1]
        top, comments = find_preceding_sheet_comments(sprites_c_lines, shape['decl_line'])
        comments[sheet_name] = None  # mark this sheet as needing (re)write
        new_lines = []
        # preserve insertion order: existing sheets first (in original top-down
        # order), updating this sheet's line in place; append if new
        ordered_files = sorted(comments.keys(),
                                key=lambda fn: comments[fn] if comments[fn] is not None else 1 << 30)
        for fn in ordered_files:
            new_lines.append(sheet_comment_line(fn, x0, y0) if fn == sheet_name
                              else sprites_c_lines[comments[fn]])
        edits.append((top, shape['decl_line'], new_lines))

    edits.sort(key=lambda e: e[0], reverse=True)
    for top, decl_line, new_lines in edits:
        sprites_c_lines[top:decl_line] = new_lines

    # --- Append new shape blocks ---------------------------------------------
    new_names = []
    if new_frames:
        clang_on_idx = None
        for idx, line in enumerate(sprites_c_lines):
            if '// clang-format on' in line:
                clang_on_idx = idx
                break
        if clang_on_idx is None:
            print("ERROR: could not find '// clang-format on' marker in sprites.c", file=sys.stderr)
            sys.exit(1)

        insertion = []
        for f in new_frames:
            x0, y0 = f['bbox'][0], f['bbox'][1]
            name = frame_name(x0, y0)
            new_names.append(name)
            insertion.append(sheet_comment_line(sheet_name, x0, y0))
            insertion.extend(format_new_shape_block(name, f))

        sprites_c_lines[clang_on_idx:clang_on_idx] = insertion

    # --- Append to spriteShape[] table ---------------------------------------
    if new_names:
        table_start = None
        for idx, line in enumerate(sprites_c_lines):
            if 'const unsigned char *const spriteShape[]' in line:
                table_start = idx
                break
        if table_start is None:
            print("ERROR: could not find spriteShape[] table in sprites.c", file=sys.stderr)
            sys.exit(1)
        table_end = None
        for idx in range(table_start, len(sprites_c_lines)):
            if sprites_c_lines[idx].strip() == '};':
                table_end = idx
                break
        if table_end is None:
            print("ERROR: could not find end of spriteShape[] table", file=sys.stderr)
            sys.exit(1)

        existing_entries = sum(
            1 for idx in range(table_start + 1, table_end)
            if sprites_c_lines[idx].strip() and not sprites_c_lines[idx].strip().startswith('//')
        )

        table_insertion = []
        for k, name in enumerate(new_names):
            idx_num = existing_entries + k
            table_insertion.append(f"    shape_{name},      // {idx_num:02d}\n")
        sprites_c_lines[table_end:table_end] = table_insertion

    sprites_c_path.write_text(''.join(sprites_c_lines))

    # --- Update sprites.h enum -------------------------------------------------
    if new_names:
        sprites_h_lines = sprites_h_path.read_text().splitlines(keepends=True)
        frame_max_idx = None
        for idx, line in enumerate(sprites_h_lines):
            if re.search(r'\bFRAME_MAX\s*,', line):
                frame_max_idx = idx
                break
        if frame_max_idx is None:
            print("ERROR: could not find FRAME_MAX in sprites.h", file=sys.stderr)
            sys.exit(1)
        enum_insertion = [f"    {name},\n" for name in new_names]
        sprites_h_lines[frame_max_idx:frame_max_idx] = enum_insertion
        sprites_h_path.write_text(''.join(sprites_h_lines))

    # --- Regenerate visual comments -------------------------------------------
    if not args.no_vis and SPRITE_VIS.exists():
        subprocess.run([sys.executable, str(SPRITE_VIS), str(sprites_c_path)], check=True)

    # --- Report ------------------------------------------------------------
    print(f"Updated {len(matched)} existing shape(s) with sheet-location comments.")
    print(f"Added {len(new_names)} new shape(s):")
    for f, name in zip(new_frames, new_names):
        x0, y0 = f['bbox'][0], f['bbox'][1]
        print(f"  shape_{name}  @ ({x0},{y0})  "
              f"{'double' if f['is_double'] else 'single'}-width, {f['n_rows']} rows")
    if new_names:
        print("\nNew shapes use placeholder colour HMT0 throughout and a centre point")
        print("at the bottom row, middle column - review/rename/recolour as needed.")


if __name__ == '__main__':
    main()
