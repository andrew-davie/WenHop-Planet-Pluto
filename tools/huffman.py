#!/usr/bin/env python3
"""
huffman_compress.py — Shared Huffman compression for pixel-display phrase strings.

Each phrase is compressed independently using a shared codebook built from all
phrases together. The C decoder is self-contained and needs no external tables.

Input format (one or more lines):
    {"=\"TEXT#",STARS},

Usage:
    python3 huffman_compress.py phrases.c [output_prefix]

Outputs:
    <prefix>.h   — decoder prototype + Phrase struct
    <prefix>.c   — Huffman tables, compressed arrays, decoder, phrases[]
"""

import sys, re, heapq
from collections import Counter

# ─── Parse ────────────────────────────────────────────────────────────────────

def parse_phrases(src):
    """Return list of (full_string, stars) from C initializer lines.
    full_string is the actual display string: ="TEXT#
    """
    out = []
    for m in re.finditer(r'\{"=\\"(.*?)#",(\d+)\}', src):
        full  = '="' + m.group(1) + '#'
        stars = int(m.group(2))
        if stars > 5:
            print(f"WARNING: stars={stars} clamped to 5 for: {full[:40]!r}", file=sys.stderr)
            stars = 5
        out.append((full, stars))
    return out

# ─── Huffman tree ─────────────────────────────────────────────────────────────

class _N:
    __slots__ = ('s', 'f', 'L', 'R')
    def __init__(self, s=None, f=0, L=None, R=None):
        self.s, self.f, self.L, self.R = s, f, L, R
    def __lt__(self, o): return self.f < o.f

def _build_tree(freq):
    h = [_N(s=s, f=f) for s, f in freq.items()]
    heapq.heapify(h)
    if len(h) == 1:
        return h[0]
    while len(h) > 1:
        a, b = heapq.heappop(h), heapq.heappop(h)
        heapq.heappush(h, _N(f=a.f + b.f, L=a, R=b))
    return h[0]

def _get_lengths(node, depth=0, out=None):
    if out is None: out = {}
    if node.s is not None:
        out[node.s] = max(depth, 1)
    else:
        _get_lengths(node.L, depth + 1, out)
        _get_lengths(node.R, depth + 1, out)
    return out

# ─── Canonical codes ──────────────────────────────────────────────────────────

def make_canonical(lengths):
    """Return (codes, sorted_syms).
    codes[sym] = (int_code, int_length).
    sorted_syms: symbols in canonical order (by length, then ord).
    """
    syms = sorted(lengths, key=lambda s: (lengths[s], ord(s)))
    code, prev_len, codes = 0, 0, {}
    for s in syms:
        l = lengths[s]
        if prev_len:
            code <<= (l - prev_len)
        codes[s] = (code, l)
        code += 1
        prev_len = l
    return codes, syms

def make_tables(lengths, syms):
    """Return (count, first_code, start_idx, max_len) arrays indexed by code length."""
    max_len = max(lengths.values())
    count = [0] * (max_len + 1)
    for s in syms:
        count[lengths[s]] += 1

    first = [0] * (max_len + 1)
    start = [0] * (max_len + 1)
    c, idx = 0, 0
    for l in range(1, max_len + 1):
        first[l] = c
        start[l] = idx
        c = (c + count[l]) << 1
        idx += count[l]
    return count, first, start, max_len

# ─── Encode / Decode (Python) ─────────────────────────────────────────────────

def encode(text, codes):
    """Return (bytes, num_padding_bits)."""
    bits = []
    for ch in text:
        c, l = codes[ch]
        for i in range(l - 1, -1, -1):
            bits.append((c >> i) & 1)
    pad = (-len(bits)) % 8
    bits += [0] * pad
    data = bytes(
        int(''.join(str(b) for b in bits[i:i+8]), 2)
        for i in range(0, len(bits), 8)
    )
    return data, pad

def decode_py(data, codes):
    """Decode bytes back to string (Python, for verification)."""
    rev = {(c, l): s for s, (c, l) in codes.items()}
    bits = []
    for byte in data:
        for i in range(7, -1, -1):
            bits.append((byte >> i) & 1)
    result, pos = [], 0
    while pos < len(bits):
        for l in range(1, 32):
            if pos + l > len(bits): break
            c = int(''.join(str(b) for b in bits[pos:pos+l]), 2)
            if (c, l) in rev:
                sym = rev[(c, l)]
                pos += l
                if sym == '#':
                    return ''.join(result) + '#'
                result.append(sym)
                break
    return ''.join(result)

# ─── C generation ─────────────────────────────────────────────────────────────

_DECODER = r"""int phrase_decode(const uint8_t *src, char *dst, int max_len) {
    uint32_t win   = 0;
    int      wbits = 0, si = 0, di = 0;

    while (di < max_len - 1) {
        /* Refill bit window from src (MSB first).
           '#' terminates before we exhaust the stream, so no bounds check needed. */
        while (wbits < HUFF_MAX && wbits + 8 <= 32) {
            win   |= (uint32_t)src[si++] << (24 - wbits);
            wbits += 8;
        }

        /* Canonical decode: try each code length shortest-first. */
        for (int len = 1; len <= HUFF_MAX && len <= wbits; len++) {
            uint32_t code = win >> (32 - len);
            uint16_t lo   = h_first[len];
            uint8_t  cnt  = h_count[len];
            if (cnt && code >= lo && code < (uint32_t)(lo + cnt)) {
                char ch = (char)h_syms[h_start[len] + (code - lo)];
                win   <<= len;
                wbits  -= len;
                if (ch == '#') { dst[di] = '\0'; return di; }
                dst[di++] = ch;
                break;
            }
        }
    }
    dst[di] = '\0';
    return di;
}"""

