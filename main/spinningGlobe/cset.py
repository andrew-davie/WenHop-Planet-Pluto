#!/usr/bin/env python3
# Authored with Claude (Anthropic), model claude-sonnet-4-6, via Cowork.
"""
img2tiles.py

Convert an arbitrary input image into a tiled "trixel" characterset + map,
emitted as a C source file, for a display mode where:

  - a "trixel" is 1 pixel wide x 3 scanlines deep. Each of the 3 scanlines
    carries one bit of an RGB triad (scanline 0 = R, scanline 1 = G,
    scanline 2 = B), so a trixel encodes one of 8 colours (the 8 corners
    of the RGB colour cube: black, red, green, blue, yellow, magenta,
    cyan, white).
  - a character is TRIXEL_W pixels wide x 10 trixels deep -> TRIXEL_W x 30
    scanlines.  TRIXEL_W defaults to 5 and is set via --trixel-width.
  - a character is stored as 10 trixel-rows, each row = 3 bytes (R,G,B).
    Each byte uses TRIXEL_W of its bits (bits TRIXEL_W-1..0), one bit per
    column, bit(TRIXEL_W-1) = leftmost column (col 0) .. bit0 = rightmost.
    -> 10 * 3 = 30 bytes per character.
  - characters are numbered 0..255 (max charset size 256). The charset is
    built unbounded first -- seeded with the 8 solid colours, then one exact
    character per unique cell pattern found in the image, so nothing is
    approximated yet -- then reduced down to the requested budget by
    repeatedly finding the globally closest matching pair of non-locked
    characters and replacing both with the dominant (higher-usage) one,
    until the budget is met. The 8 solid-colour seed characters are locked
    and never participate in any merge.
  - the map is wrap-around: the rightmost 10 CHARACTER columns are a
    copy of the leftmost 10 character columns of the same row, so a
    horizontally-scrolling display can wrap seamlessly. The map's
    declared width therefore equals (input width in characters) + 10.
  - a companion PNG is also written, reconstructed entirely from the
    emitted charset + map data (i.e. it shows exactly what the C data
    decodes to -- post-quantization, post-charset-budget lossy matching,
    including the wrap-buffer seam), as a sanity check on the output.

Usage:
    python3 img2tiles.py <image> <char_width> <char_height>
                          [--trixel-width N] [--max-chars N] [--output FILE.c]
                          [--recon-scale N] [--no-recon]
                          [--adaptive-palette]
"""

import argparse
import os
import sys

import numpy as np
from PIL import Image

TRIXEL_W = 5     # pixels wide per character — default; overridden by --trixel-width
TRIXEL_H = 10    # trixels deep per character — default; overridden by --trixel-height
SCANLINES_PER_TRIXEL = 3
WRAP_COLUMNS = 10  # extra character-columns appended for wrap-around (default; overridden by --wrap-columns)
NUM_LOCKED = 8    # the 8 solid-colour seed characters are always locked

# ---------------------------------------------------------------------------
# Gamma / sRGB helpers
# ---------------------------------------------------------------------------
# Source images are stored in sRGB (gamma ~2.2).  k-means and colour-distance
# calculations should be done in LINEAR light so that cluster centres and
# palette averages are physically correct.  Palette values written to the C
# file are converted back to sRGB so the display hardware sees the right values.

def _srgb_to_linear(arr: np.ndarray) -> np.ndarray:
    """sRGB [0,255] → linear light [0,255] (float64 out)."""
    a = np.asarray(arr, dtype=np.float64) / 255.0
    lin = np.where(a <= 0.04045, a / 12.92, ((a + 0.055) / 1.055) ** 2.4)
    return lin * 255.0

def _linear_to_srgb(arr: np.ndarray) -> np.ndarray:
    """Linear light [0,255] → sRGB [0,255] (float64 out)."""
    a = np.clip(np.asarray(arr, dtype=np.float64) / 255.0, 0.0, 1.0)
    srgb = np.where(a <= 0.0031308, a * 12.92,
                    1.055 * np.power(a, 1.0 / 2.4) - 0.055)
    return srgb * 255.0

# ---------------------------------------------------------------------------

# Bayer 4x4 ordered-dither matrix, normalised to [0, 1).  Used by the dark-colour
# recovery pass (fixed RGB palette only) to deterministically scatter dominant-channel
# bits into regions that would otherwise collapse to solid black.
_BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6],
                     [3, 11, 1,  9], [15, 7, 13, 5]], dtype=np.float32) / 16.0

# The 8 RGB-cube corner colours (fixed palette), index = bit0:R bit1:G bit2:B
CUBE_RGB = np.array(
    [(255 if (i & 1) else 0,
      255 if (i & 2) else 0,
      255 if (i & 4) else 0) for i in range(8)],
    dtype=np.uint8,
)

# Precompute the 8x8 squared-distance table for the fixed RGB palette.
# reduce_charset accepts a dist_table parameter to override this for adaptive palettes.
_DIFF = CUBE_RGB.astype(np.int16)[:, None, :] - CUBE_RGB.astype(np.int16)[None, :, :]
DIST8 = (_DIFF.astype(np.int32) ** 2).sum(axis=2)  # shape (8,8)

# Symbolic names for the 8 binary combination slots (for C comments / diagnostics).
_PALETTE_SLOT_NAMES = [
    "origin", "+P1", "+P2", "+P1+P2", "+P3", "+P1+P3", "+P2+P3", "+P1+P2+P3",
]

# ---------------------------------------------------------------------------
# Atari 2600 NTSC TIA palette
# 128 colours; TIA register values are even bytes 0x00..0xFE.
# Entry i corresponds to register value (i * 2).
# RGB values are the widely-used NTSC approximation (Z26/Trebor-style).
# ---------------------------------------------------------------------------
_NTSC_RGB = np.array([
    # Hue 0 – greyscale
    (  0,   0,   0), ( 64,  64,  64), (108, 108, 108), (144, 144, 144),
    (176, 176, 176), (200, 200, 200), (220, 220, 220), (236, 236, 236),
    # Hue 1 – yellow-green / gold
    ( 68,  68,   0), (100, 100,  16), (132, 132,  36), (160, 160,  52),
    (184, 184,  64), (208, 208,  80), (232, 232,  92), (252, 252, 104),
    # Hue 2 – orange-brown
    (112,  40,   0), (132,  68,  20), (152,  92,  40), (172, 120,  64),
    (188, 140,  76), (204, 160,  92), (220, 180, 104), (236, 200, 120),
    # Hue 3 – red-orange
    (132,  24,   0), (152,  52,  24), (172,  80,  48), (192, 104,  72),
    (208, 128,  92), (224, 148, 112), (236, 168, 128), (252, 188, 148),
    # Hue 4 – red
    (136,   0,   0), (156,  32,  32), (176,  60,  60), (192,  88,  88),
    (208, 112, 112), (224, 136, 136), (236, 160, 160), (252, 180, 180),
    # Hue 5 – purple-red / magenta
    (120,   0,  92), (140,  32, 116), (160,  60, 136), (176,  88, 156),
    (192, 112, 176), (208, 132, 192), (220, 156, 208), (236, 176, 224),
    # Hue 6 – blue-purple
    ( 72,   0, 120), ( 96,  32, 144), (120,  64, 168), (140,  92, 188),
    (160, 116, 204), (180, 140, 220), (196, 164, 232), (212, 188, 248),
    # Hue 7 – blue-violet
    ( 20,   0, 132), ( 48,  32, 152), ( 76,  60, 172), (104,  88, 192),
    (124, 112, 208), (148, 136, 224), (168, 160, 236), (188, 184, 248),
    # Hue 8 – blue
    (  0,   0, 136), ( 28,  32, 156), ( 56,  64, 176), ( 80,  96, 192),
    (104, 120, 208), (124, 144, 224), (144, 168, 236), (164, 188, 248),
    # Hue 9 – blue-cyan
    (  0,  24, 124), ( 28,  56, 144), ( 56,  84, 168), ( 80, 116, 188),
    (104, 136, 204), (124, 156, 220), (144, 180, 236), (164, 200, 248),
    # Hue A – cyan-blue / teal
    (  0,  44,  92), ( 28,  76, 120), ( 56, 104, 144), ( 80, 132, 172),
    (104, 156, 192), (124, 180, 212), (144, 204, 232), (164, 224, 252),
    # Hue B – green-cyan
    (  0,  60,  44), ( 28,  92,  72), ( 56, 124, 100), ( 92, 156, 128),
    (116, 180, 148), (140, 204, 168), (160, 224, 188), (180, 244, 208),
    # Hue C – green
    (  0,  60,   0), ( 32,  92,  32), ( 64, 124,  64), ( 92, 156,  92),
    (116, 180, 116), (140, 208, 140), (160, 228, 160), (180, 248, 180),
    # Hue D – yellow-green
    ( 20,  56,   0), ( 52,  88,  24), ( 80, 120,  48), (108, 152,  72),
    (132, 180,  92), (156, 204, 112), (180, 228, 132), (200, 252, 152),
    # Hue E – yellow-olive
    ( 44,  48,   0), ( 76,  80,  24), (104, 112,  52), (132, 140,  76),
    (156, 168,  96), (180, 192, 120), (204, 212, 136), (224, 236, 156),
    # Hue F – orange-yellow
    ( 68,  40,   0), (100,  72,  24), (132, 104,  48), (160, 136,  72),
    (184, 160,  92), (208, 188, 112), (232, 212, 132), (252, 234, 152),
], dtype=np.uint8)  # shape (128, 3); index i → TIA register value i*2


