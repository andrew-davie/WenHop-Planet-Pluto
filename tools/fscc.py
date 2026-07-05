#!/usr/bin/env python3
"""
atari_chronocolour.py -- convert an image into a 40x66 "optimal chronocolour"
trixel grid for the Atari 2600.

BACKGROUND / MODEL
------------------
Display is 40 pixels wide, 198 scanlines deep. Every group of 3 consecutive
scanlines is a "trixel line" (66 of them). Each of the 3 physical scanlines
in a trixel line is drawn in a single solid Atari colour for that line, and
each of its 40 pixels can be on (shows that scanline's colour) or off (shows
black). Across 3 successive video frames the display code "rolls" which of
the 3 chosen colours (A, B, C) is shown on which physical scanline:

    frame 0: row0=A  row1=B  row2=C
    frame 1: row0=B  row1=C  row2=A
    frame 2: row0=C  row1=A  row2=B

i.e. colour shown on physical row r, frame f  =  colour[(r + f) mod 3]

A viewer's eye/persistence blends both the 3 adjacent scanlines (spatially)
and the 3 rolled frames (temporally). Averaging all 9 (row, frame) samples
shows that the perceived colour of a trixel column depends only on how many
of its 3 pixels are switched on (k = 0..3), not on *which* ones:

    perceived = (k / 3) * mean(A, B, C)

So each trixel line behaves like a single hue/colour "mean(A,B,C)" that can
be shown at 4 brightness levels (0, 1/3, 2/3, 1) per column. This script:

  1. Downsamples the source image to 40x66 target colours (one per trixel).
  2. For each of the 66 trixel lines, finds the best A,B,C triple (each an
     independent colour taken from the real 128-colour Atari NTSC palette)
     so that mean(A,B,C) is as good a "reference colour" as possible for
     that line -- this is a rank-1 (single-direction) fit across the 40
     columns, found via SVD, then snapped to real palette colours.
  3. For each column, picks the brightness level k in {0,1,2,3} that best
     reproduces its target colour as (k/3)*mean(A,B,C), and turns on the
     corresponding pixels in each of the 3 physical scanline bitmaps.
  4. Emits a C header + source with the palette triples and scanline
     bitmaps for all 66 trixel lines.
  5. Renders two PNG previews:
       - <prefix>_reconstructed_singleframe.png : exactly what the screen
         looks like on ONE frame with no rolling applied yet (frame 0,
         one flat colour per scanline) -- the "non-rolling" reconstruction.
       - <prefix>_reconstructed_blended.png     : the intended, fully
         blended/averaged appearance once rolling + persistence kick in.

Atari NTSC palette
------------------
The Atari 2600 TIA colour byte is HHHHLLL0 in bits: bits 7-4 select one of
16 hues (hue 0 = greyscale), bits 3-1 select one of 8 luminances, bit 0 is
unused -- giving 128 usable byte values (0, 2, 4 ... 254).
RGB approximations below are the commonly published NTSC TIA chart values
(as tabulated by Glenn Saunders / randomterrain.com and mirrored on lospec,
sourced from the Wikimedia "Atari2600_NTSC_palette.png" reference chart),
listed luma-major (8 rows) / hue-minor (16 columns per row) as on that
chart. Exact analog decode varies slightly by TV/emulator; substitute your
own dev-kit's palette constants if you need pixel-exact fidelity.

USAGE
-----
    python3 atari_chronocolour.py input.png [-o OUTPUT_PREFIX]

Produces:
    <prefix>.h
    <prefix>.c
    <prefix>_reconstructed_singleframe.png
    <prefix>_reconstructed_blended.png
"""
import sys
import argparse
import numpy as np
from PIL import Image

WIDTH = 40
TRIXEL_ROWS = 66
SCANLINES = TRIXEL_ROWS * 3          # 198
BYTES_PER_ROW = WIDTH // 8           # 5 (40 is exactly divisible by 8)
UPSCALE = 8                          # preview PNG pixel scale factor

