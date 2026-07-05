#!/usr/bin/env python3
"""
atari_scanline_blend.py -- convert an image into 198 single-colour Atari
2600 scanlines (WIDTH pixels wide, currently 48) whose on-pixels, blended
vertically with their nearby scanlines, reconstruct the source image as
closely as possible in a single video frame.

Optionally (--frames 2) also solves a SECOND frame that alternates with
the first every other video frame; the eye's persistence of vision then
temporally averages the two (50/50) on top of the usual vertical spatial
blend, letting the pair reconstruct the source more closely than either
frame could alone. See solve_two_frame() for the maths.

MODEL
-----
The display is WIDTH x HEIGHT (48x198). Every scanline y has:
  - one Atari NTSC colour (of the 128 available)
  - WIDTH pixels, each independently on or off

An "on" pixel shows that scanline's colour; an "off" pixel shows black.
The eye (composite video bleed / phosphor persistence / scanline
proximity) blends each displayed pixel vertically with the pixels
directly above and below it, out to a distance of 3 scanlines, with
weight falling off with distance:

    blended(y, x) = sum_{d=-3..3} weight(d) * raw(y + d, x)

    where raw(y, x) = colour[y]  if pixel (y,x) is on
                      black      otherwise

    weight(d) = exp(-d^2 / (2*sigma^2))   (Gaussian falloff, default sigma=1.0)

Goal: choose colour[y] (one of 128 Atari colours) and on/off[y][x] for
every scanline so that blended(y,x) is as close as possible to the
(40x198-resampled) source image, in a least-squares sense.

This is a large joint combinatorial problem (198 colour choices x 2^40
on/off patterns each, all coupled vertically). It's solved here by
alternating, physically-motivated local optimisation (this is allowed to
take a while -- a minute or two on a big image is fine):

  1. Initialise colour[y] from the average of that scanline's target
     pixels (snapped to the nearest real Atari colour), and on/off from
     a simple "closer to colour[y] than to black" rule.
  2. Repeat for several rounds:
       a. ON/OFF REFINEMENT (Direct Binary Search): vertical blending
          only mixes within a column, never across columns, so the 40
          columns are independent 1-D problems. For each column, sweep
          down the 198 rows (in randomised order) and greedily flip any
          pixel whose flip reduces total error against the target, given
          the *current* scanline colours. Repeat sweeps until a column
          stops improving.

          An additional SWAP move (--swaps, off by default) tries
          toggling an adjacent (y, y+1) pair together, on the theory that
          it could catch improvements where flipping either pixel alone
          makes things worse but the pair together helps. Tested this
          properly (separate RNG stream for swap ordering so it's not
          confounded with flip-sweep randomness, and let flip+swap fully
          alternate to real convergence, not an arbitrary iteration cap)
          and it consistently converges to a WORSE final result than
          flips alone -- every seed tried, both metrics, on a real photo.
          Not a bug: interleaving a second move type changes which states
          get visited, and greedy coordinate descent has no guarantee
          that visiting more states lands you somewhere better -- it can
          walk into a deeper local optimum for one move type that's
          actually worse once you're stuck there. Left available for
          experimentation but off by default on the evidence.
       b. COLOUR REFINEMENT: for each scanline, given the current on/off
          pattern (of this line and its neighbours), solve the (closed
          form) weighted least-squares colour that best explains the
          rows it influences, then snap to the nearest real Atari
          colour.
     Both steps only ever move to a strictly-better or equal total
     error, so the process converges to a local optimum. Since DBS is a
     local search, different random sweep orders land in different local
     optima -- solve_best_of() runs several seeds and keeps the best.

     Error is measured per-pixel as a WEIGHTED squared RGB difference
     (--metric luma, the default, uses ITU-R BT.601 luma coefficients
     0.299/0.587/0.114 so the optimiser spends its on/off budget on
     differences the eye actually notices, rather than treating R/G/B as
     equally important; --metric rgb reverts to flat unweighted error).
  3. Emit the result as C data (one colour byte + one WIDTH-bit on/off
     bitmap per scanline) and two PNG previews:
       - <prefix>_reconstructed.png     the blended result (this is what
         the optimiser is actually judged against -- the "does it look
         good" answer)
       - <prefix>_reconstructed_raw.png the unblended raw pixels actually
         written to the TIA (flat colour where on, black where off) --
         useful to see the underlying dot pattern.

Atari NTSC palette: see comment in the palette table below (same sourced
128-colour NTSC chart used previously; hue<<4 | luma<<1 byte format).

USAGE
-----
    python3 atari_scanline_blend.py input.png [-o OUTPUT_PREFIX]
                                     [--width 48] [--height 198]
                                     [--sigma 1.0] [--radius 3]
                                     [--rounds 10] [--sweeps 3]
                                     [--metric luma] [--seeds 3]
                                     [--smoothness 0.01] [--swaps]
                                     [--frames 1|2]

Screen is WIDTH x HEIGHT, configurable via --width/--height (default 48x198).
Source images are assumed to already be at the correct display aspect ratio
before this tool is called -- it does a plain resize, no letterboxing or
cropping. If you change --width, you likely need to adjust WIDTH_STRETCH's
underlying assumption too (see comment above WIDTH_STRETCH) to keep preview
PNGs' aspect ratio honest.
"""
import argparse
import numpy as np
from PIL import Image

