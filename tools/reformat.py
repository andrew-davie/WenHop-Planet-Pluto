#!/usr/bin/env python3
"""
reformat.py — reformat all entries in planetInfo.c in-place.

Usage:
    python3 reformat.py [path/to/planetInfo.c]

Processes every entry in the file (active and inactive) and rewraps the
text to fit the 48px display, respecting existing } paragraph breaks.

Last-line fallback order:
  1. Standard:           content ≤ 32px  (closing '#' fits on same line)
  2. Drop full-stop:     content ≤ 32px  (period removed from last word)
  3. Drop closing quote: content ≤ 48px  (quote may clip slightly)
  4. Drop both:          content ≤ 48px  (period removed, quote may clip)

Entries exceeding 11 display lines are flagged and left untouched.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from review_format import _rewrap_adaptive

DEFAULT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'planetInfo.c')
ENTRY_RE = re.compile(r'(.*\{"=\\")(.+?)(#",)(\d+)(\},.*\n?)')


def count_display_lines(pipe_text):
    lns = pipe_text.split('|')
    return len(lns) + sum(s.count('}') for s in lns)


def reformat(path):
    with open(path) as f:
        lines = f.readlines()

    changed = unchanged = flagged = 0

    for i, line in enumerate(lines):
        m = ENTRY_RE.match(line)
        if not m:
            continue

        prefix, old_pipe, mid, stars, suffix = m.groups()

        # Flatten: pipes become spaces, } paragraph marks are preserved
        flat = old_pipe.replace('|', ' ')

        new_pipe, note = _rewrap_adaptive(flat)

        n = count_display_lines(new_pipe)
        if n > 11:
            print(f"L{i+1}: FLAGGED — {n} display lines (skipped)")
            flagged += 1
            continue

        if new_pipe != old_pipe:
            lines[i] = f'{prefix}{new_pipe}{mid}{stars}{suffix}'
            changed += 1
            if note:
                print(f"L{i+1}: {note}")
        else:
            unchanged += 1

    with open(path, 'w') as f:
        f.writelines(lines)

    total = changed + unchanged + flagged
    print(f"\n{total} entries: {changed} reformatted, {unchanged} unchanged, {flagged} flagged.")


if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PATH
    if not os.path.exists(path):
        print(f"File not found: {path}", file=sys.stderr)
        sys.exit(1)
    reformat(path)