# --------------------------------------------------------------------------
# Atari 2600 NTSC palette: 8 luma rows x 16 hue columns (see docstring).
# --------------------------------------------------------------------------
_NTSC_HEX = [
    "000000", "444400", "702800", "841800", "880000", "78005c", "480078", "140084",
    "000088", "00187c", "002c5c", "00402c", "003c00", "143800", "2c3000", "442800",
    "404040", "646410", "844414", "983418", "9c2020", "8c2074", "602090", "302098",
    "1c209c", "1c3890", "1c4c78", "1c5c48", "205c20", "345c1c", "4c501c", "644818",
    "6c6c6c", "848424", "985c28", "ac5030", "b03c3c", "a03c88", "783ca4", "4c3cac",
    "3840b0", "3854a8", "386890", "387c64", "407c40", "507c38", "687034", "846830",
    "909090", "a0a034", "ac783c", "c06848", "c05858", "b0589c", "8c58b8", "6858c0",
    "505cc0", "5070bc", "5084ac", "509c80", "5c9c5c", "6c9850", "848c4c", "a08444",
    "b0b0b0", "b8b840", "bc8c4c", "d0805c", "d07070", "c070b0", "a070cc", "7c70d0",
    "6874d0", "6888cc", "689cc0", "68b494", "74b474", "84b468", "9ca864", "b89c58",
    "c8c8c8", "d0d050", "cca05c", "e09470", "e08888", "d084c0", "b484dc", "9488e0",
    "7c8ce0", "7c9cdc", "7cb4d4", "7cd0ac", "8cd08c", "9ccc7c", "b4c078", "d0b46c",
    "dcdcdc", "e8e85c", "dcb468", "eca880", "eca0a0", "dc9cd0", "c49cec", "a8a0ec",
    "90a4ec", "90b4ec", "90cce8", "90e4c0", "a4e4a4", "b4e490", "ccd488", "e8cc7c",
    "ececec", "fcfc68", "fcbc94", "fcb4b4", "ecb0e0", "d4b0fc", "bcb4fc", "a4b8fc",
    "a4c8fc", "a4e0fc", "a4fcd4", "b8fcb8", "c8fca4", "e0ec9c", "fce08c", "ffffff",
]


def _hex_to_rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def _build_palette():
    byte_to_rgb = {}
    for luma in range(8):
        for hue in range(16):
            byte = (hue << 4) | (luma << 1)
            byte_to_rgb[byte] = _hex_to_rgb(_NTSC_HEX[luma * 16 + hue])
    bytes_sorted = sorted(byte_to_rgb)
    rgb_arr = np.array([byte_to_rgb[b] for b in bytes_sorted], dtype=np.float64)
    return bytes_sorted, rgb_arr


PALETTE_BYTES, PALETTE_RGB = _build_palette()   # 128 entries each

# Which of the 3 physical rows (top, mid, bottom) are lit for a given
# brightness level k (0..3). Order within a level doesn't change the
# perceived (rolled+blended) colour -- see docstring -- so these are chosen
# just to spread "on" pixels evenly and avoid always favouring one row.
K_TO_PATTERN = {
    0: (0, 0, 0),
    1: (0, 1, 0),
    2: (1, 0, 1),
    3: (1, 1, 1),
}


def load_target_colours(path):
    """Load an image and resample it to 40x66 target RGB colours."""
    img = Image.open(path).convert("RGB")
    img = img.resize((WIDTH, TRIXEL_ROWS), Image.LANCZOS)
    return np.asarray(img, dtype=np.float64)  # shape (66, 40, 3)


def best_reference_colour(row_targets):
    """
    row_targets: (40, 3) array of target colours for one trixel line.
    Returns a continuous RGB "reference" colour R such that (k/3)*R for
    k in 0..3 best explains the 40 columns (best rank-1 fit through the
    origin, found via SVD).
    """
    # Rank-1 approx of row_targets ~= outer(k_i, R_dir)
    _, _, vt = np.linalg.svd(row_targets, full_matrices=False)
    r_dir = vt[0]
    proj = row_targets @ r_dir
    if proj.mean() < 0:
        r_dir = -r_dir
        proj = -proj
    proj = np.clip(proj, 0, None)
    if proj.max() <= 1e-6:
        return np.zeros(3)
    scale = np.percentile(proj, 95)
    if scale <= 1e-6:
        scale = proj.max()
    r_continuous = r_dir * scale
    return np.clip(r_continuous, 0, 255)


def nearest_palette_candidates(colour, k=10):
    d = np.sum((PALETTE_RGB - colour) ** 2, axis=1)
    idx = np.argsort(d)[:k]
    return idx  # indices into PALETTE_BYTES / PALETTE_RGB