def nearest_ntsc(rgb, exclude=()) -> int:
    """Return the TIA register byte (0x00..0xFE, even) for the nearest NTSC colour.
    TIA values in *exclude* are skipped, preventing duplicate assignments.
    TIA values 0x00–0x0E (hue 0, greyscale — no colour information) are always skipped."""
    r, g, b = int(rgb[0]), int(rgb[1]), int(rgb[2])
    diff = _NTSC_RGB.astype(np.int32) - np.array([r, g, b], dtype=np.int32)
    dists = (diff ** 2).sum(axis=1)
    for idx in np.argsort(dists):
        tia = int(idx) * 2
        # if tia < 16:          # skip hue 0 (greyscale, TIA 0x00–0x0E)
        #     continue
        if tia not in exclude:
            return tia
    return 16  # fallback: first coloured TIA value (unreachable under normal use)


def _nearest_palette_entry(palette: np.ndarray, target) -> int:
    """Return the index in palette (shape N×3) whose colour is closest to target."""
    diff = palette.astype(np.int32) - np.array(target, dtype=np.int32)
    return int((diff ** 2).sum(axis=1).argmin())


def sanitize_identifier(name: str) -> str:
    out = []
    for ch in name:
        out.append(ch if (ch.isalnum() or ch == "_") else "_")
    ident = "".join(out)
    if not ident or ident[0].isdigit():
        ident = "_" + ident
    return ident


# ---------------------------------------------------------------------------
# Adaptive palette — direct trixel-primary optimisation
# ---------------------------------------------------------------------------

def find_adaptive_palette(arr: np.ndarray) -> tuple:
    """
    Compute the best 8-colour palette for an image using k-means clustering (k=8).

    K-means places palette entries where the pixels actually are, minimising the
    mean squared quantisation error.  This substantially outperforms the previous
    PCA parallelepiped approach, which placed entries at the extreme corners of the
    data bounding box -- regions that are often sparsely populated, causing most
    pixels to map to the same 2-3 corners and losing almost all detail.

    Initialisation uses k-means++ (distance-weighted random seed selection) for
    reliable convergence to a good solution.  Clusters are sorted by perceived
    luminance so that index 0 is always the darkest entry (preserving the
    zero_content semantics used by reduce_charset).

    Parameters
    ----------
    arr : (H, W, 3) float32/uint8 image array at the target trixel resolution

    Returns
    -------
    palette : (8, 3) uint8  -- the 8 palette colours, sorted darkest to brightest
    None, None              -- origin/primaries no longer used; kept for API compat
    """
    pixels = arr.reshape(-1, 3).astype(np.float64)

    # Subsample for speed on large images (fixed seed for reproducibility).
    rng = np.random.default_rng(42)
    if pixels.shape[0] > 50_000:
        sample = pixels[rng.choice(pixels.shape[0], 50_000, replace=False)]
    else:
        sample = pixels

    k = 8

    # k-means++ initialisation: each successive centre is chosen with probability
    # proportional to its squared distance from the nearest existing centre.
    first = int(rng.integers(len(sample)))
    centers = [sample[first].copy()]
    for _ in range(k - 1):
        d2 = np.min(
            [((sample - c) ** 2).sum(axis=1) for c in centers], axis=0
        )
        d2_sum = d2.sum()
        if d2_sum <= 0:
            # Degenerate: all remaining candidates coincide with existing centres.
            centers.append(sample[int(rng.integers(len(sample)))].copy())
        else:
            probs = d2 / d2_sum
            centers.append(sample[int(rng.choice(len(sample), p=probs))].copy())

    centers = np.array(centers, dtype=np.float64)   # (8, 3)

    # Lloyd's iterations until convergence or 100 steps.
    for _ in range(100):
        diffs  = sample[:, np.newaxis, :] - centers[np.newaxis, :, :]
        labels = (diffs ** 2).sum(axis=-1).argmin(axis=-1)   # (N,)
        new_centers = np.array([
            sample[labels == j].mean(axis=0) if (labels == j).any() else centers[j]
            for j in range(k)
        ])
        if np.allclose(centers, new_centers, atol=0.5):
            break
        centers = new_centers

    # Final labels and per-cluster pixel counts (on the sample).
    diffs  = sample[:, np.newaxis, :] - centers[np.newaxis, :, :]
    labels = (diffs ** 2).sum(axis=-1).argmin(axis=-1)
    counts = np.array([(labels == j).sum() for j in range(k)], dtype=np.float64)

    # Sort clusters darkest → brightest (luminance weights per ITU-R BT.601).
    luma  = centers @ np.array([0.299, 0.587, 0.114])
    order = np.argsort(luma)
    palette_u8    = np.clip(centers[order], 0, 255).round().astype(np.uint8)
    counts_sorted = counts[order]   # pixel counts in the same luminance order

    return palette_u8, counts_sorted, None


