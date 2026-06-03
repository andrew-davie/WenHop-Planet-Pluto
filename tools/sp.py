#!/usr/bin/env python3
# sprite sheet processor
import re
import argparse
import os
import sys
from collections import deque
from PIL import Image
import numpy as np
from pathlib import Path


def make_palette(R, G, B):
    palette = []
    for i in range(8):
        r_bit = (i >> 2) & 1  # Bit 2 = R
        g_bit = (i >> 1) & 1  # Bit 1 = G
        b_bit = i & 1         # Bit 0 = B
        c = np.clip(R*r_bit + G*g_bit + B*b_bit, 0, 255)
        palette.append(c.astype(np.uint8))
    return np.array(palette)  # shape (8,3)


def pixmap(image, debug=False):
    """
    Convert an RGB image to a 2D array of 3-bit palette indices.

    Each pixel is mapped to one of 8 colours by thresholding each channel at 128.
    The resulting index encodes which colour planes are active:
      bit 2 (value 4) = red plane
      bit 1 (value 2) = green plane
      bit 0 (value 1) = blue plane

    So index 0=black, 1=blue, 2=green, 3=cyan, 4=red, 5=magenta, 6=yellow, 7=white.

    This is intentionally a hard threshold, not nearest-neighbour: source sprites
    are assumed to already be clean 8-colour images.
    """
    R = np.array([0xFF, 0x00, 0x00], dtype=np.int16)
    G = np.array([0x00, 0xFF, 0x00], dtype=np.int16)
    B = np.array([0x00, 0x00, 0xFF], dtype=np.int16)

    img = Image.open(image).convert("RGB")
    palette = make_palette(R, G, B)
    img_arr = np.array(img, dtype=np.int16)  # shape (H, W, 3)

    r_mask = img_arr[:, :, 0] >= 128
    g_mask = img_arr[:, :, 1] >= 128
    b_mask = img_arr[:, :, 2] >= 128

    # Combine channel masks into a 3-bit palette index per pixel
    index_image = (r_mask.astype(np.uint8) << 2) | \
                  (g_mask.astype(np.uint8) << 1) | \
                  b_mask.astype(np.uint8)

    if debug:
        mapped_img_arr = palette[index_image]
        mapped_img = Image.fromarray(mapped_img_arr)
        mapped_img.save(f"{str(image).split('.')[0]}_converted.png")

    return index_image


def is_black(pixel):
    """Return True if pixel is black (handles RGB or RGBA)."""
    if len(pixel) == 4:
        return pixel[0] == 0 and pixel[1] == 0 and pixel[2] == 0 and pixel[3] != 0
    return pixel[0] == 0 and pixel[1] == 0 and pixel[2] == 0


def natural_key(s):
    # Sort by filename key (s[0]), not the frame ID, to get stable ordering
    # across manifest entries with arbitrary ID strings.
    parts = re.split(r'(\d+)', s[0])
    return [int(p) if p.isdigit() else p for p in parts]


def extract_frames(img):
    width, height = img.size
    pixels = img.load()
    visited = [[False]*height for _ in range(width)]
    frames = []

    def flood_fill(start_x, start_y):
        q = deque([(start_x, start_y)])
        coords = []
        min_x, min_y = start_x, start_y
        max_x, max_y = start_x, start_y

        while q:
            x, y = q.popleft()
            if not (0 <= x < width and 0 <= y < height):
                continue
            if visited[x][y]:
                continue
            visited[x][y] = True
            pix = pixels[x, y]
            if is_black(pix):
                continue

            coords.append((x, y))

            if x < min_x: min_x = x
            if y < min_y: min_y = y
            if x > max_x: max_x = x
            if y > max_y: max_y = y

            q.extend([(x+1,y), (x-1,y), (x,y+1), (x,y-1)])
            q.extend([(x-1,y-1), (x-1,y+1), (x+1,y+1), (x+1,y-1)])

        return coords, (min_x, min_y, max_x+1, max_y+1)

    for y in range(height):
        for x in range(width):
            if visited[x][y]:
                continue
            if not is_black(pixels[x, y]):
                coords, bbox = flood_fill(x, y)
                if coords:
                    frames.append((coords, bbox))
    return frames