# WIDTH/HEIGHT/BYTES_PER_ROW/WIDTH_STRETCH are defaults, overridden from the
# command line (--width/--height) in main() before anything else runs. Every
# function below reads these as module globals (not captured at def-time),
# so reassigning them up front is enough to retarget the whole tool at a
# different screen size.
WIDTH = 48
HEIGHT = 198
BYTES_PER_ROW = (WIDTH + 7) // 8   # ceil-div: pads the last byte if WIDTH isn't a multiple of 8
UPSCALE = 6
# Correct for non-square Atari pixels in preview PNGs. Calibrated at WIDTH=40
# assuming ~160 visible colour clocks (4 clocks/pixel -> stretch 4x). At other
# widths across the same physical visible width that's 160/WIDTH clocks/pixel
# -- scaled proportionally. Revisit if the real pixel geometry assumed here
# is wrong for your target width.
WIDTH_STRETCH = 4 * (40 / WIDTH)

# --------------------------------------------------------------------------
# Atari 2600 NTSC palette: 8 luma rows x 16 hue columns.
# (Same source/derivation as the earlier chronocolour tool: commonly
# published NTSC TIA chart values, luma-major/hue-minor, byte = hue<<4|luma<<1.)
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

# ITU-R BT.601 luma coefficients -- used to weight R/G/B error so the
# optimiser spends its on/off budget on differences the eye actually
# notices, rather than treating all three channels as equally important.
LUMA_WEIGHTS = np.array([0.299, 0.587, 0.114])
RGB_WEIGHTS = np.array([1.0, 1.0, 1.0])


def get_weights(metric):
    if metric == "luma":
        return LUMA_WEIGHTS
    if metric == "rgb":
        return RGB_WEIGHTS
    raise ValueError(f"unknown metric {metric!r}")


def nearest_palette_index(colour, weights=RGB_WEIGHTS):
    d = np.sum(weights * (PALETTE_RGB - colour) ** 2, axis=1)
    return int(np.argmin(d))


def nearest_palette_index_smooth(colour, weights, neighbours, smoothness):
    """Like nearest_palette_index, but also penalises palette choices that
    are far from already-committed neighbouring scanlines' colours. This
    exists because plain per-line least-squares snapping, with no penalty
    for jumping to a totally different hue than the line next to it, lets
    smooth photographic gradients (skin tones etc) come out as jittery
    hue-shifting noise: many different quantised palette entries are all
    nearly-equally-good fits for any *single* line in isolation, so the
    greedy per-line solve chatters between them arbitrarily even though the
    source barely changes row to row. Blending in a same-units squared
    distance to each neighbour's colour breaks close ties in favour of
    smoothness, without overriding a genuinely large jump when the fit
    error actually justifies one."""
    d = np.sum(weights * (PALETTE_RGB - colour) ** 2, axis=1)
    if smoothness > 0:
        for nb in neighbours:
            d = d + smoothness * np.sum((PALETTE_RGB - nb) ** 2, axis=1)
    return int(np.argmin(d))


