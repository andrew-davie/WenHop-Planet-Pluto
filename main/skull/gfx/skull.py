import os
from typing import Any

from PIL import Image
import sys


def build(pix: Any, im, y, start, stop, step):

    r = 0
    b = 0
    g = 0

    for cx in range(start, stop, step):

        p = pix[cx, y]
        r = (r << 1) | (p & 1)
        g = (g << 1) | ((p & 2) >> 1)
        b = (b << 1) | ((p & 4) >> 2)

    if start-stop == 4:
        r <<= 4
        g <<= 4
        b <<= 4

    return r, g, b


def icc_convert(source: str):

    im = Image.open(source)
    pix = im.load()

    base_name: str = os.path.basename(source)

    print(f"base = {base_name}\n");

    ident = base_name.split('.')[0]
    output_name = './' + ident + '.c'

    f = open(output_name, 'w')

    red_lines = []
    green_lines = []
    blue_lines = []
    mask_lines = []

    for y in range(0, im.size[1]):

        chary = im.size[1] - 1 - y

        R = []
        G = []
        B = []
        mask = []

        (r, g, b) = build(pix, im, chary, 0, 8, 1)
        R.append(r)
        B.append(b)
        G.append(g)

        (r, g, b) = build(pix, im, chary, 15, 7, -1)
        R.append(r)
        B.append(b)
        G.append(g)

        (r, g, b) = build(pix, im, chary, 16, 24, 1)
        R.append(r)
        B.append(b)
        G.append(g)

        (r, g, b) = build(pix, im, chary, 31, 23, -1)
        R.append(r)
        B.append(b)
        G.append(g)

        red_lines.append(R)
        green_lines.append(G)
        blue_lines.append(B)
        mask_lines.append(mask)

    f.write(f'const unsigned char {ident}[] = ' + '{\n')
    for byte_pos in [0,1,3,2]:
        f.write(f'// column {byte_pos}\n')
        for line in range(im.size[1]-1, -1, -1):
            f.write('    0b' + format(red_lines[line][byte_pos], '08b') +
                    ', 0b' + format(green_lines[line][byte_pos], '08b') +
                    ', 0b' + format(blue_lines[line][byte_pos], '08b') +
                    f',\n')
    f.write('};\n')

    f.close()


if __name__ == '__main__':

    icc_convert('skull.gif')
    icc_convert('jaw.gif')
    icc_convert('bone1.gif')
    icc_convert('bone2.gif')
