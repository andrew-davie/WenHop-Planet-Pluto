#!/usr/bin/env python3
"""
Format a planet review into pixel-wrapped output.

Usage:
    python review_format.py
    (then paste or type the review when prompted)

Or pipe/pass as argument:
    python review_format.py "**Earth** — Tried to purchase the moon. ★★☆☆☆"
"""

import re
import sys

# ---------------------------------------------------------------------------
# Character widths for printable ASCII, starting at SPACE (ASCII 32).
# Replace the values below with your actual per-character pixel widths.
# Index 0 = space (32), index 1 = ! (33), index 2 = " (34), ...
# Must cover at least ASCII 32–126 (95 entries).
# ---------------------------------------------------------------------------
CHAR_WIDTHS = [
    # Pixel widths for ASCII 32–126 from fontcompact_charWidths[]
    # sp    !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /
      1,    2,    7,    7,    0,    2,    2,    1,    5,    4,    7,    7,    2,    4,    1,    2,
    # 0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
      3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    1,    2,    0,    0,    0,    4,
    # @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
      0,    4,    4,    4,    4,    4,    4,    4,    4,    3,    3,    4,    3,    5,    4,    4,
    # P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
      4,    4,    4,    4,    5,    4,    4,    5,    5,    4,    4,    3,    4,    3,    3,    4,
    # `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
      2,    3,    3,    4,    4,    4,    4,    3,    3,    1,    2,    3,    1,    5,    3,    3,
    # p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~
      3,    3,    2,    2,    2,    3,    3,    5,    3,    3,    3,    0,    0,    0,    0,
]

# ---------------------------------------------------------------------------


# Characters that act as formatting markers (zero width) unless escaped with backslash.
# Backslash itself is always zero width (it is the escape character).
# '}' is a paragraph-break marker: zero width, forces a line boundary in wrap_text.
FORMATTING_CHARS = {'_', '<', '>', '=', '~', '}'}


def char_width(ch):
    code = ord(ch)
    if 32 <= code <= 126:
        return CHAR_WIDTHS[code - 32] + 1  # +1 pixel inter-character spacing
    return 0  # non-ASCII or control chars get zero width


def word_pixel_width(word):
    """Width of a word in pixels, respecting backslash escapes and formatting chars.
    - Backslash: 0 width, causes next char to be treated as literal.
    - Unescaped formatting chars (e.g. _): 0 width.
    - Everything else: char_width() as normal.
    """
    total = 0
    i = 0
    while i < len(word):
        ch = word[i]
        if ch == '\\' and i + 1 < len(word):
            # Escaped: backslash=0, next char uses raw width regardless of formatting
            total += char_width(word[i + 1])
            i += 2
        elif ch in FORMATTING_CHARS:
            i += 1  # formatting marker: zero width
        else:
            total += char_width(ch)
            i += 1
    return total


def parse_review(raw):
    """Strip 'PlanetName —' prefix and '★☆☆☆☆' suffix.
    Returns (review_text, filled_star_count)."""
    text = raw.strip()

    # Count filled stars before stripping
    filled = text.count('★')

    # Remove trailing star block  e.g. ★★☆☆☆
    text = re.sub(r'\s*[★☆]+\s*$', '', text).strip()

    # Remove leading planet name (optional **bold**) and em-dash
    # Handles: **Earth** — ...  or  Earth — ...
    text = re.sub(r'^\*{0,2}[^—]+\*{0,2}\s*—\s*', '', text).strip()

    return text, filled


def _split_paragraphs(text):
    """Split text on unescaped '}', returning list of (segment, has_break) pairs.
    Each segment with has_break=True was followed by a '}' in the source."""
    segments = []
    current = []
    i = 0
    while i < len(text):
        if text[i] == '\\' and i + 1 < len(text):
            current.append(text[i:i + 2])
            i += 2
        elif text[i] == '}':
            segments.append((''.join(current), True))
            current = []
            i += 1
        else:
            current.append(text[i])
            i += 1
    segments.append((''.join(current), False))
    return segments