def load_target(path):
    """Resample to the screen's native WIDTHxHEIGHT. The source is assumed
    to already be at the correct display aspect ratio (see WIDTH_STRETCH)
    -- this does a plain resize, no letterboxing/cropping."""
    img = Image.open(path).convert("RGB")
    img = img.resize((WIDTH, HEIGHT), Image.LANCZOS)
    return np.asarray(img, dtype=np.float64)  # (HEIGHT, WIDTH, 3)


def make_kernel(sigma, radius):
    d = np.arange(-radius, radius + 1)
    w = np.exp(-(d.astype(np.float64) ** 2) / (2.0 * sigma * sigma))
    return {int(k): float(v) for k, v in zip(d, w)}


def solve(target, sigma=1.0, radius=3, rounds=10, sweeps=3, seed=0,
          metric="luma", swaps=False, smoothness=0.01, verbose=True, label=""):
    weights = get_weights(metric)
    rng = np.random.default_rng(seed)
    # Separate RNG stream for swap-sweep ordering so that turning `swaps`
    # on/off doesn't perturb the flip-sweep random draws (shared RNG state
    # would make swaps=True/False runs diverge from the very first sweep,
    # confounding any comparison between them).
    swap_rng = np.random.default_rng(seed + 999983)
    kernel = make_kernel(sigma, radius)          # {offset: weight}
    offsets = sorted(kernel)
    sumsq_w = sum(kernel[d] ** 2 for d in offsets)

    def werr(diff):
        """Weighted squared error for a single (or batch of) RGB diff(s)."""
        return np.sum(weights * diff ** 2, axis=-1)

    # --- initialise colours from row averages, snapped to palette -------
    # (smoothed against the previous row only, top to bottom -- there's no
    # "next row" colour yet on this first pass)
    color_idx = np.zeros(HEIGHT, dtype=np.int32)
    colour = np.zeros((HEIGHT, 3), dtype=np.float64)
    for y in range(HEIGHT):
        avg = target[y].mean(axis=0)
        neighbours = [colour[y - 1]] if y > 0 else []
        color_idx[y] = nearest_palette_index_smooth(avg, weights, neighbours, smoothness)
        colour[y] = PALETTE_RGB[color_idx[y]]

    # --- initialise on/off: on if target closer to colour[y] than black -
    on = np.zeros((HEIGHT, WIDTH), dtype=bool)
    for y in range(HEIGHT):
        d_colour = werr(target[y] - colour[y])
        d_black = werr(target[y])
        on[y] = d_colour < d_black

    # --- blended (198,40,3): current predicted display ------------------
    def full_rebuild():
        b = np.zeros((HEIGHT, WIDTH, 3), dtype=np.float64)
        for d in offsets:
            w = kernel[d]
            if w == 0:
                continue
            y0, y1 = max(0, -d), min(HEIGHT, HEIGHT - d)   # valid y such that y+d in range
            src = (on[y0 + d:y1 + d, :, None] * colour[y0 + d:y1 + d, None, :])
            b[y0:y1] += w * src
        return b

    blended = full_rebuild()

    def total_werror():
        return float(np.sum(weights * (target - blended) ** 2))

    def total_rgb_sse():
        return float(np.sum((target - blended) ** 2))

    tag = f"[{label}] " if label else ""
    if verbose:
        print(f"{tag}round 0 (init): weighted SSE = {total_werror():.1f}")

    for rnd in range(1, rounds + 1):
        # ---------------- phase 1: on/off refinement (DBS, per column) --
        for x in range(WIDTH):
            # Alternate flip-convergence and a swap-sweep until NEITHER
            # improves anything -- a real local optimum over both move
            # types, not an arbitrarily-capped number of alternations
            # (capping this early left improvements on the table and
            # actually made luma-weighted runs converge worse than
            # unweighted ones purely from being cut off too soon).
            for _outer in range(8):
                # -- flip sweeps: single-pixel on/off toggles --
                any_flip_changed = False
                for sweep in range(sweeps):
                    order = rng.permutation(HEIGHT)
                    changed = False
                    for y in order:
                        y = int(y)
                        cur_on = on[y, x]
                        col = colour[y]
                        delta_total = 0.0
                        affected = []
                        for d in offsets:
                            yy = y + d
                            if yy < 0 or yy >= HEIGHT:
                                continue
                            w = kernel[d]
                            old_pix = blended[yy, x]
                            new_pix = old_pix + (w * col if not cur_on else -w * col)
                            delta_total += werr(target[yy, x] - new_pix) - werr(target[yy, x] - old_pix)
                            affected.append((yy, new_pix))
                        if delta_total < -1e-9:
                            for yy, new_pix in affected:
                                blended[yy, x] = new_pix
                            on[y, x] = not cur_on
                            changed = True
                    if changed:
                        any_flip_changed = True
                    if not changed:
                        break

                if not swaps:
                    break

                # -- swap sweep: toggle an adjacent (y, y+1) pair together.
                # Catches improvements a single-pixel flip can't reach (each
                # flip alone makes things worse, but the pair together helps). --
                order2 = swap_rng.permutation(HEIGHT - 1)
                swapped = False
                for y in order2:
                    y = int(y)
                    on_y, on_y1 = on[y, x], on[y + 1, x]
                    if on_y == on_y1:
                        continue
                    col_y, col_y1 = colour[y], colour[y + 1]
                    delta_map = {}
                    for d in offsets:
                        yy = y + d
                        if 0 <= yy < HEIGHT:
                            dv = kernel[d] * (col_y if not on_y else -col_y)
                            delta_map[yy] = delta_map.get(yy, 0.0) + dv
                    for d in offsets:
                        yy = (y + 1) + d
                        if 0 <= yy < HEIGHT:
                            dv = kernel[d] * (col_y1 if not on_y1 else -col_y1)
                            delta_map[yy] = delta_map.get(yy, 0.0) + dv
                    delta_total = 0.0
                    new_vals = {}
                    for yy, dv in delta_map.items():
                        old_pix = blended[yy, x]
                        new_pix = old_pix + dv
                        delta_total += werr(target[yy, x] - new_pix) - werr(target[yy, x] - old_pix)
                        new_vals[yy] = new_pix
                    if delta_total < -1e-9:
                        for yy, new_pix in new_vals.items():
                            blended[yy, x] = new_pix
                        on[y, x] = not on_y
                        on[y + 1, x] = not on_y1
                        swapped = True
                if not swapped and not any_flip_changed:
                    break

        # ---------------- phase 2: colour refinement per scanline -------
        # (weighting cancels out of the per-channel normal equations here --
        # each channel is solved independently either way -- so this closed
        # form is unchanged; only the palette snap below needs the weights.)
        for y in range(HEIGHT):
            xs_on = np.nonzero(on[y])[0]
            if len(xs_on) == 0:
                continue
            correction = np.zeros(3, dtype=np.float64)
            for d in offsets:
                yy = y + d
                if yy < 0 or yy >= HEIGHT:
                    continue
                w = kernel[d]
                if w == 0:
                    continue
                s_d = np.sum(target[yy, xs_on] - blended[yy, xs_on], axis=0)
                correction += w * s_d
            correction /= (len(xs_on) * sumsq_w)
            new_continuous = colour[y] + correction
            neighbours = []
            if y > 0:
                neighbours.append(colour[y - 1])
            if y < HEIGHT - 1:
                neighbours.append(colour[y + 1])
            new_idx = nearest_palette_index_smooth(new_continuous, weights, neighbours, smoothness)
            if new_idx != color_idx[y]:
                new_colour = PALETTE_RGB[new_idx]
                delta_c = new_colour - colour[y]
                for d in offsets:
                    yy = y + d
                    if yy < 0 or yy >= HEIGHT:
                        continue
                    w = kernel[d]
                    if w == 0:
                        continue
                    blended[yy, xs_on] += w * delta_c
                colour[y] = new_colour
                color_idx[y] = new_idx

        if verbose:
            print(f"{tag}round {rnd}: weighted SSE = {total_werror():.1f}")

    rmse = (total_rgb_sse() / (HEIGHT * WIDTH * 3)) ** 0.5
    return color_idx, on, blended, rmse, total_werror()


