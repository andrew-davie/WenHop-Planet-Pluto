from PIL import Image
import sys


def build(pix, im, y, start, stop, step):

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


#print(im.size)




def chrono_colour(file_name):

    import re
    varname = result = re.sub(r'[^0-9A-Za-z]', '_', file_name)

    # Load and convert to chrono colour
    im = Image.open(file_name) #.convert("256")
    width, height = im.size
    pix = im.load()

    if width != 32:
        raise ValueError(f"Image '{file_name}' must be 32 pixels wide, got {width}")



    Red_Lines = []
    Green_Lines = []
    Blue_Lines = []
    mask_Lines = []

    for y in range(0, height):

        chary = y

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

        Red_Lines.append(R)
        Green_Lines.append(G)
        Blue_Lines.append(B)

        mask_Lines.append(mask)


    # Generate C code

    _id  = f"const unsigned char {varname}[{height}][4][3]"
    c_header = f'extern {_id};\n'

#    c_code = f'#include "{varname}.h\n\n'
    c_code = f"{_id} = {{\n"

    # const struct image[lines][4][3] = {
    #         COL 0    COL 1    COL 2    COL 3
    #     { { R,G,B}, {R,G,B}, {R,G,B}, {R,G,B} }, // line 0
    #     { { R,G,B}, {R,G,B}, {R,G,B}, {R,G,B} }, // line 1
    # etc..
    # };

    for line in range(height):          # ICC lines (~66)
        c_code += "{ "
        for col in range(4):
            c_code += f"{{0x{Red_Lines[line][col]:02X}, "
            c_code += f"0x{Green_Lines[line][col]:02X}, "
            c_code += f"0x{Blue_Lines[line][col]:02X}}}"
            if col < 3:
                c_code += ", "
        c_code += f" }},   // {line}\n"
    c_code += "\n};\n\n"

    c_header += f"#define {varname}_HEIGHT {height}"

    return c_header, c_code


if __name__ == "__main__":

    import argparse
    import glob

    parser = argparse.ArgumentParser(description="Convert image to ChronoColour C-array")
    parser.add_argument('-o', '--output', type=str, required=True, help='Output file (no extension)')
    parser.add_argument('files', nargs='+', help='Input filenames or wildcards')

    args = parser.parse_args()

    # Resolve files from patterns
    files = []
    for pattern in args.files:
        matched = glob.glob(pattern)
        if not matched:
            print(f"Warning: pattern '{pattern}' did not match any files")
        files.extend(matched)

    import os
    dir = os.path.dirname(args.output)
    if dir:
        os.makedirs(dir, exist_ok=True)

    with open(args.output+'.c', "w") as output_c, open(args.output+'.h', "w") as output_h:

        print('#pragma once\n', file=output_h)

        for f in files:

            try:
                header, code = chrono_colour(f)
                print(header, file=output_h)
                print(code, file=output_c)
            except ValueError as e:
                print(e)  # or log, or re-raise, whatever