def find_trixel_primaries(arr: np.ndarray) -> tuple:
    """
    Directly optimise 3 primary colours P1, P2, P3 such that the 8 trixel
    binary combinations minimise total quantisation error over the image.

    Unlike k-means (which finds 8 unconstrained colours), this enforces the
    trixel palette structure from the start:
        index 0 = black
        index 1 = P1               index 2 = P2               index 4 = P3
        index 3 = (P1+P2)/2        index 5 = (P1+P3)/2        index 6 = (P2+P3)/2
        index 7 = (P1+P2+P3)/3

    Algorithm:
      1. k-means(k=8) → 8 representative colour centres covering the full image.
      2. Exhaustive search over C(8,3)=56 trios to find the best P1,P2,P3 init
         (minimises coverage error over all 8 k-means centres).
      3. EM refinement with a least-squares M-step, run against the 8 k-means
         centres (equally weighted) to avoid domination by high-frequency colours
         (e.g. ~70% ocean blue washing out all land detail).

    Returns
    -------
    palette : (8, 3) uint8
    primaries : (3, 3) float64   -- P1, P2, P3 before clipping to uint8
    None                         -- kept for API compatibility
    """
    from itertools import combinations as _combos

    # Mixing weight matrix W: palette[k] = W[k] @ P  (P shape 3×3, rows=P1,P2,P3)
    W = np.array([
        [0,    0,    0   ],  # 0 – black
        [1,    0,    0   ],  # 1 – P1
        [0,    1,    0   ],  # 2 – P2
        [0.5,  0.5,  0   ],  # 3 – (P1+P2)/2
        [0,    0,    1   ],  # 4 – P3
        [0.5,  0,    0.5 ],  # 5 – (P1+P3)/2
        [0,    0.5,  0.5 ],  # 6 – (P2+P3)/2
        [1/3,  1/3,  1/3 ],  # 7 – (P1+P2+P3)/3
    ], dtype=np.float64)

    # Step 1: k-means(k=8) for representative colour centres.
    km_pal, _, _ = find_adaptive_palette(arr)
    kf = km_pal.astype(np.float64)   # (8, 3) – equally weighted in step 3

    # Step 2: exhaustive search over all C(8,3)=56 trios to find best P init.
    best_err = np.inf
    best_P = None
    for trio in _combos(range(8), 3):
        a, b, c = kf[trio[0]], kf[trio[1]], kf[trio[2]]
        P_try = np.array([a, b, c], dtype=np.float64)
        combos = W @ P_try
        combos[0] = 0.0
        err = sum(((combos - kc) ** 2).sum(axis=1).min() for kc in kf)
        if err < best_err:
            best_err = err
            best_P = P_try.copy()
    P = best_P

    # Step 3: EM refinement using the 8 k-means centres as targets.
    # Running EM on raw pixels would let a dominant colour (e.g. ocean blue at
    # ~70% of pixels) pull all primaries toward it.  Using the k-means centres
    # (one representative per colour cluster, equal weight) eliminates that bias.
    for _ in range(200):
        palette_f = W @ P             # (8, 3)
        palette_f[0] = 0.0            # index 0 always black

        # E-step: assign each k-means centre to nearest palette entry.
        diffs  = kf[:, np.newaxis, :] - palette_f[np.newaxis, :, :]
        labels = (diffs ** 2).sum(axis=-1).argmin(axis=-1)   # (8,)

        # M-step: exclude centres assigned to black (don't constrain P).
        mask   = labels > 0
        if mask.sum() < 3:
            break
        W_fit = W[labels[mask]]       # (M, 3)
        s_fit = kf[mask]              # (M, 3)

        P_new = np.zeros((3, 3))
        for ch in range(3):
            sol, _, _, _ = np.linalg.lstsq(W_fit, s_fit[:, ch], rcond=None)
            P_new[:, ch] = sol
        P_new = np.clip(P_new, 0, 255)

        if np.allclose(P, P_new, atol=0.5):
            break
        P = P_new

    palette_f = W @ P
    palette_f[0] = 0.0
    palette_u8 = np.clip(np.round(palette_f), 0, 255).astype(np.uint8)
    return palette_u8, P, None


def compute_dist_table(palette: np.ndarray) -> np.ndarray:
    """Compute the 8x8 pairwise squared RGB distance table for a given palette."""
    diff = palette.astype(np.int32)[:, None, :] - palette.astype(np.int32)[None, :, :]
    return (diff ** 2).sum(axis=2)  # (8, 8)


# ---------------------------------------------------------------------------
# Quantization
# ---------------------------------------------------------------------------

def _dark_colour_recovery(arr_orig: np.ndarray, result: np.ndarray) -> np.ndarray:
    """
    Post-quantisation pass (fixed RGB palette only): pixels that snapped to black
    (index 0) but carry a clear colour tint get their dominant channel scattered in
    via Bayer ordered dithering.  This prevents dark-but-coloured regions (e.g.
    deep ocean blue, dark forest green) from collapsing entirely to solid black.

    Only the single strongest channel is recovered, and only when it is at least
    1.5x the second-strongest channel, to avoid spurious noise in neutral dark areas.
    """
    h, w = result.shape
    black_px = (result == 0)
    if not black_px.any():
        return result

    dom_ch  = arr_orig.argmax(axis=-1)
    dom_val = arr_orig[np.arange(h)[:, None], np.arange(w)[None, :], dom_ch]
    second_val = np.sort(arr_orig, axis=-1)[..., 1]

    MIN_SIGNAL = 24.0
    DOMINANCE  = 1.5
    colour_signal = (black_px
                     & (dom_val >= MIN_SIGNAL)
                     & (dom_val >= DOMINANCE * (second_val + 1.0)))
    if not colour_signal.any():
        return result

    by = np.arange(h)[:, None] % 4
    bx = np.arange(w)[None, :] % 4
    bayer_t = _BAYER4[by, bx]
    frac = dom_val / 128.0
    result = result.copy()
    for c in range(3):
        flip = colour_signal & (dom_ch == c) & (frac > bayer_t)
        result[flip] |= np.uint8(1 << c)
    return result


def quantize_to_trixel_grid(img: Image.Image, px_w: int, px_h: int,
                             dither: bool, dither_band: int = 64,
                             adaptive_palette: np.ndarray = None,
                             black_threshold: int = 30) -> np.ndarray:
    """
    Resize to (px_w, px_h) and map each pixel to a 0-7 palette colour index.

    Fixed RGB palette (adaptive_palette=None):
        Per-channel threshold at 128 (nearest of 8 RGB-cube corners).  Dithering
        uses independent per-channel Floyd-Steinberg within a configurable band
        around 128; channels outside the band snap deterministically.  A
        dark-colour recovery pass then Bayer-dithers the dominant channel into
        regions that collapsed entirely to black.

    Adaptive palette (adaptive_palette is (8,3) uint8):
        True nearest-colour lookup in RGB space against all 8 palette entries.
        No per-axis projection is used -- that approach assumed the primaries were
        orthogonal and spanned the data range, which breaks for greyscale images
        (where 2 of the 3 PCA axes are perpendicular to the data and have near-zero
        range).  Nearest-colour is O(8*N), handles any palette geometry, and gives
        the correct Voronoi assignment in all cases.  Dithering is Floyd-Steinberg
        error diffusion in full RGB space (serpentine scan).
    """
    resized = img.convert("RGB").resize((px_w, px_h), Image.LANCZOS)
    arr = np.asarray(resized, dtype=np.float64)  # (H, W, 3)
    h, w = arr.shape[:2]

    # ------------------------------------------------------------------
    # Adaptive palette path: nearest-colour + optional RGB-space FS dither
    # ------------------------------------------------------------------
    if adaptive_palette is not None:
        pal = adaptive_palette.astype(np.float64)  # (8, 3)
        # Squared distance threshold: pixels further from black than this won't
        # use palette index 0.  Computed from per-channel RMS threshold.
        black_sq_thresh = float(black_threshold ** 2) * 3.0

        if not dither:
            # Vectorised nearest-colour: argmin over 8 squared distances.
            diffs = arr[:, :, np.newaxis, :] - pal[np.newaxis, np.newaxis, :, :]
            dists = (diffs ** 2).sum(axis=-1)          # (H, W, 8)
            # Suppress black (index 0) for pixels not close to black.
            black_dist = (arr ** 2).sum(axis=-1)       # (H, W)
            dists[:, :, 0] = np.where(black_dist <= black_sq_thresh,
                                      dists[:, :, 0], np.inf)
            return dists.argmin(axis=-1).astype(np.uint8)

        # Floyd-Steinberg in RGB space (serpentine scan).
        work   = arr.copy()
        result = np.zeros((h, w), dtype=np.uint8)
        for y in range(h):
            row = work[y]
            if y % 2 == 0:
                xs, ahead = range(w), 1
            else:
                xs, ahead = range(w - 1, -1, -1), -1
            for x in xs:
                pixel = row[x]
                dists = ((pal - pixel) ** 2).sum(axis=-1)   # (8,)
                # Suppress black unless pixel is genuinely close to black.
                if (pixel ** 2).sum() > black_sq_thresh:
                    dists[0] = np.inf
                idx   = int(dists.argmin())
                result[y, x] = idx
                err = pixel - pal[idx]                       # (3,) RGB error
                nxt = x + ahead
                if 0 <= nxt < w:
                    row[nxt] += err * (7.0 / 16.0)
                if y + 1 < h:
                    prv = x - ahead
                    if 0 <= prv < w:
                        work[y + 1, prv] += err * (3.0 / 16.0)
                    work[y + 1, x]   += err * (5.0 / 16.0)
                    if 0 <= nxt < w:
                        work[y + 1, nxt] += err * (1.0 / 16.0)
        return result

    # ------------------------------------------------------------------
    # Fixed RGB palette path: per-channel threshold / FS dither
    # ------------------------------------------------------------------
    if not dither:
        r_bit = (arr[..., 0] >= 128).astype(np.uint8)
        g_bit = (arr[..., 1] >= 128).astype(np.uint8)
        b_bit = (arr[..., 2] >= 128).astype(np.uint8)
        result = r_bit | (g_bit << 1) | (b_bit << 2)
        return _dark_colour_recovery(arr.astype(np.float32), result)

    # Floyd-Steinberg dithering (serpentine scan, per-channel independent).
    low  = max(0,   128 - dither_band)
    high = min(255, 128 + dither_band)

    work = arr.copy()
    out_bits = np.zeros((h, w, 3), dtype=np.uint8)
    for y in range(h):
        row = work[y]
        xs = range(w) if (y % 2 == 0) else range(w - 1, -1, -1)
        if y % 2 == 0:
            ahead, behind = 1, -1
        else:
            ahead, behind = -1, 1
        for x in xs:
            for c in range(3):
                val = row[x, c]
                if val < low:
                    out_bits[y, x, c] = 0
                    continue
                if val >= high:
                    out_bits[y, x, c] = 1
                    continue
                val = min(255.0, max(0.0, val))
                bit = 1 if val >= 128.0 else 0
                out_bits[y, x, c] = bit
                err = val - (255.0 if bit else 0.0)
                nxt = x + ahead
                if 0 <= nxt < w:
                    row[nxt, c] += err * (7.0 / 16.0)
                if y + 1 < h:
                    nxt2 = x + behind
                    if 0 <= nxt2 < w:
                        work[y + 1, nxt2, c] += err * (3.0 / 16.0)
                    work[y + 1, x, c] += err * (5.0 / 16.0)
                    if 0 <= nxt < w:
                        work[y + 1, nxt, c] += err * (1.0 / 16.0)

    result = out_bits[..., 0] | (out_bits[..., 1] << 1) | (out_bits[..., 2] << 2)
    return _dark_colour_recovery(arr.astype(np.float32), result)