def solve_best_of(target, seeds=3, tag="", **kwargs):
    """Run solve() from several random seeds (DBS is a local search, so
    different sweep orders land in different local optima) and keep the
    lowest-weighted-error result."""
    verbose = kwargs.pop("verbose", True)
    best = None
    for s in range(seeds):
        label = f"{tag}seed {s+1}/{seeds}" if seeds > 1 else tag.rstrip()
        result = solve(target, seed=s, verbose=verbose, label=label, **kwargs)
        werror = result[-1]
        if best is None or werror < best[-1]:
            best = result
            best_seed = s
    if verbose and seeds > 1:
        print(f"{tag}best result: seed {best_seed + 1}/{seeds} "
              f"(weighted SSE = {best[-1]:.1f}, RMS = {best[-2]:.2f})")
    return best


def blended_from(color_idx, on, sigma=1.0, radius=3):
    """Rebuild the within-frame vertically-blended display from a
    committed (colour_idx, on) pair -- used to score a frame on its own,
    and to compute the residual a second correcting frame should target."""
    colour = PALETTE_RGB[color_idx]
    kernel = make_kernel(sigma, radius)
    offsets = sorted(kernel)
    b = np.zeros((HEIGHT, WIDTH, 3), dtype=np.float64)
    for d in offsets:
        w = kernel[d]
        if w == 0:
            continue
        y0, y1 = max(0, -d), min(HEIGHT, HEIGHT - d)
        src = on[y0 + d:y1 + d, :, None] * colour[y0 + d:y1 + d, None, :]
        b[y0:y1] += w * src
    return b


