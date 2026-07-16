#!/usr/bin/env python3
"""
sprite_vis.py - Add emoji visualisation comments to ONE/TWO sprite macro lines.

Usage:
    python3 sprite_vis.py input.c [output.c]

If output.c is omitted, modifies in place (writes to input.c).
"""

import re
import sys
from typing import Optional

BLACK  = '◼️'
YELLOW = '🟨'
RED    = '🟢'
BROWN  = '❌'

# Colour code -> emoji square, keyed by the 4-char suffix passed to ONE()/TWO()
# (see the colour enum: NONE, BONE, HMT0..3, BDY0..2).
COLOR_MAP = {
    'NONE': BLACK,
    'BONE': '⬜',
    'HMT0': '🟥',
    'HMT1': '🟧',
    'HMT2': YELLOW,
    'HMT3': '🟩',
    'BDY0': '🟦',
    'BDY1': '🟪',
    'BDY2': '🟫',
}
DEFAULT_ON_COLOR = YELLOW  # fallback for unrecognised/missing colour codes

COLOR_ARG_RE = re.compile(r'^[X_]+$')

SP_LINE_RE = re.compile(
    r'^(?P<indent>\s*)'
    r'(?P<call>(?:ONE|TWO)\s*\(\s*[^)]*\))'
    r'(?P<tail>[,;]?)'
    r'\s*'
    r'(?://.*)?'
    r'\s*$'
)

# Generated padding rows and border rows (stripped on re-run for idempotency)
PADDING_ROW_RE = re.compile(r'^\s*//\s+\|[' + RED + BROWN + r']+\|')
BORDER_ROW_RE  = re.compile(r'^\s*//\s+\+-+\+')

PATTERN_RE     = re.compile(r'[X_]{2,}')
ARRAY_START_RE = re.compile(r'const\s+unsigned\s+char\s+\w+\[\]\s*=\s*\{')
SCALAR_RE      = re.compile(r'^\s*(?:\w+\s*\|\s*)?(-?0x[0-9a-fA-F]+|-?\d+)\s*[,;]?\s*(?://.*)?$')
BYTE0_LINE_RE  = re.compile(r'^(?P<indent>\s*)(?:\w+\s*\|\s*)?(-?0x[0-9a-fA-F]+|-?\d+)(?P<tail>\s*[,;]?\s*(?://[^\n]*)?)$')

COMMENT_COL = 48   # column where '// NN' starts on sprite lines


# ---------------------------------------------------------------------------
# Canvas rendering
# ---------------------------------------------------------------------------

def render_canvas_row(sprite_width: int,
                      canvas_col_start: int, canvas_width: int,
                      cx: int, is_centre_row: bool,
                      pixel_data: Optional[list],
                      col_colors: Optional[list] = None) -> str:
    result = []
    for col in range(canvas_col_start, canvas_col_start + canvas_width):
        in_sprite_h = (0 <= col < sprite_width)
        is_cx       = (col == cx)
        if is_centre_row and is_cx:
            result.append(RED)
        elif pixel_data is not None and in_sprite_h:
            if pixel_data[col] == 'X':
                on_color = col_colors[col] if col_colors else DEFAULT_ON_COLOR
                result.append(on_color)
            else:
                result.append(BLACK)
        else:
            result.append(BROWN)
    return ''.join(result)


def extract_col_colors(inner_text: str, sprite_width: int) -> list:
    """Work out the emoji colour for each column of an ONE()/TWO() sprite row,
    based on the trailing colour-code argument(s) (e.g. HMT0, BDY2, NONE)."""
    args       = [a.strip() for a in inner_text.split(',')]
    color_args = [a for a in args if a and not COLOR_ARG_RE.match(a)]
    emojis     = [COLOR_MAP.get(code, DEFAULT_ON_COLOR) for code in color_args]

    if not emojis:
        return [DEFAULT_ON_COLOR] * sprite_width

    if sprite_width == 16 and len(emojis) >= 2:
        half = sprite_width // 2
        return [emojis[0]] * half + [emojis[1]] * half

    return [emojis[0]] * sprite_width


def build_canvas_params(n_rows: int, sprite_width: int, cx: int, cy: int):
    centre_row_from_top = (n_rows - 1) - cy

    canvas_col_start = min(0, cx)
    canvas_col_end   = max(sprite_width, cx + 1)
    canvas_width     = canvas_col_end - canvas_col_start

    extra_rows_above = max(0, -centre_row_from_top)
    extra_rows_below = max(0, centre_row_from_top - (n_rows - 1))

    return (canvas_col_start, canvas_width,
            extra_rows_above, extra_rows_below,
            centre_row_from_top)


# ---------------------------------------------------------------------------
# Line formatting
# ---------------------------------------------------------------------------

def format_sprite_line(raw_line: str, cx: int, is_centre_row: bool, row_idx: int,
                       canvas_col_start: int, canvas_width: int,
                       sprite_width: int) -> str:
    m = SP_LINE_RE.match(raw_line)
    if not m:
        return raw_line

    call   = m.group('call')
    indent = m.group('indent')
    tail   = m.group('tail')

    inner = re.search(r'\((.+)\)', call, re.DOTALL)
    if not inner:
        return raw_line

    patterns = PATTERN_RE.findall(inner.group(1))
    if not patterns:
        return raw_line

    flat = list(''.join(patterns))
    flat = (flat + ['_'] * sprite_width)[:sprite_width]

    col_colors = extract_col_colors(inner.group(1), sprite_width)

    visual = render_canvas_row(sprite_width, canvas_col_start, canvas_width,
                               cx, is_centre_row, flat, col_colors)

    comment = f'// {row_idx:02d}   |{visual}|'
    base    = f'{indent}{call}{tail}'
    pad     = max(1, COMMENT_COL - len(base))
    return f'{base}{" " * pad}{comment}\n'