# ---------------------------------------------------------------------------
# Character encoding helpers
# ---------------------------------------------------------------------------

def cell_to_bytes30(cell: np.ndarray):
    """cell: (TRIXEL_H, TRIXEL_W) array of 0-7 colour indices -> list of 30 bytes (R,G,B per row).
    Each byte uses TRIXEL_W bits: bit(TRIXEL_W-1)=col0 (left) .. bit0=col(TRIXEL_W-1) (right)."""
    out = []
    for row in cell:
        rbyte = gbyte = bbyte = 0
        for col, val in enumerate(row):
            shift = TRIXEL_W - 1 - col  # col0 -> bit(TRIXEL_W-1) .. col(TRIXEL_W-1) -> bit0
            rbyte |= ((int(val) >> 0) & 1) << shift
            gbyte |= ((int(val) >> 1) & 1) << shift
            bbyte |= ((int(val) >> 2) & 1) << shift
        out.extend((rbyte, gbyte, bbyte))
    return out


def flat_to_bytes30(flat: np.ndarray):
    """flat: (TRIXEL_H * TRIXEL_W,) array of 0-7 colour indices -> list of 30 bytes."""
    return cell_to_bytes30(flat.reshape(TRIXEL_H, TRIXEL_W))


def bytes30_to_grid(b30):
    """Inverse of cell_to_bytes30: 30 bytes -> (TRIXEL_H, TRIXEL_W) array of 0-7 colour indices."""
    grid = np.zeros((TRIXEL_H, TRIXEL_W), dtype=np.uint8)
    for row in range(TRIXEL_H):
        rb, gb, bb = b30[row * 3], b30[row * 3 + 1], b30[row * 3 + 2]
        for col in range(TRIXEL_W):
            shift = TRIXEL_W - 1 - col  # col0 -> bit(TRIXEL_W-1) .. col(TRIXEL_W-1) -> bit0
            r = (rb >> shift) & 1
            g = (gb >> shift) & 1
            b = (bb >> shift) & 1
            grid[row, col] = r | (g << 1) | (b << 2)
    return grid


# ---------------------------------------------------------------------------
# Charset reduction
# ---------------------------------------------------------------------------

def merge_chars(grid_a: np.ndarray, weight_a: int,
                grid_b: np.ndarray, weight_b: int) -> np.ndarray:
    """
    Select the dominant (higher usage weight) character rather than averaging.
    When weights are equal, prefer grid_a (the merge initiator).
    """
    return grid_a.copy() if weight_a >= weight_b else grid_b.copy()


