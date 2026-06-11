#!/usr/bin/env python3
"""
find_dup_structs.py - Find duplicate struct definitions in C source files.

Compares struct bodies (ignoring names and comments) and reports matching pairs.

Usage:
    python find_dup_structs.py <file> [file ...]
    python find_dup_structs.py --help
"""

import re
import sys
import argparse
from collections import defaultdict
from pathlib import Path


def strip_comments(src: str) -> str:
    """Remove // and /* */ comments from C source."""
    # Block comments
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.DOTALL)
    # Line comments
    src = re.sub(r'//[^\n]*', '', src)
    return src


def normalise_body(body: str) -> str:
    """
    Normalise a struct body for comparison:
    - collapse whitespace
    - strip leading/trailing whitespace
    """
    # Collapse all whitespace runs to a single space
    body = re.sub(r'\s+', ' ', body).strip()
    return body


def extract_structs(src: str) -> list[tuple[str, str, int]]:
    """
    Extract structs from C source.
    Returns list of (name, normalised_body, line_number).

    Handles:
        struct Foo { ... };
        typedef struct { ... } Foo;
        typedef struct Bar { ... } Foo;

    Does NOT handle nested structs as separate entities — they are
    treated as part of the outer struct body.
    """
    # Strip comments first
    src_clean = strip_comments(src)

    # Track line numbers: build a map from char offset -> line number in original src
    # We work on cleaned src, so line numbers are approximate but close enough.
    results = []

    # Regex to find the start of a struct definition.
    # Matches: [typedef] struct [tag] {
    struct_start_re = re.compile(
        r'\b(?P<typedef>typedef\s+)?struct\s*(?P<tag>\w+)?\s*\{',
        re.DOTALL
    )

    i = 0
    lines_before = src_clean[:0].count('\n')

    for m in struct_start_re.finditer(src_clean):
        start = m.start()
        brace_open = m.end() - 1  # points at '{'

        # Count braces to find the matching '}'
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
            # Unmatched brace — malformed source, skip
            continue

        body_end = pos  # index of closing '}'
        body = src_clean[brace_open + 1:body_end]

        # After '}', look for the typedef alias or plain semicolon
        # e.g.  } Foo; or } Foo, *FooPtr;
        tail = src_clean[body_end + 1:]
        alias_m = re.match(r'\s*(?P<alias>[\w\s,*]+)?\s*;', tail)
        alias = None
        if alias_m and alias_m.group('alias'):
            # Last word in the alias list is the typedef name
            words = alias_m.group('alias').split()
            if words:
                alias = words[-1].strip('*,')

        tag = m.group('tag')

        # Determine the best name
        if alias:
            name = alias
        elif tag:
            name = tag
        else:
            name = '<anonymous>'

        line_no = src_clean[:start].count('\n') + 1
        norm = normalise_body(body)

        results.append((name, norm, line_no))

    return results


def find_duplicates(structs: list[tuple[str, str, str, int]]):
    """
    structs: list of (filename, name, norm_body, line_no)
    Returns dict mapping norm_body -> list of (filename, name, line_no)
    where the list has 2+ entries (i.e. duplicates).
    """
    body_map = defaultdict(list)
    for filename, name, norm, line_no in structs:
        body_map[norm].append((filename, name, line_no))

    return {body: entries for body, entries in body_map.items() if len(entries) > 1}


def main():
    parser = argparse.ArgumentParser(
        description='Find duplicate struct definitions in C source files.'
    )
    parser.add_argument('files', nargs='+', metavar='FILE',
                        help='C source files to scan')
    parser.add_argument('--show-body', action='store_true',
                        help='Print the normalised struct body for each duplicate group')
    args = parser.parse_args()

    all_structs = []  # (filename, name, norm_body, line_no)

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

        structs = extract_structs(src)
        for name, norm, line_no in structs:
            all_structs.append((str(path), name, norm, line_no))

    if not all_structs:
        print('No structs found.')
        return 0

    duplicates = find_duplicates(all_structs)

    if not duplicates:
        print('No duplicate structs found.')
        return 0

    print(f'Found {len(duplicates)} duplicate struct body/bodies:\n')

    for group_idx, (body, entries) in enumerate(duplicates.items(), 1):
        print(f'--- Group {group_idx} ({len(entries)} instances) ---')
        for filename, name, line_no in entries:
            print(f'  {filename}:{line_no}  struct {name}')
        if args.show_body:
            members = [m.strip() for m in body.split(';') if m.strip()]
            print('  Body: {')
            for mem in members:
                print(f'    {mem};')
            print('  }')
        print()

    return 1 if duplicates else 0


if __name__ == '__main__':
    sys.exit(main())