def solve_n_frame(target, n_frames=2, seeds=3, sigma=1.0, radius=3, **kwargs):
    """Solve n_frames frames that, cycled (frame0, frame1, ..., frame(n-1),
    frame0, ...) one per video frame, temporally average (via persistence
    of vision, equal 1/n weight each) to a better match of the target than
    any single frame alone.

    Each frame is solved with the exact same machinery as the single-frame
    case (solve_best_of() doesn't know or care whether its "target" is a
    real image or a residual) -- only what target array it's handed
    changes, frame by frame:

    We need sum(blended_i for i in 0..n-1) == n*target once all n frames
    are done. Process frames in order, and at each step split whatever
    total is still owed evenly across however many frames are left to
    contribute it:

        remaining = n * target                  # total still owed
        for i in 0..n-1:
            frames_left = n - i
            this_target = remaining / frames_left     # this frame's fair share
            solve frame i to fit this_target -> blended_i
            remaining -= blended_i                     # carry the shortfall on

    Frame 0's fair share is (n*target)/n == target exactly, so it's just
    the ordinary single-frame fit (and its own RMS-vs-target is a fair
    "what if we'd stopped after 1 frame" baseline). Every subsequent frame
    picks up whatever the frames before it under- or over-shot, divided
    across the frames still left to help fix it -- so frame n-1 (the last)
    gets no further help and must absorb 100% of whatever's still wrong.
    This is exactly the n=2 case when n=2.
    """
    if n_frames < 1:
        raise ValueError("n_frames must be >= 1")
    verbose = kwargs.get("verbose", True)

    remaining = n_frames * target
    color_idxs, ons, blendeds = [], [], []
    rmse_first = None
    for i in range(n_frames):
        frames_left = n_frames - i
        this_target = remaining / frames_left
        result = solve_best_of(this_target, seeds=seeds, sigma=sigma, radius=radius,
                                tag=f"[frame{i}] ", **kwargs)
        color_idx_i, on_i, blended_i, rmse_i, werr_i = result
        color_idxs.append(color_idx_i)
        ons.append(on_i)
        blendeds.append(blended_i)
        if i == 0:
            rmse_first = rmse_i
        remaining = remaining - blended_i

        if verbose:
            partial_avg = sum(blendeds) / (i + 1)
            partial_rmse = (float(np.sum((target - partial_avg) ** 2)) / (HEIGHT * WIDTH * 3)) ** 0.5
            print(f"after {i+1}/{n_frames} frame(s): running-average RMS = {partial_rmse:.2f}")

    averaged = sum(blendeds) / n_frames
    final_rgb_sse = float(np.sum((target - averaged) ** 2))
    final_rmse = (final_rgb_sse / (HEIGHT * WIDTH * 3)) ** 0.5

    if verbose:
        print(f"1-frame RMS would have been: {rmse_first:.2f}")
        print(f"{n_frames}-frame alternating RMS (unweighted RGB): {final_rmse:.2f}")

    return {
        "color_idxs": color_idxs, "ons": ons, "blendeds": blendeds,
        "rmse_first": rmse_first, "averaged": averaged, "final_rmse": final_rmse,
    }


