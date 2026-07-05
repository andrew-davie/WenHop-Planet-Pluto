#!/usr/bin/env python3
"""
huffman_compress.py — Shared Huffman compression for pixel-display phrase strings.
Supports word-level tokenisation with automatic dictionary-size optimisation.

Each phrase is compressed independently using a shared codebook built from all
phrases together. The C decoder is self-contained and needs no external tables.

Input format (one or more lines):
    {"=\"TEXT#",STARS},

Usage:
    python3 huffman_compress.py <input.c> [output_prefix] [--words N]

    --words N   Fix dictionary size to N words (0 = chars only).
                Omit to auto-search for the best N via ternary search.

Outputs:
    <prefix>.h   — decoder prototype + Phrase struct
    <prefix>.c   — Huffman tables, compressed arrays, decoder, phrases[]
"""

import sys, re, heapq
from collections import Counter

# ─── Parse ────────────────────────────────────────────────────────────────────

def _strip_c_comments(src):
    """Remove C-style /* block */ and // line comments."""
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.DOTALL)
    src = re.sub(r'//.*$',    '', src, flags=re.MULTILINE)
    return src

def _preprocess(src):
    """Strip lines inside #if 0 blocks; pass #if 1 blocks through.
    Handles nesting via a stack. #else toggles the top condition.
    Preprocessor directive lines themselves are always removed.
    """
    stack  = []
    active = lambda: all(stack) if stack else True
    lines  = []
    for line in src.splitlines():
        s = line.strip()
        if   re.match(r'#\s*if\s+0\b', s):   stack.append(False)
        elif re.match(r'#\s*if\s+1\b', s):   stack.append(True)
        elif re.match(r'#\s*else\b',   s):
            if stack: stack[-1] = not stack[-1]
        elif re.match(r'#\s*endif\b',  s):
            if stack: stack.pop()
        else:
            if active():
                lines.append(line)
    return '\n'.join(lines)

def parse_phrases(src):
    """Return list of (full_string, stars) from C initializer lines.
    full_string is the actual display string: ="TEXT#
    Respects #if 0 / #if 1 / #else / #endif preprocessor guards.
    Ignores C-style block comments and // line comments.
    """
    src = _strip_c_comments(src)
    src = _preprocess(src)
    out = []
    for m in re.finditer(r'\{"=\\"(.*?)#",(\d+)\}', src):
        full  = '="' + m.group(1) + '#'
        stars = int(m.group(2))
        if stars > 5:
            print(f"WARNING: stars={stars} clamped to 5 for: {full[:40]!r}", file=sys.stderr)
            stars = 5
        out.append((full, stars))
    return out

# ─── BPE Tokenisation ─────────────────────────────────────────────────────────

# Characters that must never be merged into a larger BPE token:
#   '#' — decoder terminator sentinel
#   '}' — paragraph-break control character (zero-width, non-printable)
_BPE_PROTECTED = frozenset('#}')

def _bpe_merge_allowed(a, b):
    """Return True if the (a, b) pair may be merged by BPE."""
    return not any(ch in tok for ch in _BPE_PROTECTED for tok in (a, b))

def _bpe_step(tokenized, a, b):
    """Apply one BPE merge: replace every adjacent (a, b) pair with a+b."""
    merged = a + b
    result = []
    for tokens in tokenized:
        out, i = [], 0
        while i < len(tokens):
            if i < len(tokens) - 1 and tokens[i] == a and tokens[i+1] == b:
                out.append(merged)
                i += 2
            else:
                out.append(tokens[i])
                i += 1
        result.append(out)
    return result

def _word_dict_from(tokenized):
    """Extract multi-char tokens from a tokenized corpus, sorted by frequency."""
    freq = Counter(tok for ts in tokenized for tok in ts if len(tok) > 1)
    return [tok for tok, _ in freq.most_common()]

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
    """Return (codes, sorted_syms). Handles single-char and word tokens.
    Sort order: by code length, then single chars before words, then lexicographic.
    """
    def sort_key(s):
        return (lengths[s], len(s) > 1, s)
    syms = sorted(lengths, key=sort_key)
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

# ─── Encode / Decode ──────────────────────────────────────────────────────────

