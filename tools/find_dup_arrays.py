#!/usr/bin/env python3
"""
find_dup_arrays.py - Find duplicate C array definitions.

Compares array initialisers (ignoring names, comments, and whitespace)
and reports matching groups.

Usage:
    python find_dup_arrays.py <file> [file ...]
    python find_dup_arrays.py *.c *.h --show-body
"""

import re
import sys
import argparse
from collections import defaultdict
from pathlib import Path


def strip_comments(src: str) -> str:
    """Remove // and /* */ comments from C source."""
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.DOTALL)
    src = re.sub(r'//[^\n]*', '', src)
    return src


def normalise_body(body: str) -> str:
    """Collapse all whitespace to single spaces and strip ends."""
    return re.sub(r'\s+', ' ', body).strip()


def extract_arrays(src: str, filename: str) -> list[tuple[str, str, str, int]]:
    """
    Extract array definitions from C source.
    Returns list of (filename, name, normalised_initialiser, line_no).

    Matches patterns like:
        [static] [const] [volatile] type name[...] = { ... };
    The key insight: find '= {', then brace-match to find the closing '}'.
    """
    src_clean = strip_comments(src)

    # Match the array declaration up to and including '= {'
    # Captures the array name (last word before the '[...]')
    decl_re = re.compile(
        r'(?:(?:static|const|volatile|extern|inline)\s+)*'  # optional qualifiers
        r'(?:\w+\s+)+'                                        # type (one or more words)
        r'(?P<name>\w+)'                                      # array name
        r'\s*\[.*?\]'                                         # [...] — dimension(s)
        r'(?:\s*\[.*?\])*'                                    # optional further dimensions
        r'\s*=\s*\{',                                         # = {
        re.DOTALL
    )

    results = []

    for m in decl_re.finditer(src_clean):
        name = m.group('name')
        brace_open = m.end() - 1  # index of '{'

        # Brace-match to find closing '}'
        depth = 0
        pos = brace_open
        while pos < len(src_clean):
            ch = src_clean[pos]
            if ch == '{':
                depth += 1
            elif ch == '}':
                depth -= 1
                if depth == 0:
                    break
            pos += 1

        if depth != 0:
            continue  # malformed

        body = src_clean[brace_open + 1:pos]
        norm = normalise_body(body)
        line_no = src_clean[:m.start()].count('\n') + 1
        results.append((filename, name, norm, line_no))

    return results


def find_duplicates(arrays: list[tuple[str, str, str, int]]) -> dict:
    """Return groups with 2+ members, keyed by normalised body."""
    body_map = defaultdict(list)
    for filename, name, norm, line_no in arrays:
        body_map[norm].append((filename, name, line_no))
    return {body: entries for body, entries in body_map.items() if len(entries) > 1}


def main():
    parser = argparse.ArgumentParser(
        description='Find duplicate C array definitions.'
    )
    parser.add_argument('files', nargs='+', metavar='FILE',
                        help='C source/header files to scan')
    parser.add_argument('--show-body', action='store_true',
                        help='Print the normalised initialiser for each group')
    parser.add_argument('--max-body', type=int, default=200,
                        metavar='N',
                        help='Truncate displayed body to N chars (default: 200)')
    args = parser.parse_args()

    all_arrays = []

    for path_str in args.files:
        path = Path(path_str)
        if not path.exists():
            print(f'ERROR: {path_str}: file not found', file=sys.stderr)
            continue
        try:
            src = path.read_text(encoding='utf-8', errors='replace')
        except OSError as e:
            print(f'ERROR: {path_str}: {e}', file=sys.stderr)
            continue

        found = extract_arrays(src, str(path))
        all_arrays.extend(found)

    if not all_arrays:
        print('No array definitions found.')
        return 0

    duplicates = find_duplicates(all_arrays)

    if not duplicates:
        print('No duplicate arrays found.')
        return 0

    print(f'Found {len(duplicates)} duplicate array initialiser(s):\n')

    for group_idx, (body, entries) in enumerate(duplicates.items(), 1):
        print(f'--- Group {group_idx} ({len(entries)} instances) ---')
        for filename, name, line_no in entries:
            print(f'  {filename}:{line_no}  {name}')
        if args.show_body:
            display = body if len(body) <= args.max_body else body[:args.max_body] + ' ...'
            print(f'  Initialiser: {{ {display} }}')
        print()

    return 1 if duplicates else 0


if __name__ == '__main__':
    sys.exit(main())