def reduce_charset(grids: list, weights: list, target_size: int,
                   num_locked: int = NUM_LOCKED,
                   dist_table: np.ndarray = None,
                   verbose: bool = True):
    """
    Reduce an unbounded list of unique characters down to target_size by
    repeatedly merging the globally closest matching pair of non-locked
    characters, replacing both with the dominant (higher-usage) one.

    The first num_locked characters (the 8 solid-colour seeds) are locked:
    they are never selected as a merge participant, so they always survive.

    dist_table : (8,8) int32 pairwise squared distance table between the 8
        palette colours.  Defaults to DIST8 (fixed RGB palette).  Pass the
        result of compute_dist_table(palette) when using an adaptive palette.

    Non-zero-content characters additionally exclude zero-content (all-index-0)
    characters from their nearest-neighbour candidate pool, preventing non-origin
    cells from being pulled toward the origin (darkest) palette corner.
    """
    if dist_table is None:
        dist_table = DIST8

    cell_size = TRIXEL_H * TRIXEL_W  # trixels per character cell

    n = len(grids)
    if n <= target_size:
        return list(range(n)), {}

    total_merges_needed = n - target_size
    capacity = n + total_merges_needed + 1

    grid_mat = np.zeros((capacity, cell_size), dtype=np.uint8)
    grid_mat[:n] = np.stack(grids)

    alive = np.zeros(capacity, dtype=bool)
    alive[:n] = True

    locked = np.zeros(capacity, dtype=bool)
    locked[:min(num_locked, n)] = True

    # zero_content[k]: all trixels are palette index 0 (origin/darkest corner).
    # Non-origin chars exclude these from their nn candidate pool.
    zero_content = np.zeros(capacity, dtype=bool)
    zero_content[:n] = (grid_mat[:n].sum(axis=1) == 0)

    redirect = {}
    nn_idx  = np.full(capacity, -1, dtype=np.int64)
    nn_dist = np.full(capacity, -1, dtype=np.int64)
    NEVER   = np.iinfo(np.int64).max

    # Initial nearest-neighbour computation.
    # Locked chars are valid nn candidates so that nearly-solid chars can be
    # absorbed directly into their matching solid-colour locked entry rather than
    # merging with each other and producing visible texture in flat areas.
    # Zero-content (darkest) chars are still excluded for non-zero-content chars.
    CHUNK = 200
    for start in range(0, n, CHUNK):
        chunk_idxs = np.arange(start, min(start + CHUNK, n))
        chunk_grids = grid_mat[chunk_idxs]
        dist_block = np.zeros((len(chunk_idxs), n), dtype=np.int64)
        for k in range(cell_size):
            col_vals = grid_mat[:n, k]
            row_vals = chunk_grids[:, k]
            dist_block += dist_table[row_vals[:, None], col_vals[None, :]]
        for ci, gi in enumerate(chunk_idxs):
            if locked[gi]:
                continue
            row = dist_block[ci].copy()
            row[gi] = NEVER
            if not zero_content[gi]:
                row[zero_content[:n]] = NEVER
            best_j = int(np.argmin(row))
            nn_idx[gi] = best_j
            nn_dist[gi] = int(row[best_j])

    cur_len = n
    active_count = n
    merges_done = 0

    while active_count > target_size:
        mergeable = alive[:cur_len] & ~locked[:cur_len] & (nn_dist[:cur_len] >= 0)
        masked = np.where(mergeable, nn_dist[:cur_len], NEVER)
        i = int(np.argmin(masked))
        j = int(nn_idx[i])

        # ------------------------------------------------------------------
        # Absorption into a locked char: i is redirected directly to locked j.
        # j's grid never changes; only its weight grows.  This lets nearly-solid
        # chars collapse cleanly into their matching locked solid-colour entry
        # rather than producing visible texture in otherwise-flat areas.
        # ------------------------------------------------------------------
        if locked[j]:
            weights[j] += weights[i]
            alive[i] = False
            redirect[i] = j
            active_count -= 1
            merges_done += 1
            # Chars whose nn was i need a new nn (search all alive chars).
            for k in np.nonzero(
                    alive[:cur_len] & ~locked[:cur_len] & (nn_idx[:cur_len] == i))[0]:
                cm = alive[:cur_len].copy()
                cm[k] = False
                if not zero_content[k]:
                    cm &= ~zero_content[:cur_len]
                ci = np.nonzero(cm)[0]
                if ci.size == 0:
                    nn_idx[k] = -1; nn_dist[k] = -1; continue
                pk = dist_table[grid_mat[k]]
                dk = pk[np.arange(cell_size)[None, :], grid_mat[ci]].sum(axis=1)
                p = int(np.argmin(dk))
                nn_idx[k] = ci[p]; nn_dist[k] = int(dk[p])
            if verbose and (merges_done % 200 == 0 or merges_done == total_merges_needed):
                print("  reducing charset: %d/%d merges done" % (merges_done, total_merges_needed))
            continue

        # ------------------------------------------------------------------
        # Normal merge: neither i nor j is locked.  Create new char M.
        # ------------------------------------------------------------------
        wi, wj = weights[i], weights[j]
        merged_grid = merge_chars(grid_mat[i], wi, grid_mat[j], wj)
        m = cur_len
        grid_mat[m] = merged_grid
        grids.append(merged_grid)
        weights.append(wi + wj)
        alive[i] = False
        alive[j] = False
        alive[m] = True
        locked[m] = False
        zero_content[m] = (merged_grid.sum() == 0)
        redirect[i] = m
        redirect[j] = m
        cur_len += 1
        active_count -= 1

        m_nonzero = not zero_content[m]

        # M's own nn: search all alive chars (including locked) so M can itself
        # be absorbed into a locked char if it's closest to one.
        m_cands_mask = alive[:cur_len].copy()
        m_cands_mask[m] = False
        if m_nonzero:
            m_cands_mask &= ~zero_content[:cur_len]
        m_cands_idx = np.nonzero(m_cands_mask)[0]

        # Non-locked active chars for the fixed_for_free / orphan-candidate logic.
        active_idx = np.nonzero(alive[:m] & ~locked[:m])[0]
        if m_nonzero and active_idx.size > 0:
            active_idx = active_idx[~zero_content[active_idx]]

        if m_cands_idx.size > 0:
            per_pos = dist_table[merged_grid]
            d_m_all = per_pos[np.arange(cell_size)[None, :], grid_mat[m_cands_idx]].sum(axis=1)
            best_all = int(np.argmin(d_m_all))
            nn_idx[m] = m_cands_idx[best_all]
            nn_dist[m] = int(d_m_all[best_all])

            if active_idx.size > 0:
                cand = grid_mat[active_idx]
                d_m = per_pos[np.arange(cell_size)[None, :], cand].sum(axis=1)

                cur_nn = nn_idx[active_idx]
                cur_nd = nn_dist[active_idx]
                orphan_candidate = (cur_nn == i) | (cur_nn == j)
                improved = (cur_nd < 0) | (d_m < cur_nd)

                fixed_for_free = active_idx[improved]
                nn_idx[fixed_for_free] = m
                nn_dist[fixed_for_free] = d_m[improved]

                true_orphans = active_idx[orphan_candidate & (~improved)]
                for k in true_orphans:
                    # Include locked chars in the orphan nn search.
                    others_mask = alive[:cur_len].copy()
                    others_mask[k] = False
                    if not zero_content[k]:
                        others_mask &= ~zero_content[:cur_len]
                    others_idx = np.nonzero(others_mask)[0]
                    if others_idx.size == 0:
                        nn_idx[k] = -1; nn_dist[k] = -1; continue
                    cand_k = grid_mat[others_idx]
                    per_pos_k = dist_table[grid_mat[k]]
                    d_k = per_pos_k[np.arange(cell_size)[None, :], cand_k].sum(axis=1)
                    pos = int(np.argmin(d_k))
                    nn_idx[k] = others_idx[pos]; nn_dist[k] = int(d_k[pos])
        else:
            nn_idx[m] = -1; nn_dist[m] = -1

        merges_done += 1
        if verbose and (merges_done % 200 == 0 or merges_done == total_merges_needed):
            print("  reducing charset: %d/%d merges done" % (merges_done, total_merges_needed))

    final_alive = list(np.nonzero(alive[:cur_len])[0])
    return final_alive, redirect


def _find_final(idx: int, redirect: dict) -> int:
    while idx in redirect:
        idx = redirect[idx]
    return idx