def encode(tokens, codes):
    """Return (bytes, num_padding_bits). tokens is a list of char or word strings."""
    bits = []
    for tok in tokens:
        c, l = codes[tok]
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
    """Decode bytes back to string (Python, for verification).
    Works with both char and word tokens — just concatenates whatever tokens decode to.
    """
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
                tok = rev[(c, l)]
                pos += l
                if tok == '#':
                    return ''.join(result) + '#'
                result.append(tok)   # tok is 1-char string or word string
                break
    return ''.join(result)

# ─── C generation ─────────────────────────────────────────────────────────────

# h_syms encoding:
#   value  0-127 : literal char (output as-is)
#   value  128+  : word token; h_words[value - 128] is the string to output
# This works because all display chars are ASCII < 128 (including '#' = 35).

_DECODER_CHARS_ONLY = r"""int phrase_decode(const uint8_t *src, char *dst, int max_len) {
    uint32_t win   = 0;
    int      wbits = 0, si = 0, di = 0;

    while (di < max_len - 1) {
        /* Refill bit window from src (MSB first). */
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
                if (ch == '#') { dst[di++] = '#'; dst[di] = '\0'; return di; }
                dst[di++] = ch;
                break;
            }
        }
    }
    dst[di] = '\0';
    return di;
}"""

_DECODER_WITH_WORDS = r"""int phrase_decode(const uint8_t *src, char *dst, int max_len) {
    uint32_t win   = 0;
    int      wbits = 0, si = 0, di = 0;

    while (di < max_len - 1) {
        /* Refill bit window from src (MSB first). */
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
                uint8_t sym = h_syms[h_start[len] + (code - lo)];
                win   <<= len;
                wbits  -= len;
                if (sym == '#') { dst[di++] = '#'; dst[di] = '\0'; return di; }
                if (sym >= 128) {
                    /* Word token: copy string from dictionary. */
                    const char *w = h_words[sym - 128];
                    while (*w && di < max_len - 1) dst[di++] = *w++;
                } else {
                    dst[di++] = (char)sym;
                }
                break;
            }
        }
    }
    dst[di] = '\0';
    return di;
}"""

def gen_header(phrases, prefix):
    max_decoded = max(len(text) for text, _ in phrases)
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
        f"#define PHRASE_COUNT     {len(phrases)}",
        f"#define PHRASE_BUF_SIZE  {max_decoded + 1}  /* longest decoded phrase + null */",
        "",
        "typedef struct {",
        "    const uint8_t *data;",
        "    uint8_t        stars : 3;  /* 0-5, fits in 3 bits */",
        "} Phrase;",
        "",
        "extern const Phrase phrases[PHRASE_COUNT];",
    ]
    return '\n'.join(lines)

def gen_source(phrases, all_tokens, codes, syms, lengths, word_dict, prefix):
    count, first, start, max_len = make_tables(lengths, syms)
    n = len(syms)
    use_words = bool(word_dict)

    # word_dict is in ranking order — index in this list = h_words[] index.
    word_index = {w: i for i, w in enumerate(word_dict)}

    def sym_byte(s):
        """Map a token to its uint8 value for h_syms."""
        return ord(s) if len(s) == 1 else (128 + word_index[s])

    lines = [
        f"/* {prefix}.c  —  auto-generated by huffman_compress.py, do not edit */",
        f'#include "{prefix}.h"',
        "",
        f"#define HUFF_MAX {max_len}   /* maximum code length in bits */",
        f"#define HUFF_N   {n}         /* number of symbols */",
        "",
        "/* Canonical Huffman decode tables                                           */",
        "/* h_syms : tokens in canonical order                                        */",
        "/*          value < 128 = literal char; >= 128 = word index + 128            */",
        "/* h_count: symbols at each code length (index = length)                     */",
        "/* h_first: first canonical code at each length                              */",
        "/* h_start: index into h_syms where each length's symbols begin              */",
        "",
    ]

    if use_words:
        def _c_str(s):
            return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'
        word_strs = ', '.join(_c_str(w) for w in word_dict)
        lines += [
            f"/* Word dictionary ({len(word_dict)} entries); h_syms value 128+i -> h_words[i] */",
            f"static const char * const h_words[{len(word_dict)}] = {{ {word_strs} }};",
            "",
        ]

    lines += [
        "static const uint8_t  h_syms [HUFF_N]        = { " +
            ', '.join(str(sym_byte(s)) for s in syms) + " };",
        "static const uint8_t  h_count[HUFF_MAX + 1]  = { " +
            ', '.join(str(count[i]) for i in range(max_len + 1)) + " };",
        "static const uint16_t h_first[HUFF_MAX + 1]  = { " +
            ', '.join(str(first[i]) for i in range(max_len + 1)) + " };",
        "static const uint8_t  h_start[HUFF_MAX + 1]  = { " +
            ', '.join(str(start[i]) for i in range(max_len + 1)) + " };",
        "",
        _DECODER_WITH_WORDS if use_words else _DECODER_CHARS_ONLY,
        "",
    ]

    total_orig = total_comp = 0
    for i, ((text, stars), tokens) in enumerate(zip(phrases, all_tokens)):
        data, pad = encode(tokens, codes)
        total_orig += len(text)
        total_comp += len(data)
        preview = text[2:-1].replace('|', ' ')[:36]
        hex_b = ', '.join(f'0x{b:02X}' for b in data)
        lines.append(
            f"static const uint8_t p{i:03d}[{len(data):3d}] = {{{hex_b}}};"
            f"  /* {len(data):3d}B <- {len(text):3d}ch  {preview!r} */"
        )

    lines += [
        "",
        "const Phrase phrases[PHRASE_COUNT] = {",
    ]
    for i, (text, stars) in enumerate(phrases):
        lines.append(f"    {{p{i:03d}, {stars}}},")
    lines.append("};")

    return '\n'.join(lines), total_orig, total_comp

