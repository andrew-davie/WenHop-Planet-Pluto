from PIL import Image
import sys

def image_to_c_1bpp_x_y(filename):

    import re
#    parts = re.split(r"[/.]", filename)
    varname = result = re.sub(r'[^0-9A-Za-z]', '_', filename)

    # Load and convert to 1-bit image
    img = Image.open(filename).convert("1")
    width, height = img.size
    pixels = img.load()

    # Pad width to multiple of 8
    padded_width = (width + 7) & ~7
    width_bytes = padded_width // 8

    # Build data as [y][x]
    data = [[0 for _ in range(width_bytes)] for _ in range(height)]

    for y in range(height):
        for xblock in range(0, padded_width, 8):
            byte = 0
            for bit in range(8):
                x = xblock + bit
                if x < width:
                    pixel = pixels[x, y]
                    if pixel != 0:  # non-black → 1
                        byte |= (1 << (7-bit))
            data[y][xblock // 8] = byte

    # Generate C code

    _id  = f"const unsigned char {varname}[{height}][{width_bytes}]"
    c_header = f'extern {_id};\n'

    c_code = f"{_id} = {{\n"

    for x, col in enumerate(data):
        hex_bytes = ", ".join(f"0x{b:02X}" for b in col)
        c_code += f"  {{ {hex_bytes} }}, // column {x}\n"
    c_code += "};\n"

    c_header += f"#define {varname}_HEIGHT {height}"

    return c_header, c_code

# Example usage
if __name__ == "__main__":

    import argparse
    import glob

    parser = argparse.ArgumentParser(description="Create 6-wide sprite matrix from image")
    parser.add_argument('-o', '--output', type=str, required=True, help='Output file')
    parser.add_argument('files', nargs='+', help='Input filenames or wildcards')

    args = parser.parse_args()

    # Resolve files from patterns
    files = []
    for pattern in args.files:
        matched = glob.glob(pattern)
        if not matched:
            print(f"Warning: pattern '{pattern}' did not match any files")
        files.extend(matched)

    filename = sys.argv[1]

    with open(args.output+'.c', "w") as output_c, open(args.output+'.h', "w") as output_h:

        print('#pragma once\n', file=output_h)

        for f in files:
            header, code = image_to_c_1bpp_x_y(f)
            print(header, file=output_h)
            print(code, file=output_c)