def format_padding_row(indent: str, cx: int, is_centre_row: bool,
                       canvas_col_start: int, canvas_width: int,
                       sprite_width: int, label: str) -> str:
    """Comment-only line with // at COMMENT_COL, matching sprite lines."""
    visual = render_canvas_row(sprite_width, canvas_col_start, canvas_width,
                               cx, is_centre_row, None)
    # sprite lines have '//' at COMMENT_COL, then ' NN   |visual|'
    # padding rows: pad with spaces to COMMENT_COL, then '//       |visual|'
    prefix = f'{indent}'
    pad    = max(0, COMMENT_COL - len(prefix))
    suffix = f'  {label}' if label else ''
    return f'{prefix}{" " * pad}//      |{visual}|{suffix}\n'


def format_border_row(indent: str, canvas_width: int) -> str:
    """Horizontal border line matching canvas pixel width."""
    dashes = '-' * (canvas_width * 16 // 10 + 1)
    prefix = f'{indent}'
    pad    = max(0, COMMENT_COL - len(prefix))
    return f'{prefix}{" " * pad}//      +{dashes}+\n'


# ---------------------------------------------------------------------------
# Byte-0 rewriter
# ---------------------------------------------------------------------------

def rewrite_byte0(line: str, n_rows: int, use_double: bool) -> str:
    m = BYTE0_LINE_RE.match(line.rstrip('\n'))
    if not m:
        return line
    indent     = m.group('indent')
    tail_clean = m.group('tail').rstrip().strip()
    comma      = next((ch for ch in tail_clean if ch in ',;'), '')
    value      = f'SPRITE_DOUBLE | {n_rows}' if use_double else str(n_rows)
    return f'{indent}{value}{comma}\n'


# ---------------------------------------------------------------------------
# Main processor
# ---------------------------------------------------------------------------

def process_file(src: str) -> str:
    lines  = src.splitlines(keepends=True)
    result = []
    i      = 0

    while i < len(lines):
        if ARRAY_START_RE.search(lines[i]):
            result.append(lines[i])
            i += 1

            # Collect scalar header lines (before first ONE/TWO), skipping
            # any previously generated padding or border rows
            scalars               = []
            scalar_result_indices = []
            while i < len(lines) and not SP_LINE_RE.match(lines[i]):
                if PADDING_ROW_RE.match(lines[i]) or BORDER_ROW_RE.match(lines[i]):
                    i += 1  # discard old generated rows
                    continue
                m = SCALAR_RE.match(lines[i])
                if m:
                    scalars.append(int(m.group(1), 0))
                    scalar_result_indices.append(len(result))
                result.append(lines[i])
                i += 1

            cx = scalars[1] if len(scalars) > 1 else -1
            cy = scalars[2] if len(scalars) > 2 else -1

            # Collect all ONE/TWO lines, stripping old padding/border rows
            sprite_lines = []
            while i < len(lines) and (SP_LINE_RE.match(lines[i])
                                      or PADDING_ROW_RE.match(lines[i])
                                      or BORDER_ROW_RE.match(lines[i])):
                if SP_LINE_RE.match(lines[i]):
                    sprite_lines.append(lines[i])
                i += 1

            n_rows       = len(sprite_lines)
            use_double   = any('TWO' in l for l in sprite_lines)
            sprite_width = 16 if use_double else 8

            # Rewrite byte 0
            if scalar_result_indices:
                result[scalar_result_indices[0]] = rewrite_byte0(
                    result[scalar_result_indices[0]], n_rows, use_double)

            # Canvas geometry
            (canvas_col_start, canvas_width,
             extra_rows_above, extra_rows_below,
             centre_row_from_top) = build_canvas_params(n_rows, sprite_width, cx, cy)

            # Grab indent from first sprite line
            indent_m = SP_LINE_RE.match(sprite_lines[0]) if sprite_lines else None
            indent   = indent_m.group('indent') if indent_m else '    '

            # Top border
            result.append(format_border_row(indent, canvas_width))

            # Extra rows ABOVE the sprite
            for k in range(extra_rows_above):
                canvas_row = k - extra_rows_above
                is_cr      = (canvas_row == centre_row_from_top)
                label      = '← centre' if is_cr else ''
                result.append(format_padding_row(
                    indent, cx, is_cr,
                    canvas_col_start, canvas_width, sprite_width, label))

            # Sprite rows
            for row_idx, sline in enumerate(sprite_lines):
                is_cr = (row_idx == centre_row_from_top)
                result.append(format_sprite_line(
                    sline, cx, is_cr, row_idx,
                    canvas_col_start, canvas_width, sprite_width))

            # Extra rows BELOW the sprite
            for k in range(extra_rows_below):
                canvas_row = n_rows + k
                is_cr      = (canvas_row == centre_row_from_top)
                label      = '← centre' if is_cr else ''
                result.append(format_padding_row(
                    indent, cx, is_cr,
                    canvas_col_start, canvas_width, sprite_width, label))

            # Bottom border
            result.append(format_border_row(indent, canvas_width))

        else:
            result.append(lines[i])
            i += 1

    return ''.join(result)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    infile  = sys.argv[1]
    outfile = sys.argv[2] if len(sys.argv) > 2 else infile

    with open(infile, 'r', encoding='utf-8') as f:
        src = f.read()

    result = process_file(src)

    with open(outfile, 'w', encoding='utf-8') as f:
        f.write(result)

    print(f"Written to {outfile}")


if __name__ == '__main__':
    main()