# ─── Compression helpers ──────────────────────────────────────────────────────

def _word_table_bytes(word_dict, ptr_size=4):
    """ROM cost of the h_words[] string table (strings + null terminators + pointers)."""
    return sum(len(w) + 1 for w in word_dict) + len(word_dict) * ptr_size

def _huffman_size(all_tokens):
    """Total compressed bytes for a tokenized corpus."""
    freq = Counter(tok for ts in all_tokens for tok in ts)
    tree = _build_tree(freq)
    lengths = _get_lengths(tree)
    codes, _ = make_canonical(lengths)
    return sum(len(encode(ts, codes)[0]) for ts in all_tokens)

def _huffman_state(all_tokens):
    """Full Huffman state for a tokenized corpus: (codes, syms, lengths)."""
    freq = Counter(tok for ts in all_tokens for tok in ts)
    tree = _build_tree(freq)
    lengths = _get_lengths(tree)
    codes, syms = make_canonical(lengths)
    return codes, syms, lengths

# ─── BPE search ───────────────────────────────────────────────────────────────

def bpe_search(phrases, baseline, max_merges=100):
    """Incrementally apply BPE merges, evaluating net saving at each step.
    Returns (best_n, best_saving, best_tokenized, best_word_dict).

    '#' and '}' are excluded from merges so control characters stay atomic.
    O(max_merges x corpus_size) — single forward pass, no re-runs.
    """
    tokenized = [list(text) for text, _ in phrases]

    # Evaluate n=0 (chars only)
    s0 = baseline - _huffman_size(tokenized)
    print(f"  merges={0:3d}: net saving {s0:+d} B", file=sys.stderr)
    best_n, best_saving = 0, s0
    best_tok, best_wdict = [t[:] for t in tokenized], []

    for n in range(1, max_merges + 1):
        # Count adjacent pairs, excluding any that touch protected control chars
        pair_freq = Counter()
        for tokens in tokenized:
            for a, b in zip(tokens, tokens[1:]):
                if _bpe_merge_allowed(a, b):
                    pair_freq[(a, b)] += 1
        if not pair_freq:
            break

        a, b = max(pair_freq, key=pair_freq.get)
        tokenized = _bpe_step(tokenized, a, b)

        word_dict = _word_dict_from(tokenized)
        comp      = _huffman_size(tokenized)
        overhead  = _word_table_bytes(word_dict)
        saving    = baseline - comp - overhead

        print(f"  merges={n:3d}: net saving {saving:+d} B  [{a!r}+{b!r}]", file=sys.stderr)

        if saving > best_saving:
            best_n, best_saving = n, saving
            best_tok  = [t[:] for t in tokenized]
            best_wdict = word_dict[:]

    return best_n, best_saving, best_tok, best_wdict

# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    args = sys.argv[1:]
    if not args:
        print("Usage: huffman_compress.py <input.c> [output_prefix] [--words N]", file=sys.stderr)
        sys.exit(1)

    top_n     = None   # None = auto-search
    positional = []
    i = 0
    while i < len(args):
        if args[i] == '--words' and i + 1 < len(args):
            i += 1
            top_n = int(args[i])
        else:
            positional.append(args[i])
        i += 1

    input_file = positional[0]
    prefix     = positional[1] if len(positional) > 1 else "phrases_compressed"

    with open(input_file) as f:
        raw = f.read()

    phrases = parse_phrases(raw)
    if not phrases:
        print("No phrases found — check input format.", file=sys.stderr)
        sys.exit(1)

    total_chars = sum(len(text) for text, _ in phrases)
    print(f"Phrases       : {len(phrases)}", file=sys.stderr)

    # ── Baseline: char-only Huffman ──
    char_tokens    = [list(text) for text, _ in phrases]
    baseline_bytes = _huffman_size(char_tokens)
    print(f"Baseline      : {baseline_bytes} bytes ({baseline_bytes/total_chars*100:.1f}%) "
          f"— chars-only Huffman", file=sys.stderr)

    # ── BPE search or fixed merge count ──
    if top_n is None:
        print(f"BPE search (0–100 merges)...", file=sys.stderr)
        best_n, best_saving, all_tokens, word_dict = bpe_search(phrases, baseline_bytes)
        print(f"Best          : {best_n} merges → net saving {best_saving:+d} B", file=sys.stderr)
    elif top_n == 0:
        print(f"BPE merges    : 0 (chars only)", file=sys.stderr)
        all_tokens, word_dict = char_tokens, []
    else:
        print(f"BPE merges    : {top_n} (fixed)", file=sys.stderr)
        tokenized = char_tokens
        for _ in range(top_n):
            pair_freq = Counter()
            for tokens in tokenized:
                for a, b in zip(tokens, tokens[1:]):
                    if _bpe_merge_allowed(a, b):
                        pair_freq[(a, b)] += 1
            if not pair_freq: break
            a, b = max(pair_freq, key=pair_freq.get)
            tokenized = _bpe_step(tokenized, a, b)
        all_tokens = tokenized
        word_dict  = _word_dict_from(all_tokens)

    # ── Build final Huffman state ──
    codes, syms, lengths = _huffman_state(all_tokens)

    total_tokens = sum(len(t) for t in all_tokens)
    n_unique     = len(set(tok for ts in all_tokens for tok in ts))
    print(f"Token set     : {n_unique} unique  ({total_tokens} total vs {total_chars} chars)", file=sys.stderr)
    if word_dict:
        print(f"BPE tokens    : {len(word_dict)}  e.g. {word_dict[:8]}", file=sys.stderr)
    print(f"Max code len  : {max(lengths.values())} bits", file=sys.stderr)

    # ── Verify round-trip ──
    errors = 0
    for (text, _), tokens in zip(phrases, all_tokens):
        data, _ = encode(tokens, codes)
        got = decode_py(data, codes)
        if got != text:
            print(f"VERIFY FAIL: expected {text!r}", file=sys.stderr)
            print(f"             got      {got!r}", file=sys.stderr)
            errors += 1
    if errors:
        print(f"{errors} verification failures — aborting.", file=sys.stderr)
        sys.exit(1)
    print(f"Verification  : all {len(phrases)} phrases OK", file=sys.stderr)

    # ── Generate C ──
    header = gen_header(phrases, prefix)
    source, total_orig, total_comp = gen_source(
        phrases, all_tokens, codes, syms, lengths, word_dict, prefix
    )

    with open(f"{prefix}.h", 'w') as f: f.write(header + '\n')
    with open(f"{prefix}.c", 'w') as f: f.write(source + '\n')

    overhead = _word_table_bytes(word_dict)
    ratio    = total_comp / total_orig * 100
    avg_bits = total_comp * 8 / total_orig
    print(f"Original      : {total_orig} bytes", file=sys.stderr)
    print(f"Compressed    : {total_comp} bytes ({ratio:.1f}%)", file=sys.stderr)
    if word_dict:
        print(f"Token table   : {overhead} bytes overhead", file=sys.stderr)
        net = baseline_bytes - total_comp - overhead
        print(f"Net saving    : {net:+d} bytes vs baseline", file=sys.stderr)
    print(f"Avg bits/char : {avg_bits:.2f}", file=sys.stderr)
    print(f"Written       : {prefix}.h, {prefix}.c", file=sys.stderr)

if __name__ == "__main__":
    main()