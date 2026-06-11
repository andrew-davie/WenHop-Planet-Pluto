#!/usr/bin/env python3
"""
sprite_vis.py - Add emoji visualisation comments to SP/SP2 sprite macro lines.

Usage:
    python3 sprite_vis.py input.c [output.c]

If output.c is omitted, modifies in place (writes to input.c).
"""

import re
import sys

BLACK  = '◼️'
YELLOW = '🟩'
RED    = '🟥'

SP_LINE_RE = re.compile(
    r'^(?P<indent>\s*)'
    r'(?P<call>(?:ONE|TWO)\s*\(\s*[^)]*\))'
    r'(?P<tail>[,;]?)'
    r'\s*'
    r'(?://\s*(?P<num>\d+).*)?'
    r'\s*$'
)

PATTERN_RE     = re.compile(r'[X_]{2,}')
ARRAY_START_RE = re.compile(r'const\s+unsigned\s+char\s+\w+\[\]\s*=\s*\{')
ARRAY_NAME_RE  = re.compile(r'const\s+unsigned\s+char\s+(\w+)')
SCALAR_RE      = re.compile(r'^\s*(?:\w+\s*\|\s*)?(0x[0-9a-fA-F]+|\d+)\s*[,;]?\s*(?://.*)?$')
# Matches the first scalar line (byte 0) to allow rewriting it
BYTE0_LINE_RE  = re.compile(r'^(?P<indent>\s*)(?:\w+\s*\|\s*)?(0x[0-9a-fA-F]+|\d+)(?P<tail>\s*[,;]?\s*(?://[^\n]*)?)$')


def pattern_to_emoji(pat: str, col_start: int, cx: int, is_centre_row: bool) -> str:
    result = []
    for i, c in enumerate(pat):
        col = col_start + i
        if is_centre_row and col == cx:
            result.append(RED)
        else:
            result.append(YELLOW if c == 'X' else BLACK)
    return ''.join(result)


def build_visual(patterns: list[str], cx: int, is_centre_row: bool) -> str:
    col = 0
    parts = []
    for pat in patterns:
        parts.append(pattern_to_emoji(pat, col, cx, is_centre_row))
        col += len(pat)
    return ''.join(parts)


def format_line(raw_line: str, cx: int, is_centre_row: bool) -> str:
    m = SP_LINE_RE.match(raw_line)
    if not m:
        return raw_line

    call   = m.group('call')
    indent = m.group('indent')
    tail   = m.group('tail')
    num    = m.group('num')

    inner = re.search(r'\((.+)\)', call, re.DOTALL)
    if not inner:
        return raw_line

    patterns = PATTERN_RE.findall(inner.group(1))
    if not patterns:
        return raw_line

    # Suppress centre marker if cx is outside horizontal range
    n_cols = sum(len(p) for p in patterns)
    if cx < 0 or cx >= n_cols:
        is_centre_row = False

    visual = build_visual(patterns, cx, is_centre_row)

    if num is not None:
        comment = f'// {int(num):02d}   |{visual}|'
    else:
        comment = f'//   |{visual}|'

    base = f'{indent}{call}{tail}'
    pad  = max(1, 48 - len(base))
    return f'{base}{" " * pad}{comment}\n'


def rewrite_byte0(line: str, n_rows: int, use_double: bool) -> str:
    m = BYTE0_LINE_RE.match(line.rstrip('\n'))
    if not m:
        return line
    indent = m.group('indent')
    tail   = m.group('tail').rstrip()
    # Preserve trailing comma/semicolon but strip any existing comment
    tail_clean = tail.strip()
    comma = ''
    for ch in tail_clean:
        if ch in ',;':
            comma = ch
            break
    if use_double:
        value = f'SPRITE_DOUBLE | {n_rows}'
    else:
        value = str(n_rows)
    return f'{indent}{value}{comma}\n'


def process_file(src: str) -> str:
    lines  = src.splitlines(keepends=True)
    result = []
    i      = 0

    while i < len(lines):
        if ARRAY_START_RE.search(lines[i]):
            block_start = i
            result.append(lines[i])
            i += 1

            # Collect scalar lines (before first ONE/TWO), recording their
            # positions in result[] so we can rewrite byte 0 later.
            scalars        = []
            scalar_result_indices = []  # where each scalar line lands in result[]
            pre_sprite_lines = []
            while i < len(lines) and not SP_LINE_RE.match(lines[i]):
                m = SCALAR_RE.match(lines[i])
                if m:
                    scalars.append(int(m.group(1), 0))
                    scalar_result_indices.append(len(result))
                result.append(lines[i])
                pre_sprite_lines.append(lines[i])
                i += 1

            cx = scalars[1] if len(scalars) > 1 else -1
            cy = scalars[2] if len(scalars) > 2 else -1

            # Collect all ONE/TWO lines
            sprite_lines = []
            while i < len(lines) and SP_LINE_RE.match(lines[i]):
                sprite_lines.append(lines[i])
                i += 1

            n_rows     = len(sprite_lines)
            use_double = any('TWO' in l for l in sprite_lines)

            # Rewrite byte 0 in result[]
            if scalar_result_indices:
                byte0_idx = scalar_result_indices[0]
                result[byte0_idx] = rewrite_byte0(result[byte0_idx], n_rows, use_double)

            # Declared rows now comes from what we just wrote
            declared_rows = n_rows  # by definition, since we just set it

            centre_row_from_top = (declared_rows - 1) - cy if 0 <= cy < declared_rows else -1
            if not (0 <= centre_row_from_top < n_rows):
                centre_row_from_top = -1

            for row_idx, sline in enumerate(sprite_lines):
                is_centre_row = (row_idx == centre_row_from_top)
                result.append(format_line(sline, cx, is_centre_row))

        else:
            result.append(lines[i])
            i += 1

    return ''.join(result)


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