def _wrap_segment(text, max_px):
    """Wrap a single paragraph segment into '|'-joined lines."""
    words = text.split()
    if not words:
        return ''
    space_w = char_width(' ')
    lines = []
    current_words = []
    current_width = 0
    for word in words:
        w = word_pixel_width(word)
        if not current_words:
            current_words = [word]
            current_width = w
        else:
            new_width = current_width + space_w + w
            if new_width > max_px:
                lines.append(' '.join(current_words))
                current_words = [word]
                current_width = w
            else:
                current_words.append(word)
                current_width = new_width
    if current_words:
        lines.append(' '.join(current_words))
    return '|'.join(lines)


def wrap_text(text, max_px=48):
    """Word-wrap text so each line fits within max_px pixels.
    Lines are joined with '|'.
    Unescaped '}' is a paragraph-break marker: forces a line boundary and is
    appended (zero-width) to the last line of each paragraph so the renderer
    can add inter-paragraph spacing.  '\\}' passes '}' through as a literal."""
    segments = _split_paragraphs(text)
    all_lines = []
    for seg_text, has_break in segments:
        wrapped = _wrap_segment(seg_text.strip(), max_px)
        if wrapped:
            if has_break:
                # Attach '}' to the last line of this paragraph segment
                lines = wrapped.split('|')
                lines[-1] += '}'
                all_lines.append('|'.join(lines))
            else:
                all_lines.append(wrapped)
        elif has_break and all_lines:
            # '}' with no preceding text: attach to whatever came before
            all_lines[-1] += '}'
    if not all_lines:
        return ''
    # Join segments: if the previous segment ends with '}', no '|' is needed
    # because '}' already acts as the line boundary.
    result = all_lines[0]
    for seg in all_lines[1:]:
        result += seg if result.endswith('}') else '|' + seg
    return result


MAX_WORD_PX = 47


def long_words(text):
    """Return list of (word, width) for words exceeding MAX_WORD_PX."""
    seen = set()
    result = []
    for w in text.split():
        bare = w.strip('.,!?;:\'"')
        if bare not in seen and word_pixel_width(bare) > MAX_WORD_PX:
            seen.add(bare)
            result.append((bare, word_pixel_width(bare)))
    return result


def _rewrap_adaptive(text):
    """Wrap text applying fallback rules when the last line is too wide for the
    closing '#' quote (which costs 16px of the 48px budget, leaving 32px).

    Fallback order (per user spec):
      1. Standard: "" prefix + ## suffix  → first/last line ≤ 32px
      2. Drop full-stop: strip trailing '.' from last word, retry standard
      3. Drop quotes on last line: "" prefix only (no ## suffix) → last ≤ 48px
      4. Drop both: strip '.' AND no ## suffix

    Returns (pipe_text, note) where note describes any fallback applied.
    """
    def _do_wrap(t, drop_stop, drop_quotes):
        # Build the text to wrap
        flat = t.rstrip()
        if drop_stop and flat.endswith('.'):
            flat = flat[:-1]
        suffix = '' if drop_quotes else '##'
        w = wrap_text('""' + flat + suffix)
        ls = w.split('|')
        ls[0] = ls[0][2:]           # strip ""
        if not drop_quotes:
            ls[-1] = ls[-1][:-2]    # strip ##
        return '|'.join(ls)

    H_W = char_width('#')
    LIMIT = 48
    LAST_BUDGET = LIMIT - 2 * H_W  # 32px with quotes

    def last_line_w(pipe):
        lns = pipe.split('|')
        last_seg = lns[-1]
        last_line = last_seg.rsplit('}', 1)[1] if '}' in last_seg else last_seg
        return word_pixel_width(last_line)

    # Step 1: standard
    result = _do_wrap(text, drop_stop=False, drop_quotes=False)
    if last_line_w(result) <= LAST_BUDGET:
        return result, ''

    # Step 2: drop full-stop
    result = _do_wrap(text, drop_stop=True, drop_quotes=False)
    if last_line_w(result) <= LAST_BUDGET:
        return result, 'dropped full-stop'

    # Step 3: drop quotes (full-stop restored)
    result = _do_wrap(text, drop_stop=False, drop_quotes=True)
    if last_line_w(result) <= LIMIT:
        return result, 'dropped closing quote'

    # Step 4: drop both
    result = _do_wrap(text, drop_stop=True, drop_quotes=True)
    return result, 'dropped full-stop + closing quote'


