#!/usr/bin/env python3
"""
Reformats C source files containing const byte arrays of exactly 30 bytes.
Each group of 3 bytes = 1 row of 5 pixels (R bits, G bits, B bits).
Outputs 3 bytes per line in binary format with ASCII art comment.

ASCII shading map (combined RGB brightness 0-7):
  0 → ' '  1 → '.'  2 → '@'  3 → '#'
  4 → '░'  5 → '▒'  6 → '▓'  7 → '█'
"""

import re
import sys
import os

SHADE = [' ', '.', '@', '#', '░', '▒', '▓', '█']


def pixel_char(r_bit, g_bit, b_bit):
    """Map 3 single bits to a shade character (0–7)."""
    return SHADE[r_bit * 4 + g_bit * 2 + b_bit]


def row_ascii(r_byte, g_byte, b_byte):
    """Generate 5-char ASCII art string from one row's RGB bytes."""
    chars = []
    for bit_pos in range(4, -1, -1):  # bits 4..0 = pixels left to right
        r = (r_byte >> bit_pos) & 1
        g = (g_byte >> bit_pos) & 1
        b = (b_byte >> bit_pos) & 1
        chars.append(pixel_char(r, g, b))
    return ''.join(chars)


def parse_byte(token):
    """Parse a C integer literal (binary 0b…, hex 0x…, or decimal)."""
    token = token.strip().rstrip(',')
    if token.startswith(('0b', '0B')):
        return int(token, 2)
    elif token.startswith(('0x', '0X')):
        return int(token, 16)
    else:
        return int(token, 10)


def format_byte(val):
    """Format byte as 8-digit binary literal."""
    return f'0b{val:08b}'


def reformat_array_body(bytes_list, indent):
    """
    Given exactly 30 bytes, produce reformatted lines:
    3 bytes per line, binary, with // comment showing row index and ASCII art.
    Returns list of strings (no trailing newline).
    """
    assert len(bytes_list) == 30, f"Expected 30 bytes, got {len(bytes_list)}"
    lines = []
    for row in range(10):
        r = bytes_list[row * 3]
        g = bytes_list[row * 3 + 1]
        b = bytes_list[row * 3 + 2]
        art = row_ascii(r, g, b)
        b0 = format_byte(r)
        b1 = format_byte(g)
        b2 = format_byte(b)
        is_last_row = (row == 9)
        comma = '' if is_last_row else ','
        line = f'{indent}    {b0}, {b1}, {b2}{comma}  // row {row:2d}: |{art}|'
        lines.append(line)
    return lines


def extract_bytes_from_tokens(tokens):
    """Parse token list into list of ints. Returns None if any token is unparseable."""
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


# Matches:  [static] [const] [unsigned] TYPE name[] = {
ARRAY_START_RE = re.compile(
    r'^(?P<indent>\s*)'
    r'(?P<prefix>(?:(?:static|const|unsigned|volatile)\s+)*)'
    r'(?P<type>(?:unsigned\s+)?(?:uint8_t|uint16_t|int8_t|char|byte|PROGMEM\s+\w+|\w+))'
    r'\s+'
    r'(?P<name>\w+)'
    r'\s*\[.*?\]\s*=\s*\{',
    re.MULTILINE
)


def process_source(source):
    """
    Scan source for const array declarations, collect their bodies,
    check for exactly 30 bytes, reformat them.
    """
    out = []
    pos = 0
    text = source

    for m in ARRAY_START_RE.finditer(text):
        # Emit everything up to this match unchanged
        out.append(text[pos:m.end()])
        pos = m.end()

        indent = m.group('indent')
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

        body_text = text[array_start_pos:i - 1]  # between { and }
        after_brace = text[i - 1]                 # the closing '}'
        pos = i  # continue after '}'

        # Strip comments from body before tokenising
        body_stripped = re.sub(r'//[^\n]*', '', body_text)
        body_stripped = re.sub(r'/\*.*?\*/', '', body_stripped, flags=re.DOTALL)

        # Tokenise
        raw_tokens = re.split(r'[\s,]+', body_stripped.strip())
        tokens = [t for t in raw_tokens if t]

        if len(tokens) != 30:
            # Not a 30-byte array — emit unchanged
            out.append(body_text)
            out.append(after_brace)
            continue

        bytes_list = extract_bytes_from_tokens(tokens)
        if bytes_list is None or len(bytes_list) != 30:
            out.append(body_text)
            out.append(after_brace)
            continue

        # Emit reformatted body
        reformatted = reformat_array_body(bytes_list, indent)
        out.append('\n')
        out.append('\n'.join(reformatted))
        out.append('\n' + indent)
        out.append(after_brace)

    # Remainder of file
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