def pack_bits(bit_row):
    out = bytearray(BYTES_PER_ROW)
    for col, bit in enumerate(bit_row):
        if bit:
            byte_i = col // 8
            bit_i = 7 - (col % 8)
            out[byte_i] |= (1 << bit_i)
    return bytes(out)


def write_c_files(prefix, color_idx, on):
    header_path = f"{prefix}.h"
    source_path = f"{prefix}.c"
    guard = prefix.upper().replace("-", "_").replace(" ", "_").split("/")[-1] + "_H"

    with open(header_path, "w") as f:
        f.write(f"""#ifndef {guard}
#define {guard}

#include <stdint.h>

/* Single-frame vertically-blended scanline data.
 * Generated by atari_scanline_blend.py -- do not hand-edit.
 *
 * Screen is SCREEN_WIDTH x SCREEN_HEIGHT ({WIDTH}x{HEIGHT}). Each ScanLine
 * holds one Atari colour byte (COLUP0/COLUP1/COLUBK register format:
 * bits 7-4 = hue, bits 3-1 = luminance, bit 0 = 0) and a SCREEN_WIDTH-bit
 * on/off bitmap (packed MSB-first, column 0 = bit 7 of bits[0]) for that single
 * scanline. No cross-frame rolling is used -- this is a single static
 * frame; the vertical colour blending it was optimised against is purely
 * a property of the viewer/display (composite bleed / scanline
 * proximity), not of anything your display code needs to do frame to
 * frame.
 */

#define SCREEN_WIDTH  {WIDTH}
#define SCREEN_HEIGHT {HEIGHT}
#define SCREEN_BYTES_PER_ROW {BYTES_PER_ROW}

typedef struct {{
    uint8_t colour;
    uint8_t bits[SCREEN_BYTES_PER_ROW];
}} ScanLine;

extern const ScanLine screen[SCREEN_HEIGHT];

#endif /* {guard} */
""")

    with open(source_path, "w") as f:
        base = prefix.split("/")[-1]
        f.write(f'#include "{base}.h"\n\n')
        f.write("const ScanLine screen[SCREEN_HEIGHT] = {\n")
        for y in range(HEIGHT):
            bits = pack_bits(on[y])
            bits_fmt = ", ".join(f"0x{b:02X}" for b in bits)
            f.write(f"    {{ 0x{PALETTE_BYTES[color_idx[y]]:02X}, {{{bits_fmt}}} }}, /* row {y} */\n")
        f.write("};\n")

    return header_path, source_path


def write_c_files_n_frame(prefix, color_idxs, ons):
    n_frames = len(color_idxs)
    header_path = f"{prefix}.h"
    source_path = f"{prefix}.c"
    guard = prefix.upper().replace("-", "_").replace(" ", "_").split("/")[-1] + "_H"

    extern_lines = "\n".join(f"extern const ScanLine screen_frame{i}[SCREEN_HEIGHT];"
                              for i in range(n_frames))
    array_names = ", ".join(f"screen_frame{i}" for i in range(n_frames))

    with open(header_path, "w") as f:
        f.write(f"""#ifndef {guard}
#define {guard}

#include <stdint.h>

/* {n_frames}-frame vertically-blended scanline data.
 * Generated by atari_scanline_blend.py -- do not hand-edit.
 *
 * Screen is SCREEN_WIDTH x SCREEN_HEIGHT ({WIDTH}x{HEIGHT}). Each ScanLine
 * holds one Atari colour byte (COLUP0/COLUP1/COLUBK register format:
 * bits 7-4 = hue, bits 3-1 = luminance, bit 0 = 0) and a SCREEN_WIDTH-bit
 * on/off bitmap (packed MSB-first, column 0 = bit 7 of bits[0]).
 *
 * {n_frames} full screens are supplied ({array_names}). Cycle through them
 * one per video frame, in order, then wrap back to frame 0 and repeat
 * forever ({', '.join(f'frame{i}' for i in range(n_frames))}, frame0, ...).
 * Only frame0 was solved to look reasonable on its own; every later frame
 * was solved to correct whatever residual error the frames before it left
 * behind, so that the 1/{n_frames}-weighted temporal average the eye
 * perceives from the full cycle (combined with the usual vertical
 * scanline blending within each individual frame) lands closer to the
 * source image than fewer frames could manage. Do not display any frame
 * after frame0 on its own expecting it to look like the picture -- it
 * won't; each is a correction term, not a picture in its own right.
 */

#define SCREEN_WIDTH  {WIDTH}
#define SCREEN_HEIGHT {HEIGHT}
#define SCREEN_BYTES_PER_ROW {BYTES_PER_ROW}
#define SCREEN_NUM_FRAMES {n_frames}

typedef struct {{
    uint8_t colour;
    uint8_t bits[SCREEN_BYTES_PER_ROW];
}} ScanLine;

{extern_lines}

#endif /* {guard} */
""")

    with open(source_path, "w") as f:
        base = prefix.split("/")[-1]
        f.write(f'#include "{base}.h"\n\n')
        for i in range(n_frames):
            frame_name = f"screen_frame{i}"
            color_idx, on = color_idxs[i], ons[i]
            f.write(f"const ScanLine {frame_name}[SCREEN_HEIGHT] = {{\n")
            for y in range(HEIGHT):
                bits = pack_bits(on[y])
                bits_fmt = ", ".join(f"0x{b:02X}" for b in bits)
                f.write(f"    {{ 0x{PALETTE_BYTES[color_idx[y]]:02X}, {{{bits_fmt}}} }}, /* row {y} */\n")
            f.write("};\n\n")

    return header_path, source_path