def build_reconstruction(charset_bytes, extended_map, total_width, char_height,
                         palette: np.ndarray = None):
    """Decode the emitted charset+map data back into a full-resolution RGB image."""
    if palette is None:
        palette = CUBE_RGB
    decoded = [bytes30_to_grid(b) for b in charset_bytes]  # list of (TRIXEL_H, TRIXEL_W) uint8
    px_h = char_height * TRIXEL_H
    px_w = total_width * TRIXEL_W
    recon = np.zeros((px_h, px_w, 3), dtype=np.uint8)
    for ry in range(char_height):
        for rx in range(total_width):
            idx = extended_map[ry * total_width + rx]
            grid = decoded[idx]
            recon[ry * TRIXEL_H:(ry + 1) * TRIXEL_H,
                  rx * TRIXEL_W:(rx + 1) * TRIXEL_W, :] = palette[grid]
    return recon


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image", help="input image (any format Pillow can read)")
    ap.add_argument("char_width", type=int, help="map width in characters (excludes wrap buffer)")
    ap.add_argument("char_height", type=int, help="map height in characters")
    ap.add_argument("--trixel-width", type=int, default=5,
                     help="character width in pixels/bits per byte (1-8, default 5)")
    ap.add_argument("--trixel-height", type=int, default=10,
                     help="character height in trixels (1+, default 10)")
    ap.add_argument("--wrap-columns", type=int, default=10,
                     help="number of extra character-columns appended to each map row "
                          "for seamless horizontal wrap (default 10)")
    ap.add_argument("--max-chars", type=int, default=128,
                     help="maximum charset size, 8-256 (default 128)")
    ap.add_argument("--output", default=None, help="output .c filename (default: <image-stem>.c)")
    ap.add_argument("--recon-scale", type=int, default=4,
                     help="nearest-neighbour upscale factor for the reconstructed "
                          "preview PNG (default 4, use 1 for native res)")
    ap.add_argument("--no-recon", action="store_true",
                     help="skip writing the reconstructed preview PNG")
    ap.add_argument("--brightness", type=float, default=1.0,
                     help="multiply all pixel values by this factor before processing "
                          "(default 1.0; try 1.3–1.8 to lift a dim result). "
                          "Applied to the image before k-means and quantization, so "
                          "the palette and C output both reflect the adjusted colours.")
    ap.add_argument("--no-dither", action="store_true",
                     help="use nearest-colour instead of dithering "
                          "(dithering is on by default)")
    ap.add_argument("--dither-band", type=int, default=64,
                     help="half-width of the ambiguous zone around 128 in which a "
                          "channel is dithered (default 64, range 64-192). "
                          "0 = same as --no-dither; 128 = dither every channel everywhere")
    ap.add_argument("--adaptive-palette", action="store_true",
                     help="derive 3 primary colours from the source image via PCA and "
                          "use the resulting 8-combination palette instead of the fixed "
                          "RGB cube. Improves colour accuracy for images whose dominant "
                          "colours are not aligned with the R/G/B axes (e.g. seascapes, "
                          "earth tones). Emits a prefix_palette[8][3] table in the C output.")
    ap.add_argument("--primary-spread", type=float, default=0.0,
                     help="adaptive palette only: push P1 (dark primary) toward black "
                          "and P3 (bright primary) toward white before NTSC snapping, "
                          "widening the palette gamut. 0.0 = no spread (default), "
                          "1.0 = full push to black/white. Try 0.2–0.5.")
    ap.add_argument("--black-threshold", type=int, default=30,
                     help="adaptive palette only: max per-channel RMS distance to black "
                          "for palette index 0 to be used. Pixels further from black than "
                          "this threshold are mapped to the nearest non-black entry instead. "
                          "Lower = stricter (fewer pixels use black). Default: 30.")
    ap.add_argument("--near-enough", type=int, default=0,
                     help="during Phase 1 charset build, reuse an existing character if "
                          "it differs from the new cell by at most this many trixels "
                          "(0 = exact match only, default). The closest match within the "
                          "threshold is used. Reduces charset size before Phase 2 merging "
                          "at the cost of some extra approximation error.")
    args = ap.parse_args()

    # Apply --trixel-width / --trixel-height to module-level globals used by all helpers.
    global TRIXEL_W, TRIXEL_H, WRAP_COLUMNS
    TRIXEL_W = args.trixel_width
    TRIXEL_H = args.trixel_height
    WRAP_COLUMNS = args.wrap_columns

    if args.char_width <= 0 or args.char_height <= 0:
        sys.exit("error: char_width and char_height must be positive")
    if not (1 <= TRIXEL_W <= 8):
        sys.exit("error: --trixel-width must be between 1 and 8")
    if TRIXEL_H < 1:
        sys.exit("error: --trixel-height must be at least 1")
    if WRAP_COLUMNS < 0:
        sys.exit("error: --wrap-columns must be >= 0")
    if not (NUM_LOCKED <= args.max_chars <= 256):
        sys.exit("error: --max-chars must be between %d and 256" % NUM_LOCKED)
    if args.recon_scale <= 0:
        sys.exit("error: --recon-scale must be a positive integer")

    total_width = args.char_width + WRAP_COLUMNS
    if total_width > 255 or args.char_height > 255:
        sys.exit("error: map width (%d, includes +%d wrap buffer) or height (%d) "
                  "exceeds 255, cannot fit in a single byte"
                  % (total_width, WRAP_COLUMNS, args.char_height))

    if not os.path.isfile(args.image):
        sys.exit("error: input file not found: %s" % args.image)

    px_w = args.char_width * TRIXEL_W
    px_h = args.char_height * TRIXEL_H

    try:
        img = Image.open(args.image)
    except Exception as e:
        sys.exit("error: could not open image: %s" % e)

    if args.brightness != 1.0:
        arr_bright = np.clip(
            np.asarray(img.convert("RGB"), dtype=np.float32) * args.brightness,
            0, 255,
        ).astype(np.uint8)
        img = Image.fromarray(arr_bright, "RGB")
        print("brightness: %.2f" % args.brightness)

    # --- adaptive palette (optional) ---
    palette    = None   # (8,3) uint8 or None → use CUBE_RGB
    dist_table = None   # (8,8) int32 or None → use DIST8

    if args.adaptive_palette:
        arr_f = np.asarray(
            img.convert("RGB").resize((px_w, px_h), Image.LANCZOS),
            dtype=np.float32,
        )
        # k-means k=3: one cluster per broad colour region (dark / mid / bright).
        pixels_f = arr_f.reshape(-1, 3).astype(np.float64)
        rng3 = np.random.default_rng(42)
        if pixels_f.shape[0] > 50_000:
            sample3 = pixels_f[rng3.choice(pixels_f.shape[0], 50_000, replace=False)]
        else:
            sample3 = pixels_f.copy()
        # k-means++ init with k=3
        first3 = int(rng3.integers(len(sample3)))
        centers3 = [sample3[first3].copy()]
        for _ in range(2):
            d2 = np.min([((sample3 - c) ** 2).sum(axis=1) for c in centers3], axis=0)
            d2s = d2.sum()
            if d2s <= 0:
                centers3.append(sample3[int(rng3.integers(len(sample3)))].copy())
            else:
                centers3.append(sample3[int(rng3.choice(len(sample3), p=d2/d2s))].copy())
        centers3 = np.array(centers3, dtype=np.float64)
        for _ in range(100):
            diffs3 = sample3[:, np.newaxis, :] - centers3[np.newaxis, :, :]
            lbl3   = (diffs3 ** 2).sum(axis=-1).argmin(axis=-1)
            nc3 = np.array([
                sample3[lbl3 == j].mean(axis=0) if (lbl3 == j).any() else centers3[j]
                for j in range(3)
            ])
            if np.allclose(centers3, nc3, atol=0.5):
                break
            centers3 = nc3
        # Sort darkest → brightest so P1=dark, P2=mid, P3=bright.
        luma3 = centers3 @ np.array([0.299, 0.587, 0.114])
        ord3  = np.argsort(luma3)
        c1, c2, c3 = centers3[ord3[0]], centers3[ord3[1]], centers3[ord3[2]]

        # Primary spread: push P1 toward black and P3 toward white.
        # P2 (mid) is left alone.
        if args.primary_spread > 0.0:
            s = args.primary_spread
            c1 = np.clip(c1 * (1.0 - s), 0, 255)
            c3 = np.clip(c3 + (255.0 - c3) * s, 0, 255)

        print("k-means k=3 primaries (before NTSC snap):")
        print("  P1 (dark)  %3d,%3d,%3d" % (int(c1[0]), int(c1[1]), int(c1[2])))
        print("  P2 (mid)   %3d,%3d,%3d" % (int(c2[0]), int(c2[1]), int(c2[2])))
        print("  P3 (bright)%3d,%3d,%3d" % (int(c3[0]), int(c3[1]), int(c3[2])))

        # Snap those 3 to distinct NTSC TIA values.
        used_ntsc: set = set()
        ntsc_1 = nearest_ntsc(c1, used_ntsc); used_ntsc.add(ntsc_1)
        ntsc_2 = nearest_ntsc(c2, used_ntsc); used_ntsc.add(ntsc_2)
        ntsc_4 = nearest_ntsc(c3, used_ntsc); used_ntsc.add(ntsc_4)

        p1 = _NTSC_RGB[ntsc_1 // 2].astype(np.float64)
        p2 = _NTSC_RGB[ntsc_2 // 2].astype(np.float64)
        p3 = _NTSC_RGB[ntsc_4 // 2].astype(np.float64)
        print("NTSC snapping:")
        print("  P1 (%3d,%3d,%3d)  ->  TIA 0x%02X  ->  NTSC (%3d,%3d,%3d)"
              % (int(c1[0]), int(c1[1]), int(c1[2]),
                 ntsc_1, int(p1[0]), int(p1[1]), int(p1[2])))
        print("  P2 (%3d,%3d,%3d)  ->  TIA 0x%02X  ->  NTSC (%3d,%3d,%3d)"
              % (int(c2[0]), int(c2[1]), int(c2[2]),
                 ntsc_2, int(p2[0]), int(p2[1]), int(p2[2])))
        print("  P3 (%3d,%3d,%3d)  ->  TIA 0x%02X  ->  NTSC (%3d,%3d,%3d)"
              % (int(c3[0]), int(c3[1]), int(c3[2]),
                 ntsc_4, int(p3[0]), int(p3[1]), int(p3[2])))

        # Build palette; blend entries averaged in sRGB space.
        palette = np.zeros((8, 3), dtype=np.float64)
        palette[0] = 0.0
        palette[1] = p1
        palette[2] = p2
        palette[3] = (p1 + p2) / 2
        palette[4] = p3
        palette[5] = (p1 + p3) / 2
        palette[6] = (p2 + p3) / 2
        palette[7] = (p1 + p2 + p3) / 3
        palette = np.clip(palette, 0, 255).round().astype(np.uint8)
        dist_table = compute_dist_table(palette)

        print("NTSC-anchored palette (TIA 0x%02X / 0x%02X / 0x%02X):" % (ntsc_1, ntsc_2, ntsc_4))
        for i, c in enumerate(palette):
            print("  [%d] %3d,%3d,%3d  (%s)" % (i, c[0], c[1], c[2], _PALETTE_SLOT_NAMES[i]))

    # --- quantize image to trixel grid ---
    trixel_grid = quantize_to_trixel_grid(
        img, px_w, px_h,
        dither=not args.no_dither,
        dither_band=args.dither_band,
        adaptive_palette=palette,
        black_threshold=args.black_threshold,
    )  # (px_h, px_w) values 0-7

    # Phase 1: unbounded charset build, seeded with the 8 solid colours so
    # any flat region gets a perfect match immediately.
    cell_size = TRIXEL_H * TRIXEL_W
    grids   = [np.full(cell_size, c, dtype=np.uint8) for c in range(8)]
    weights = [0] * 8
    seen    = {tuple(int(v) for v in g): idx for idx, g in enumerate(grids)}
    orig_map_indices = []
    near_enough = args.near_enough

    # Pre-allocate a matrix for vectorised near-enough diff counting.
    # Max rows = 8 seeds + one per cell in the image.
    if near_enough > 0:
        _ne_max = 8 + args.char_width * args.char_height
        _ne_mat = np.zeros((_ne_max, cell_size), dtype=np.uint8)
        for _i, _g in enumerate(grids):
            _ne_mat[_i] = _g
        _ne_n = 8  # number of rows currently populated

    for ry in range(args.char_height):
        for rx in range(args.char_width):
            cell = trixel_grid[ry * TRIXEL_H:(ry + 1) * TRIXEL_H,
                                rx * TRIXEL_W:(rx + 1) * TRIXEL_W]
            flat = cell.reshape(-1)
            key  = tuple(int(v) for v in flat)
            idx  = seen.get(key)
            if idx is None and near_enough > 0:
                # Vectorised: count differing trixels against all existing chars.
                diffs = (_ne_mat[:_ne_n] != flat).sum(axis=1)  # (n,)
                best  = int(diffs.argmin())
                if diffs[best] <= near_enough:
                    idx = best
            if idx is None:
                idx = len(grids)
                seen[key] = idx
                grids.append(flat.copy())
                weights.append(0)
                if near_enough > 0:
                    _ne_mat[_ne_n] = flat
                    _ne_n += 1
            weights[idx] += 1
            orig_map_indices.append(idx)

    n_unique = len(grids)
    print("unique characters before reduction: %d (incl. %d locked solid colours)"
          % (n_unique, NUM_LOCKED))

    # Phase 2: reduce to the requested budget.
    final_alive, redirect = reduce_charset(
        grids, weights, args.max_chars, dist_table=dist_table,
    )

    final_alive_sorted = sorted(final_alive)
    output_index   = {orig: i for i, orig in enumerate(final_alive_sorted)}
    charset_bytes  = [flat_to_bytes30(grids[orig]) for orig in final_alive_sorted]
    map_indices    = [output_index[_find_final(oi, redirect)] for oi in orig_map_indices]

    # Wrap-around extension.
    extended_map = []
    cw = args.char_width
    for ry in range(args.char_height):
        row  = map_indices[ry * cw:(ry + 1) * cw]
        wrap = [row[i % cw] for i in range(WRAP_COLUMNS)]
        extended_map.extend(row)
        extended_map.extend(wrap)

    # --- emit C source ---
    stem   = os.path.splitext(os.path.basename(args.image))[0]
    prefix = sanitize_identifier(stem)
    out_path = args.output or (stem + ".c")
    out_stem = os.path.splitext(out_path)[0]

    # Build the byte bit-layout comment string dynamically.
    _unused_bits = ("bits7-%d unused" % TRIXEL_W) if TRIXEL_W < 8 else "all 8 bits used"

    lines = []
    lines.append("/* Auto-generated by img2tiles.py from \"%s\" */" % os.path.basename(args.image))
    if palette is not None:
        lines.append("/* Adaptive palette: 8 colours built from 3 EM-optimised primaries (NTSC-snapped). */")
        lines.append("/* Index bits: bit0=P1, bit1=P2, bit2=P3 (P1/P2/P3 = single-bit primaries). */")
    else:
        lines.append("/* Fixed palette: 8 RGB-cube corners.  Index bits: bit0=R, bit1=G, bit2=B. */")
    lines.append("/* Character: %d pixels wide x %d trixels deep (%d scanlines). */"
                  % (TRIXEL_W, TRIXEL_H, TRIXEL_H * SCANLINES_PER_TRIXEL))
    lines.append("/* Trixel: 1 pixel wide x 3 scanlines deep, 3-bit palette index. */")
    lines.append("/* Character storage: %d trixel-rows x 3 bytes (bit0-plane, bit1-plane, bit2-plane) = %d bytes. */"
                  % (TRIXEL_H, TRIXEL_H * SCANLINES_PER_TRIXEL))
    lines.append("/* Byte bit layout: bit%d=col0(left) .. bit0=col%d(right), %s. */"
                  % (TRIXEL_W - 1, TRIXEL_W - 1, _unused_bits))
    lines.append("/* Map: byte0=width(chars, +%d wrap buffer), byte1=height(chars), then row-major indices. */" % WRAP_COLUMNS)
    lines.append("")

    # Palette table (adaptive mode only).
    if palette is not None:
        lines.append("#pragma GCC diagnostic push")
        lines.append('#pragma GCC diagnostic ignored "-Wunused-const-variable"')
        lines.append("static const unsigned char %s_palette[8][3] = {" % prefix)
        for i, c in enumerate(palette):
            lines.append("    { %3d, %3d, %3d },  /* %d: %s */"
                          % (int(c[0]), int(c[1]), int(c[2]), i, _PALETTE_SLOT_NAMES[i]))
        lines.append("};")
        lines.append("#pragma GCC diagnostic pop")
        lines.append("")

    # NTSC Atari 2600 companion palette.
    # Palette indices 1, 2, 4 are the three single-bit primary entries.
    # Map each actual palette colour to the nearest TIA register value.
    pal_src = palette if palette is not None else CUBE_RGB
    used_ntsc = set()
    ntsc_r = nearest_ntsc(pal_src[1], used_ntsc); used_ntsc.add(ntsc_r)
    ntsc_g = nearest_ntsc(pal_src[2], used_ntsc); used_ntsc.add(ntsc_g)
    ntsc_b = nearest_ntsc(pal_src[4], used_ntsc); used_ntsc.add(ntsc_b)

    lines.append("#pragma GCC diagnostic push")
    lines.append('#pragma GCC diagnostic ignored "-Wunused-const-variable"')
    lines.append("/* NTSC Atari 2600 companion palette: nearest TIA register values for")
    lines.append(" * palette primaries (indices 1, 2, 4 — the single-bit entries). */")
    lines.append("const unsigned char %s_ntsc_palette[3] = {" % prefix)
    lines.append("    0x%02X,  /* palette[1] = (%d,%d,%d) */"
                  % (ntsc_r, int(pal_src[1][0]), int(pal_src[1][1]), int(pal_src[1][2])))
    lines.append("    0x%02X,  /* palette[2] = (%d,%d,%d) */"
                  % (ntsc_g, int(pal_src[2][0]), int(pal_src[2][1]), int(pal_src[2][2])))
    lines.append("    0x%02X,  /* palette[4] = (%d,%d,%d) */"
                  % (ntsc_b, int(pal_src[4][0]), int(pal_src[4][1]), int(pal_src[4][2])))
    lines.append("};")
    lines.append("#pragma GCC diagnostic pop")
    lines.append("")

    # Emoji palette: index 0-7 (RGB cube corner) → coloured square block.
    # Index = bit0:R bit1:G bit2:B  (matches CUBE_RGB / adaptive palette slot order).
    # Cyan (6) has no dedicated square emoji; 🟦 is the closest available.
    _TRIXEL_EMOJI = ['⬛', '🟥', '🟩', '🟨', '🟦', '🟪', '🟫', '⬜']
    #                black  red  green  yel   blue   mag  cyan  white
    #                                                     (no cyan square; brown is the only unused block)

    def _trixel_row_emoji(rbyte, gbyte, bbyte):
        """Decode one trixel row (R-byte, G-byte, B-byte) → emoji art string."""
        out = []
        for col in range(TRIXEL_W):
            shift = TRIXEL_W - 1 - col
            idx = ((rbyte >> shift) & 1) | (((gbyte >> shift) & 1) << 1) | (((bbyte >> shift) & 1) << 2)
            out.append(_TRIXEL_EMOJI[idx])
        return ''.join(out)

    for i, b in enumerate(charset_bytes):
        name = "%s_char_%03d" % (prefix, i)
        total_bytes = TRIXEL_H * SCANLINES_PER_TRIXEL
        lines.append("static const unsigned char %s[%d] = {" % (name, total_bytes))
        for row in range(TRIXEL_H):
            rb, gb, bb = b[row * 3], b[row * 3 + 1], b[row * 3 + 2]
            emoji = _trixel_row_emoji(rb, gb, bb)
            lines.append("    %3d, %3d, %3d,  /* %s */" % (rb, gb, bb, emoji))
        lines.append("};")
    lines.append("")

    n_chars  = len(charset_bytes)
    map_size = 2 + len(extended_map)
    lines.append("const unsigned char * const %s_charset[%d] = {" % (prefix, n_chars))
    for i in range(n_chars):
        lines.append("    %s_char_%03d," % (prefix, i))
    lines.append("};")
    lines.append("")

    lines.append("const unsigned char %s_map[%d] = {" % (prefix, map_size))
    lines.append("    %d, %d," % (total_width, args.char_height))
    for ry in range(args.char_height):
        row = extended_map[ry * total_width:(ry + 1) * total_width]
        lines.append("    " + ", ".join(str(v) for v in row) + ",")
    lines.append("};")
    lines.append("")

    with open(out_path, "w") as f:
        f.write("\n".join(lines) + "\n")

    # --- emit header file ---
    hlines = []
    hlines.append("/* Auto-generated by spinningGlobe/cset.py from \"%s\" */" % os.path.basename(args.image))
    hlines.append("#pragma once")
    hlines.append("")
    hlines.append("#define %s_CHARSET_SIZE %d" % (prefix.upper(), n_chars))
    hlines.append("#define %s_MAP_SIZE     %d" % (prefix.upper(), map_size))
    hlines.append("#define %s_CHAR_WIDTH   %d" % (prefix.upper(), TRIXEL_W))
    hlines.append("#define %s_CHAR_HEIGHT  %d" % (prefix.upper(), TRIXEL_H))
    hlines.append("#define %s_CHAR_BYTES   %d" % (prefix.upper(), TRIXEL_H * SCANLINES_PER_TRIXEL))
    hlines.append("")
    hlines.append("/* NTSC TIA register values for primaries [1], [2], [4] */")
    hlines.append("extern const unsigned char %s_ntsc_palette[3];" % prefix)
    hlines.append("")
    hlines.append("/* Charset: %d characters, each %d bytes */"
                   % (n_chars, TRIXEL_H * SCANLINES_PER_TRIXEL))
    hlines.append("extern const unsigned char * const %s_charset[%d];" % (prefix, n_chars))
    hlines.append("")
    hlines.append("/* Map: byte[0]=width, byte[1]=height, then row-major char indices */")
    hlines.append("extern const unsigned char %s_map[%d];" % (prefix, map_size))
    hlines.append("")
    hlines.append("")
    h_path = out_stem + ".h"
    with open(h_path, "w") as f:
        f.write("\n".join(hlines) + "\n")

    # --- reconstructed preview PNG ---
    recon_path = None
    if not args.no_recon:
        recon_px  = build_reconstruction(charset_bytes, extended_map, total_width,
                                         args.char_height, palette=palette)
        recon_img = Image.fromarray(recon_px, mode="RGB")
        if args.recon_scale != 1:
            recon_img = recon_img.resize(
                (recon_img.width * args.recon_scale, recon_img.height * args.recon_scale),
                Image.NEAREST,
            )
        recon_path = out_stem + "_recon.png"
        recon_img.save(recon_path)

    # --- stats ---
    n_merges = n_unique - len(charset_bytes)
    print("input image       : %s" % args.image)
    print("palette           : %s" % ("adaptive (k-means)" if palette is not None else "fixed RGB cube"))
    print("char size (pixels) : %d wide x %d tall (%d trixels x %d scanlines)"
          % (TRIXEL_W, TRIXEL_H * SCANLINES_PER_TRIXEL, TRIXEL_H, TRIXEL_H * SCANLINES_PER_TRIXEL))
    print("map size (chars)   : %d x %d (input %d wide + %d wrap buffer)"
          % (total_width, args.char_height, args.char_width, WRAP_COLUMNS))
    print("trixel resolution  : %d x %d" % (px_w, px_h))
    print("charset budget     : %d" % args.max_chars)
    print("unique before merge: %d" % n_unique)
    print("characters used    : %d (%d pairs merged)" % (len(charset_bytes), n_merges))
    print("output file        : %s" % out_path)
    pal_src = palette if palette is not None else CUBE_RGB
    _used = set()
    _nr = nearest_ntsc(pal_src[1], _used); _used.add(_nr)
    _ng = nearest_ntsc(pal_src[2], _used); _used.add(_ng)
    _nb = nearest_ntsc(pal_src[4], _used); _used.add(_nb)
    print("NTSC palette (TIA) : [1]=0x%02X (%d,%d,%d)  [2]=0x%02X (%d,%d,%d)  [4]=0x%02X (%d,%d,%d)"
          % (_nr, int(pal_src[1][0]), int(pal_src[1][1]), int(pal_src[1][2]),
             _ng, int(pal_src[2][0]), int(pal_src[2][1]), int(pal_src[2][2]),
             _nb, int(pal_src[4][0]), int(pal_src[4][1]), int(pal_src[4][2])))
    if recon_path:
        print("reconstructed PNG  : %s (native %dx%d px, scale x%d)"
              % (recon_path, total_width * TRIXEL_W, args.char_height * TRIXEL_H, args.recon_scale))


if __name__ == "__main__":
    main()