def encode_row(row, frame_name):
    """
    Pack one row of 3-bit palette indices into three bit-planes.

    Each palette index has 3 bits (red/green/blue plane).
    We emit one byte per plane per row, MSB = leftmost pixel:
      plane_r: bit 2 of each index, packed left-to-right into bits 7..0
      plane_g: bit 1 of each index
      plane_b: bit 0 of each index

    Max row width is 8 pixels; wider rows are a hard error.
    """
    if len(row) > 8:
        print(f"ERROR: {frame_name} row width {len(row)} exceeds maximum of 8 pixels — "
              f"output will be corrupted", file=sys.stderr)
        sys.exit(1)

    plane_r = 0  # red plane   (palette index bit 2)
    plane_g = 0  # green plane (palette index bit 1)
    plane_b = 0  # blue plane  (palette index bit 0)

    for j, val in enumerate(row):
        bit = 7 - j
        plane_r |= ((val >> 2) & 1) << bit
        plane_g |= ((val >> 1) & 1) << bit
        plane_b |= (val & 1) << bit

    return plane_r, plane_g, plane_b


def load_manifest(path):
    mapping = {}
    if not os.path.exists(path):
        return mapping
    with open(path, 'r') as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) != 5:
                continue
            id_, name, height, x, y = parts
            mapping[name] = (id_, height, x, y)
    return mapping