def choose_abc(r_continuous, k_candidates=10):
    """Search combinations of 3 palette colours (with repeats) whose mean
    is closest to r_continuous; restricted to the nearest-k candidates for
    tractability."""
    cand_idx = nearest_palette_candidates(r_continuous, k_candidates)
    cand_rgb = PALETTE_RGB[cand_idx]
    target3 = r_continuous * 3.0
    best = None
    best_err = None
    n = len(cand_idx)
    for i in range(n):
        for j in range(i, n):
            partial2 = cand_rgb[i] + cand_rgb[j]
            remaining = target3 - partial2
            # best third colour for this pair, searched across full palette
            d3 = np.sum((PALETTE_RGB - remaining) ** 2, axis=1)
            k_best = np.argmin(d3)
            total = cand_rgb[i] + cand_rgb[j] + PALETTE_RGB[k_best]
            err = np.sum((total - target3) ** 2)
            if best_err is None or err < best_err:
                best_err = err
                best = (cand_idx[i], cand_idx[j], k_best)
    ai, bi, ci = best
    return (PALETTE_BYTES[ai], PALETTE_BYTES[bi], PALETTE_BYTES[ci])


def choose_k_per_column(row_targets, r_final):
    """row_targets: (40,3); r_final: (3,) actual mean(A,B,C).
    Returns int array (40,) with values in {0,1,2,3}."""
    levels = np.array([0, 1 / 3, 2 / 3, 1.0])
    # (40,4,3) = levels[k] * r_final, compare against target
    candidates = levels[None, :, None] * r_final[None, None, :]
    diffs = row_targets[:, None, :] - candidates
    errs = np.sum(diffs ** 2, axis=2)  # (40,4)
    return np.argmin(errs, axis=1)


def pack_bits(bit_list):
    """bit_list: list/array of 0/1, length 40 (MSB-first within each byte,
    column 0 = bit7 of byte 0)."""
    out = bytearray(BYTES_PER_ROW)
    for col, bit in enumerate(bit_list):
        if bit:
            byte_i = col // 8
            bit_i = 7 - (col % 8)
            out[byte_i] |= (1 << bit_i)
    return bytes(out)


def process_image(path):
    targets = load_target_colours(path)  # (66,40,3)

    palettes = []      # list of (A,B,C) bytes, len 66
    row_bits = []       # list of (top_bits[40], mid_bits[40], bot_bits[40])
    blended_preview = np.zeros((TRIXEL_ROWS, WIDTH, 3), dtype=np.uint8)
    singleframe_preview = np.zeros((SCANLINES, WIDTH, 3), dtype=np.uint8)

    total_sq_err = 0.0

    for line in range(TRIXEL_ROWS):
        row_targets = targets[line]  # (40,3)
        r_cont = best_reference_colour(row_targets)
        a, b, c = choose_abc(r_cont)
        r_final = (PALETTE_RGB[PALETTE_BYTES.index(a)]
                   + PALETTE_RGB[PALETTE_BYTES.index(b)]
                   + PALETTE_RGB[PALETTE_BYTES.index(c)]) / 3.0
        k_col = choose_k_per_column(row_targets, r_final)

        top_bits = [0] * WIDTH
        mid_bits = [0] * WIDTH
        bot_bits = [0] * WIDTH
        for col in range(WIDTH):
            t, m, bo = K_TO_PATTERN[int(k_col[col])]
            top_bits[col], mid_bits[col], bot_bits[col] = t, m, bo

        palettes.append((a, b, c))
        row_bits.append((top_bits, mid_bits, bot_bits))

        # previews
        for col in range(WIDTH):
            level = k_col[col] / 3.0
            blended_preview[line, col] = np.clip(level * r_final, 0, 255).astype(np.uint8)

        colour_rgb = {0: PALETTE_RGB[PALETTE_BYTES.index(a)],
                      1: PALETTE_RGB[PALETTE_BYTES.index(b)],
                      2: PALETTE_RGB[PALETTE_BYTES.index(c)]}
        for sub_row, bits in enumerate((top_bits, mid_bits, bot_bits)):
            scan_y = line * 3 + sub_row
            for col in range(WIDTH):
                if bits[col]:
                    singleframe_preview[scan_y, col] = colour_rgb[sub_row].astype(np.uint8)
                # else stays black (0,0,0)

        total_sq_err += np.sum((row_targets[np.arange(WIDTH)] -
                                 (k_col[:, None] / 3.0) * r_final[None, :]) ** 2)

    rmse = (total_sq_err / (TRIXEL_ROWS * WIDTH * 3)) ** 0.5
    return palettes, row_bits, blended_preview, singleframe_preview, rmse


