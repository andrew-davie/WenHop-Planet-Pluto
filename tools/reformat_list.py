import re, sys

CHAR_WIDTHS = [
    1, 2, 4, 4, 0, 2, 2, 1, 5, 4, 7, 7, 2, 4, 1, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 2, 0, 0, 0, 4,
    0, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 3, 5, 4, 4,
    4, 4, 4, 4, 5, 4, 4, 5, 5, 4, 4, 3, 4, 3, 3, 4,
    2, 3, 3, 4, 4, 4, 4, 3, 3, 1, 2, 3, 1, 5, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 5, 3, 3, 3, 0, 0, 0, 0,
]


def char_width(ch):
    code = ord(ch)
    if 32 <= code <= 126:
        return CHAR_WIDTHS[code - 32] + 1
    return 0


def word_pixel_width(word):
    return sum(char_width(c) for c in word)


def wrap_text(text, max_px=48):
    words = text.split()
    if not words:
        return ''
    space_w = char_width(' ')
    lines, cur_words, cur_w = [], [], 0
    for word in words:
        w = word_pixel_width(word)
        if not cur_words:
            cur_words, cur_w = [word], w
        else:
            new_w = cur_w + space_w + w
            if new_w > max_px:
                lines.append(' '.join(cur_words))
                cur_words, cur_w = [word], w
            else:
                cur_words.append(word)
                cur_w = new_w
    if cur_words:
        lines.append(' '.join(cur_words))
    return '|'.join(lines)


data = open('/sessions/trusting-gallant-archimedes/mnt/outputs/reviews_raw.txt').read()
out = []

for line in data.splitlines():
    stripped = line.strip()
    if not stripped:
        continue
    if stripped == '////':
        out.append('////')
        continue

    commented = stripped.startswith('//')
    content = stripped[2:] if commented else stripped
    prefix = '//' if commented else ''

    m = re.match(r'^(\d+),"=\\"(.+?)#",$', content)
    if not m:
        out.append(stripped)
        continue

    stars = m.group(1)
    text_piped = m.group(2)
    raw = text_piped.replace('|', ' ')
    wrapped = wrap_text(raw)
    n = wrapped.count('|') + 1
    flag = f'  // {n} lines' if n > 11 else ''
    out.append(f'{prefix}{stars},"=\\"{wrapped}#",{flag}')

print('\n'.join(out))
