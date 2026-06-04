#!/usr/bin/env python3
"""
Reformats C source files containing const byte arrays of exactly 30 bytes.
Each group of 3 bytes = 1 row of 5 pixels (R bits, G bits, B bits).
Outputs 3 bytes per line in binary format with ASCII art comment.
"""

import re
import sys
import os

SHADE = [' ', '.', ':', '+', 'o', 'O', '#', '@']

def pixel_char(r_bit, g_bit, b_bit):
    return SHADE[r_bit * 4 + g_bit * 2 + b_bit]


def row_ascii(r_byte, g_byte, b_byte):
    chars = []
    for bit_pos in range(4, -1, -1):  # bits 4..0 = pixels left to right
        r = (r_byte >> bit_pos) & 1
        g = (g_byte >> bit_pos) & 1
        b = (b_byte >> bit_pos) & 1
        chars.append(pixel_char(r, g, b))
    return ''.join(chars)


def parse_byte(token):
    token = token.strip().rstrip(',')
    if token.startswith(('0b', '0B')):
        return int(token, 2)
    elif token.startswith(('0x', '0X')):
        return int(token, 16)
    else:
        return int(token, 10)


def format_byte(val):
    return f'0b{val:05b}'


def reformat_array_body(bytes_list, indent):
    """indent is the horizontal indent of the array declaration line (spaces only)."""
    assert len(bytes_list) == 30
    lines = []
    for row in range(10):
        r = bytes_list[row * 3]
        g = bytes_list[row * 3 + 1]
        b = bytes_list[row * 3 + 2]
        art = row_ascii(r, g, b)
        b0, b1, b2 = format_byte(r), format_byte(g), format_byte(b)
        comma = ',' #''' if row == 9 else ','
        lines.append(f'{indent}    {b0}, {b1}, {b2}{comma}  // {row:1d} |{art}|')
    return lines


def extract_bytes_from_tokens(tokens):
    result = []
    for t in tokens:
        t = t.strip()
        if not t:
            continue
        try:
            result.append(parse_byte(t))
        except ValueError:
            return None
    return result


ARRAY_START_RE = re.compile(
    r'(?P<indent>[^\S\n]*)'          # horizontal whitespace only (no newlines)
    r'(?P<prefix>(?:(?:static|const|unsigned|volatile)\s+)*)'
    r'(?P<type>(?:unsigned\s+)?(?:uint8_t|uint16_t|int8_t|char|byte|PROGMEM\s+\w+|\w+))'
    r'\s+'
    r'(?P<name>\w+)'
    r'\s*\[.*?\]\s*=\s*\{',
    re.MULTILINE
)


def process_source(source):
    out = []
    pos = 0
    text = source

    for m in ARRAY_START_RE.finditer(text):
        out.append(text[pos:m.end()] + '\n')
        pos = m.end()

        indent = m.group('indent')  # spaces only, no newlines
        array_start_pos = m.end()

        # Find matching closing brace
        depth = 1
        i = array_start_pos
        while i < len(text) and depth > 0:
            if text[i] == '{':
                depth += 1
            elif text[i] == '}':
                depth -= 1
            i += 1

        body_text = text[array_start_pos:i - 1]
        after_brace = text[i - 1]
        pos = i

        # Strip comments before tokenising
        body_stripped = re.sub(r'//[^\n]*', '', body_text)
        body_stripped = re.sub(r'/\*.*?\*/', '', body_stripped, flags=re.DOTALL)

        raw_tokens = re.split(r'[\s,]+', body_stripped.strip())
        tokens = [t for t in raw_tokens if t]

        if len(tokens) != 30:
            out.append(body_text)
            out.append(after_brace)
            continue

        bytes_list = extract_bytes_from_tokens(tokens)
        if bytes_list is None or len(bytes_list) != 30:
            out.append(body_text)
            out.append(after_brace)
            continue

        reformatted = reformat_array_body(bytes_list, indent)
        out.append('\n' + '\n'.join(reformatted) + '\n' + indent)
        out.append(after_brace)

    out.append(text[pos:])
    return ''.join(out)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source.c> [output.c]")
        print("  If output is omitted, file is modified in place (backup saved as .bak)")
        sys.exit(1)

    infile = sys.argv[1]
    outfile = sys.argv[2] if len(sys.argv) >= 3 else None

    with open(infile, 'r', encoding='utf-8') as f:
        source = f.read()

    result = process_source(source)

    if outfile:
        with open(outfile, 'w', encoding='utf-8') as f:
            f.write(result)
        print(f"Written to {outfile}")
    else:
        backup = infile + '.bak'
        os.rename(infile, backup)
        with open(infile, 'w', encoding='utf-8') as f:
            f.write(result)
        print(f"Modified in place. Backup at {backup}")


if __name__ == '__main__':
    main()