def write_c_files(prefix, palettes, row_bits):
    header_path = f"{prefix}.h"
    source_path = f"{prefix}.c"
    guard = prefix.upper().replace("-", "_").replace(" ", "_").split("/")[-1] + "_H"

    with open(header_path, "w") as f:
        f.write(f"""#ifndef {guard}
#define {guard}

#include <stdint.h>

/* Optimal-colour "phased chronocolour" trixel data.
 * Generated by atari_chronocolour.py -- do not hand-edit.
 *
 * Screen is TRIXEL_WIDTH pixels wide, TRIXEL_ROWS trixel-lines deep
 * (TRIXEL_ROWS * 3 == {SCANLINES} physical scanlines).
 *
 * Each TrixelLine holds:
 *   colour[3]   - the A, B, C Atari colour byte values for this line
 *                 (already in COLUP0/COLUP1/COLUBK register format,
 *                 i.e. bits 7-4 = hue, bits 3-1 = luminance, bit0 = 0)
 *   bits[3][{BYTES_PER_ROW}] - one 40-bit (packed MSB-first) on/off bitmap per
 *                 physical scanline of the trixel: bits[0]=top row,
 *                 bits[1]=middle row, bits[2]=bottom row. Column 0 is
 *                 bit 7 of bits[row][0].
 *
 * The colour actually shown on physical row r during video frame f is
 * colour[(r + f) % 3] -- i.e. row0:A,B,C  row1:B,C,A  row2:C,A,B across
 * frames 0,1,2 and repeating. That rolling is performed by your display
 * code; this data only supplies the fixed per-row bitmaps and the 3
 * colours to roll through.
 */

#define TRIXEL_WIDTH      {WIDTH}
#define TRIXEL_ROWS       {TRIXEL_ROWS}
#define TRIXEL_BYTES_PER_ROW {BYTES_PER_ROW}

typedef struct {{
    uint8_t colour[3];
    uint8_t bits[3][TRIXEL_BYTES_PER_ROW];
}} TrixelLine;

extern const TrixelLine trixel_screen[TRIXEL_ROWS];

#endif /* {guard} */
""")

    with open(source_path, "w") as f:
        base = prefix.split("/")[-1]
        f.write(f'#include "{base}.h"\n\n')
        f.write("const TrixelLine trixel_screen[TRIXEL_ROWS] = {\n")
        for line in range(TRIXEL_ROWS):
            a, b, c = palettes[line]
            top_bits, mid_bits, bot_bits = row_bits[line]
            top_b = pack_bits(top_bits)
            mid_b = pack_bits(mid_bits)
            bot_b = pack_bits(bot_bits)

            def fmt(bs):
                return ", ".join(f"0x{x:02X}" for x in bs)

            f.write(f"    {{ {{0x{a:02X}, 0x{b:02X}, 0x{c:02X}}}, "
                    f"{{ {{{fmt(top_b)}}}, {{{fmt(mid_b)}}}, {{{fmt(bot_b)}}} }} }},"
                    f" /* line {line} */\n")
        f.write("};\n")

    return header_path, source_path


def save_preview(arr, path, upscale=UPSCALE):
    img = Image.fromarray(arr, mode="RGB")
    img = img.resize((arr.shape[1] * upscale, arr.shape[0] * upscale), Image.NEAREST)
    img.save(path)


def main():
    ap = argparse.ArgumentParser(description="Convert an image to Atari 2600 optimal-colour chronocolour trixel data.")
    ap.add_argument("image", help="source image filename")
    ap.add_argument("-o", "--output-prefix", default=None,
                    help="output filename prefix (default: derived from image name)")
    args = ap.parse_args()

    prefix = args.output_prefix
    if prefix is None:
        base = args.image.rsplit("/", 1)[-1]
        base = base.rsplit(".", 1)[0]
        prefix = base + "_trixel"

    palettes, row_bits, blended, singleframe, rmse = process_image(args.image)

    header_path, source_path = write_c_files(prefix, palettes, row_bits)

    blended_path = f"{prefix}_reconstructed_blended.png"
    singleframe_path = f"{prefix}_reconstructed_singleframe.png"
    save_preview(blended, blended_path)
    save_preview(singleframe, singleframe_path)

    print(f"Wrote {header_path}")
    print(f"Wrote {source_path}")
    print(f"Wrote {blended_path}  (intended fully-blended appearance)")
    print(f"Wrote {singleframe_path}  (single non-rolled frame, one colour per scanline)")
    print(f"RMS colour error (0-255 scale): {rmse:.2f}")


if __name__ == "__main__":
    main()