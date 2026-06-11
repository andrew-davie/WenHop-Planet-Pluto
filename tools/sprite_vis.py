#!/usr/bin/env python3
"""
sprite_vis.py - Add emoji visualisation comments to SP/SP2 sprite macro lines.

Usage:
    python3 sprite_vis.py input.c [output.c]

If output.c is omitted, modifies in place (writes to input.c).
"""

import re
import sys

BLACK = '◼️'
YELLOW = '🟨'

# Matches SP( pat1, pat2, ...) or SP2( pat1, ...) lines, with optional trailing // NN comment
# Captures: indent, full SP/SP2 call (up to closing paren), trailing comma/semicolon, 
# existing comment number if any.
SP_LINE_RE = re.compile(
    r'^(?P<indent>\s*)'
    r'(?P<call>SP2?\s*\([^)]*\))'
    r'(?P<tail>[,;]?)'
    r'\s*'
    r'(?://\s*(?P<num>\d+).*)?'
    r'\s*$'
)

# Extract bit patterns (sequences of X and _) from inside macro parens
PATTERN_RE = re.compile(r'[X_]{2,}')


def pattern_to_emoji(pat: str) -> str:
    return ''.join(YELLOW if c == 'X' else BLACK for c in pat)


def process_line(line: str) -> str:
    m = SP_LINE_RE.match(line)
    if not m:
        return line

    call = m.group('call')
    # Find all bit patterns inside the parens
    inner = re.search(r'\((.+)\)', call, re.DOTALL)
    if not inner:
        return line

    patterns = PATTERN_RE.findall(inner.group(1))
    if not patterns:
        return line

    visual = ''.join(pattern_to_emoji(p) for p in patterns)

    indent = m.group('indent')
    tail = m.group('tail')
    num = m.group('num')

    num_part = f' {num}' if num is not None else ''
    comment = f'// {num_part.strip()} {visual}' if num is not None else f'// {visual}'
    # Tidy: "// 00 🟨..."
    if num is not None:
        comment = f'// {int(num):02d} {visual}'

    base = f'{indent}{call}{tail}'
    # Align comment: pad to column 50 (adjust if you like)
    pad = max(1, 50 - len(base))
    return f'{base}{" " * pad}{comment}\n'


def process_file(src: str) -> str:
    out = []
    for line in src.splitlines(keepends=True):
        out.append(process_line(line))
    return ''.join(out)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    infile = sys.argv[1]
    outfile = sys.argv[2] if len(sys.argv) > 2 else infile

    with open(infile, 'r', encoding='utf-8') as f:
        src = f.read()

    result = process_file(src)

    with open(outfile, 'w', encoding='utf-8') as f:
        f.write(result)

    print(f"Written to {outfile}")


if __name__ == '__main__':
    main()