def format_review(raw):
    text, filled = parse_review(raw)

    offenders = long_words(text)
    if offenders:
        words_str = ', '.join(f'"{w}" ({px}px)' for w, px in offenders)
        print(f"[discarded — word(s) too wide: {words_str}]", file=sys.stderr)
        return None

    wrapped, note = _rewrap_adaptive(text)
    if note:
        print(f"[note: {note}]", file=sys.stderr)

    n_lines = wrapped.count('|') + wrapped.count('}') + 1
    if n_lines > 11:
        print(f"[discarded — too many lines ({n_lines})]", file=sys.stderr)
        return None

    return f'{filled},"=\\"{wrapped}#",'


def _check_c_file(path):
    """Parse a planetInfo.c-style file and validate every active string.
    Reports first-line, last-line, and line-count violations."""
    Q_W = char_width('"')
    H_W = char_width('#')
    LIMIT   = 48
    MAX_C   = LIMIT - 2 * Q_W   # 32px — max content for first/last line

    # Stack-based preprocessor: True = this level's block is active.
    # #if 1 → push True; #if 0 → push False; #else → toggle top; #endif → pop.
    stack = []
    def active(): return all(stack) if stack else True

    total = ok = 0
    fails = []

    with open(path) as f:
        for lineno, line in enumerate(f, 1):
            s = line.strip()
            if re.match(r'#\s*if\s+0\b', s):   stack.append(False); continue
            if re.match(r'#\s*if\s+1\b', s):   stack.append(True);  continue
            if re.match(r'#\s*else\b',   s):
                if stack: stack[-1] = not stack[-1]
                continue
            if re.match(r'#\s*endif\b',  s):
                if stack: stack.pop()
                continue
            if not active(): continue

            m = re.match(r'\{"=\\"(.+?)#",(\d+)\}', s)
            if not m: continue
            total += 1
            pipe_text = m.group(1)
            lns = pipe_text.split('|')

            # } within a pipe segment creates an additional visual line break.
            # Count total rendered lines: pipe segments + unescaped } in entire text.
            n_pipe = len(lns)
            n_para = sum(seg.count('}') for seg in lns)
            n = n_pipe + n_para

            # First rendered line: content before first } in lns[0] (or all of lns[0])
            first_seg = lns[0]
            first_line = first_seg.split('}', 1)[0] if '}' in first_seg else first_seg
            f_w = word_pixel_width(first_line)

            # Last rendered line: content after last } in lns[-1] (or all of lns[-1])
            last_seg = lns[-1]
            last_line = last_seg.rsplit('}', 1)[1] if '}' in last_seg else last_seg
            l_content = word_pixel_width(last_line)
            l_w = l_content + 2 * H_W

            errors = []
            if f_w > MAX_C:
                errors.append(f'first-line {f_w}px > {MAX_C}px')
            # l_w > LIMIT: last line + quotes overflow. Accept if content alone fits
            # (closing quote dropped per fallback rule).
            if l_w > LIMIT:
                if l_content > LIMIT:
                    errors.append(f'last-line {l_content}px > {LIMIT}px (even without quotes)')
            if n > 11:
                errors.append(f'{n} lines > 11')

            if errors:
                fails.append((lineno, ', '.join(errors), pipe_text[:60]))
            else:
                ok += 1

    print(f"Checked {total} entries: {ok} OK, {len(fails)} failed.")
    for lineno, err, preview in fails:
        print(f"  L{lineno}: {err}")
        print(f"    {preview}")


def main():
    if len(sys.argv) > 1 and sys.argv[1].endswith('.c'):
        _check_c_file(sys.argv[1])
    elif len(sys.argv) > 1:
        raw = ' '.join(sys.argv[1:])
        result = format_review(raw)
        if result is not None:
            print(result)
    else:
        print("Paste review (then press Enter):")
        raw = input()
        result = format_review(raw)
        if result is not None:
            print(result)


if __name__ == '__main__':
    main()