def main():
    parser = argparse.ArgumentParser(
        description="Process sprite sheet PNG(s) into C frame data."
    )
    parser.add_argument(
        "infiles",
        nargs="+",
        metavar="SPRITE_SHEET",
        help="Input sprite sheet PNG file(s)",
    )
    parser.add_argument(
        "-o", "--output-prefix",
        default="./frames",
        metavar="PREFIX",
        help="Output filename prefix for generated .h/.c files "
             "(default: ./frames)",
    )
    parser.add_argument(
        "-m", "--manifest",
        default="manifest.txt",
        metavar="FILE",
        help="Manifest file path (default: manifest.txt)",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Save _converted.png alongside each temp frame for inspection",
    )

    args = parser.parse_args()

    mapping = load_manifest(args.manifest)

    # Track which frame names were seen in this run; used to warn about stale
    # manifest entries that no longer correspond to any extracted frame.
    seen_frames = set()

    image_set = {}  # pixel-hash -> frame_name; detects pixel-identical duplicates
    frame_counter = 0

    for infile in args.infiles:
        base, _ = os.path.splitext(infile)

        img = Image.open(infile).convert("RGBA")
        frames = extract_frames(img)

        for coords, bbox in frames:
            min_x, min_y, max_x, max_y = bbox
            frame_img = Image.new("RGB", (max_x - min_x, max_y - min_y), (0, 0, 0))
            src_pixels = img.load()
            dst_pixels = frame_img.load()

            for (x, y) in coords:
                r, g, b, a = src_pixels[x, y]
                dst_pixels[x - min_x, y - min_y] = (r, g, b)

            frame_name = f'{base}_{min_x}_{min_y}'
            outfile = Path(f"temp/{frame_name}.png")
            outfile.parent.mkdir(parents=True, exist_ok=True)
            frame_img.save(outfile)

            center_x = int((max_x - min_x) / 2)
            center_y = (max_y - min_y) - 1
            frame_height = max_y - min_y
            frame_id = f'FRAME_{frame_counter}'

            if frame_name in mapping:
                frame_id    = mapping[frame_name][0]
                frame_height = mapping[frame_name][1]
                center_x    = mapping[frame_name][2]
                center_y    = mapping[frame_name][3]

            # Duplicate detection: identical pixel content == same frame.
            pixel_hash = tuple(frame_img.tobytes())
            if pixel_hash in image_set:
                print(f'DUPLICATE of {image_set[pixel_hash]} as {frame_name} dropped')
            else:
                image_set[pixel_hash] = frame_name
                mapping[frame_name] = (frame_id, frame_height, center_x, center_y)
                seen_frames.add(frame_name)

            frame_counter += 1
            print(f"Saved {outfile}")

    # Warn about manifest entries that weren't seen in this run — they may be
    # stale (sprite removed or relocated) and will cause a FileNotFoundError
    # in pixmap() if left in place.
    stale = set(mapping.keys()) - seen_frames
    if stale:
        print(f"WARNING: {len(stale)} stale manifest entry/entries not found in "
              f"input sprites (will be dropped from output):", file=sys.stderr)
        for name in sorted(stale):
            print(f"  {name}", file=sys.stderr)
        for name in stale:
            del mapping[name]

    with open(args.manifest, "w") as f:
        for label, (id_, height, x, y) in mapping.items():
            f.write(f"{id_},{label},{height},{x},{y}\n")

    header_path = f"{args.output_prefix}.h"
    c_path = f"{args.output_prefix}.c"

    sorted_keys = sorted(mapping.items(), key=natural_key)

    with open(header_path, 'w') as hdr:
        hdr.write('#pragma once\n')
        hdr.write('#include <stdint.h>\n\n')
        hdr.write('extern uint16_t frameLowTable[];\n')
        hdr.write('extern const unsigned char frameHeight[];\n')
        hdr.write('extern const unsigned char frameCenterX[];\n')
        hdr.write('extern const unsigned char frameCenterY[];\n\n')
        hdr.write('// LEM(r, g, b): expands to the combined byte followed by each plane byte.\n')
        hdr.write('// Plane encoding: r=red plane, g=green plane, b=blue plane (MSB=leftmost pixel).\n')
        hdr.write('#define LEM(r, g, b) ((r) | (g) | (b)), (r), (g), (b)\n\n')
        hdr.write('enum FRAME {\n')

        for i, k in enumerate(sorted_keys):
            (id_, height, x, y) = mapping[k[0]]
            hdr.write(f'    {id_}, // {i}\n')

        action_names = [
            'ACTION_JOYSTICK',
            'ACTION_MOVE',
            'ACTION_POSITION',
            'ACTION_DELAY',
            'ACTION_FLIP',
            'ACTION_LOOP',
            'ACTION_PUSH',
            'ACTION_AUTOFRAME',
            'ACTION_NEXTSQUARE',
            'ACTION_STOP',
        ]
        for i, name in enumerate(action_names, start=len(sorted_keys)):
            hdr.write(f'    {name}, // {i}\n')

        hdr.write('};\n')

    with open(c_path, 'w') as src:
        header_include = Path(header_path).name
        src.write(f'#include "{header_include}"\n\n')

        for k in sorted_keys:
            (id_, height, x, y) = mapping[k[0]]
            src.write(f'__attribute__((used)) const unsigned char {k[0]}[] = {{ // {id_}\n')

            data = pixmap(f'temp/{k[0]}.png', debug=args.debug)
            print(f'file {k[0]}')

            for row in data:
                plane_r, plane_g, plane_b = encode_row(row, k[0])
                src.write(f'    LEM({plane_r},{plane_g},{plane_b}),\n')

            src.write(f'}};\n\n')

        src.write('__asm__(\".section .rodata\\n\"\n')
        src.write('        \".global frameLowTable\\n\"\n')
        src.write('        \"frameLowTable:\\n\"\n')
        for k in sorted_keys:
            src.write(f'        \".hword {k[0]}\\n\"\n')
        src.write('       );\n\n')

        src.write('const unsigned char frameHeight[] = {\n')
        for k in sorted_keys:
            (id_, height, x, y) = mapping[k[0]]
            src.write(f'    {height}, // {id_}\n')
        src.write('};\n\n')

        src.write('const unsigned char frameCenterX[] = {\n')
        for k in sorted_keys:
            (id_, height, x, y) = mapping[k[0]]
            src.write(f'    {x}, // {id_}\n')
        src.write('};\n\n')

        src.write('const unsigned char frameCenterY[] = {\n')
        for k in sorted_keys:
            (id_, height, x, y) = mapping[k[0]]
            src.write(f'    {y}, // {id_}\n')
        src.write('};\n\n')


if __name__ == "__main__":
    main()