def save_preview(arr, path, upscale=None, width_stretch=None):
    """Render at `upscale` in both axes, then stretch width by an extra
    `width_stretch` factor to correct for the Atari's non-square pixels:
    each playfield pixel is several TV colour-clocks wide, so a naive 1:1
    pixel render looks far too tall/narrow compared to what actually
    appears on screen.

    upscale/width_stretch default to the CURRENT module-level UPSCALE/
    WIDTH_STRETCH, looked up at call time -- NOT bound as literal default
    argument values, since those are only evaluated once at module load
    and would silently go stale after main() retargets WIDTH_STRETCH for
    a custom --width."""
    if upscale is None:
        upscale = UPSCALE
    if width_stretch is None:
        width_stretch = WIDTH_STRETCH
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), mode="RGB")
    w = int(round(arr.shape[1] * upscale * width_stretch))
    h = int(round(arr.shape[0] * upscale))
    img = img.resize((w, h), Image.NEAREST)
    img.save(path)


def main():
    ap = argparse.ArgumentParser(description="Convert an image into blended Atari 2600 scanlines.")
    ap.add_argument("image")
    ap.add_argument("-o", "--output-prefix", default=None)
    ap.add_argument("--width", type=int, default=48,
                    help="screen width in pixels/columns (default 48). Each "
                         "column is solved independently, so this mainly "
                         "affects runtime and horizontal detail.")
    ap.add_argument("--height", type=int, default=198,
                    help="screen height in scanlines (default 198, the Atari's "
                         "visible NTSC scanline count -- only change this if "
                         "you're targeting a non-standard display region.")
    ap.add_argument("--sigma", type=float, default=1.0, help="Gaussian blend falloff (default 1.0)")
    ap.add_argument("--radius", type=int, default=3, help="max scanline distance blended (default 3)")
    ap.add_argument("--rounds", type=int, default=10, help="outer optimisation rounds (default 10)")
    ap.add_argument("--sweeps", type=int, default=3, help="DBS sweeps per column per round (default 3)")
    ap.add_argument("--metric", choices=["luma", "rgb"], default="luma",
                    help="error metric driving the optimiser (default luma-weighted)")
    ap.add_argument("--seeds", type=int, default=3,
                    help="random restarts, keep the best (default 3)")
    ap.add_argument("--smoothness", type=float, default=0.01,
                    help="penalty (in squared-RGB-distance units) for a scanline's "
                         "colour differing from its already-committed neighbours; "
                         "stops smooth photographic gradients turning into jittery "
                         "hue-shifting noise. Calibrated small on purpose -- this "
                         "operates in the same units as full-scale RGB squared "
                         "distance (up to ~195000), so values above ~0.05-0.1 "
                         "overpower the actual fit and collapse everything toward "
                         "flat colour. There's a real trade-off here: it costs "
                         "~1-3%% more error on images that already had sharp, "
                         "justified colour transitions (tested on reef/parrot) "
                         "in exchange for meaningfully less jitter on smooth "
                         "photographic gradients (tested on a portrait). Default "
                         "0.01 is a conservative pick; 0 disables it entirely.")
    ap.add_argument("--swaps", action="store_true",
                    help="enable adjacent-pixel swap moves in addition to flips "
                         "(off by default: tested extensively and it consistently "
                         "converges to WORSE results here, not better -- see comments "
                         "in solve(). Left in for experimentation, not recommended.")
    ap.add_argument("--frames", type=int, default=1,
                    help="1 (default): a single static frame. N>1: also solve "
                         "N-1 further 'correcting' frames that, cycled one per "
                         "video frame with the first, temporally average (via "
                         "persistence of vision) to a closer match of the source "
                         "image than fewer frames could manage. Roughly N times "
                         "the runtime (solves N times).")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    if args.width < 1 or args.height < 1:
        ap.error("--width and --height must be positive")

    # Retarget the whole module at the requested screen size. Every function
    # below reads WIDTH/HEIGHT/BYTES_PER_ROW/WIDTH_STRETCH as globals looked
    # up at call time, so this must happen before load_target/solve/etc run.
    global WIDTH, HEIGHT, BYTES_PER_ROW, WIDTH_STRETCH
    WIDTH = args.width
    HEIGHT = args.height
    BYTES_PER_ROW = (WIDTH + 7) // 8
    WIDTH_STRETCH = 4 * (40 / WIDTH)

    prefix = args.output_prefix
    if prefix is None:
        base = args.image.rsplit("/", 1)[-1].rsplit(".", 1)[0]
        prefix = base + "_scanline"

    target = load_target(args.image)
    solve_kwargs = dict(seeds=args.seeds, sigma=args.sigma, radius=args.radius,
                         rounds=args.rounds, sweeps=args.sweeps, metric=args.metric,
                         swaps=args.swaps, smoothness=args.smoothness, verbose=not args.quiet)

    if args.frames == 1:
        color_idx, on, blended, rmse, werror = solve_best_of(target, **solve_kwargs)

        header_path, source_path = write_c_files(prefix, color_idx, on)

        raw = np.zeros((HEIGHT, WIDTH, 3), dtype=np.float64)
        for y in range(HEIGHT):
            raw[y][on[y]] = PALETTE_RGB[color_idx[y]]

        recon_path = f"{prefix}_reconstructed.png"
        raw_path = f"{prefix}_reconstructed_raw.png"
        save_preview(blended, recon_path)
        save_preview(raw, raw_path)

        print(f"Wrote {header_path}")
        print(f"Wrote {source_path}")
        print(f"Wrote {recon_path}  (blended -- what the optimiser targets)")
        print(f"Wrote {raw_path}  (raw unblended TIA pixels)")
        print(f"Final RMS colour error (unweighted RGB, 0-255 scale): {rmse:.2f}")
        print(f"Final weighted ({args.metric}) SSE: {werror:.1f}")
    else:
        result = solve_n_frame(target, n_frames=args.frames, **solve_kwargs)
        color_idxs, ons = result["color_idxs"], result["ons"]

        header_path, source_path = write_c_files_n_frame(prefix, color_idxs, ons)

        recon_path = f"{prefix}_reconstructed.png"
        save_preview(result["averaged"], recon_path)
        print(f"Wrote {header_path}")
        print(f"Wrote {source_path}")
        print(f"Wrote {recon_path}  (the {args.frames}-frame temporally-averaged perceived result)")

        for i in range(args.frames):
            raw_i = np.zeros((HEIGHT, WIDTH, 3), dtype=np.float64)
            for y in range(HEIGHT):
                raw_i[y][ons[i][y]] = PALETTE_RGB[color_idxs[i][y]]
            raw_path = f"{prefix}_frame{i}_raw.png"
            save_preview(raw_i, raw_path)
            note = "" if i == 0 else "  (correction layer, not a picture on its own)"
            print(f"Wrote {raw_path}  (raw unblended TIA pixels, frame {i}){note}")

        print(f"1-frame RMS would have been: {result['rmse_first']:.2f}")
        print(f"{args.frames}-frame alternating RMS (unweighted RGB, 0-255 scale): {result['final_rmse']:.2f}")


if __name__ == "__main__":
    main()