def gen_header(phrases, prefix):
    lines = [
        f"/* {prefix}.h  —  auto-generated by huffman_compress.py, do not edit */",
        "#pragma once",
        "#include <stdint.h>",
        "",
        "/* Decode a Huffman-compressed phrase into dst (null-terminated).",
        "   src     : compressed byte stream (terminated in-band by '#')",
        "   dst     : output buffer",
        "   max_len : capacity of dst including null terminator",
        "   returns : characters written, excluding null */",
        "int phrase_decode(const uint8_t *src, char *dst, int max_len);",
        "",
        f"#define PHRASE_COUNT {len(phrases)}",
        "",
        "typedef struct {",
        "    const uint8_t *data;",
        "    uint8_t        stars : 3;  /* 0-5, fits in 3 bits */",
        "} Phrase;",
        "",
        "extern const Phrase phrases[PHRASE_COUNT];",
    ]
    return '\n'.join(lines)

def gen_source(phrases, codes, syms, lengths, prefix):
    count, first, start, max_len = make_tables(lengths, syms)
    n = len(syms)

    lines = [
        f"/* {prefix}.c  —  auto-generated by huffman_compress.py, do not edit */",
        f'#include "{prefix}.h"',
        "",
        f"#define HUFF_MAX {max_len}   /* maximum code length in bits */",
        f"#define HUFF_N   {n}         /* number of symbols */",
        "",
        "/* Canonical Huffman decode tables */",
        "/* h_syms : symbols in canonical order (sorted by length then value) */",
        "/* h_count: how many symbols have each code length (index = length)  */",
        "/* h_first: first canonical code at each length                      */",
        "/* h_start: index into h_syms where each length's symbols begin      */",
        "",
        "static const uint8_t  h_syms [HUFF_N]        = { " +
            ', '.join(str(ord(s)) for s in syms) + " };",
        "static const uint8_t  h_count[HUFF_MAX + 1]  = { " +
            ', '.join(str(count[i]) for i in range(max_len + 1)) + " };",
        "static const uint16_t h_first[HUFF_MAX + 1]  = { " +
            ', '.join(str(first[i]) for i in range(max_len + 1)) + " };",
        "static const uint8_t  h_start[HUFF_MAX + 1]  = { " +
            ', '.join(str(start[i]) for i in range(max_len + 1)) + " };",
        "",
        _DECODER,
        "",
    ]

    total_orig = total_comp = 0
    for i, (text, stars) in enumerate(phrases):
        data, pad = encode(text, codes)
        total_orig += len(text)
        total_comp += len(data)
        preview = text[2:-1].replace('|', ' ')[:36]
        hex_b = ', '.join(f'0x{b:02X}' for b in data)
        lines.append(
            f"static const uint8_t p{i:03d}[{len(data):3d}] = {{{hex_b}}};"
            f"  /* {len(data):3d}B ← {len(text):3d}ch  {preview!r} */"
        )

    lines += [
        "",
        "const Phrase phrases[PHRASE_COUNT] = {",
    ]
    for i, (text, stars) in enumerate(phrases):
        lines.append(f"    {{p{i:03d}, {stars}}},")
    lines.append("};")

    return '\n'.join(lines), total_orig, total_comp

# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print("Usage: huffman_compress.py <input.c> [output_prefix]", file=sys.stderr)
        sys.exit(1)

    with open(sys.argv[1]) as f:
        raw = f.read()

    prefix = sys.argv[2] if len(sys.argv) > 2 else "phrases_compressed"

    phrases = parse_phrases(raw)
    if not phrases:
        print("No phrases found — check input format.", file=sys.stderr)
        sys.exit(1)

    print(f"Phrases       : {len(phrases)}", file=sys.stderr)

    # Build shared frequency table
    freq = Counter()
    for text, _ in phrases:
        freq.update(text)
    print(f"Symbol set    : {len(freq)} chars", file=sys.stderr)

    # Build Huffman
    tree = _build_tree(freq)
    lengths = _get_lengths(tree)
    codes, syms = make_canonical(lengths)
    max_len = max(lengths.values())
    print(f"Max code len  : {max_len} bits", file=sys.stderr)

    # Verify round-trip
    errors = 0
    for text, _ in phrases:
        data, _ = encode(text, codes)
        got = decode_py(data, codes)
        if got != text:
            print(f"VERIFY FAIL: expected {text!r}, got {got!r}", file=sys.stderr)
            errors += 1
    if errors:
        print(f"{errors} verification failures — aborting.", file=sys.stderr)
        sys.exit(1)
    print(f"Verification  : all {len(phrases)} phrases OK", file=sys.stderr)

    # Generate C
    header = gen_header(phrases, prefix)
    source, total_orig, total_comp = gen_source(phrases, codes, syms, lengths, prefix)

    with open(f"{prefix}.h", 'w') as f: f.write(header + '\n')
    with open(f"{prefix}.c", 'w') as f: f.write(source + '\n')

    ratio = total_comp / total_orig * 100
    avg_bits = total_comp * 8 / sum(len(t) for t, _ in phrases)
    print(f"Original      : {total_orig} bytes", file=sys.stderr)
    print(f"Compressed    : {total_comp} bytes ({ratio:.1f}% of original)", file=sys.stderr)
    print(f"Avg bits/char : {avg_bits:.2f}", file=sys.stderr)
    print(f"Written       : {prefix}.h, {prefix}.c", file=sys.stderr)

if __name__ == "__main__":
    main()