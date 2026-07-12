#!/usr/bin/env python3
"""
atari_scanline_blend.py -- convert an image into 198 single-colour Atari
2600 scanlines (WIDTH pixels wide, currently 48) whose on-pixels, blended
vertically with their nearby scanlines, reconstruct the source image as
closely as possible in a single video frame.

Optionally (--frames N) also solves N-1 further frames that cycle with the
first, one per video frame; the eye's persistence of vision then temporally
averages all N (equal weight each) on top of the usual vertical spatial
blend, letting the set reconstruct the source more closely than fewer
frames could. --frame-scheme picks how each frame's own colours are chosen
(cumulative residual-chasing vs. a joint per-row solve); --frame-mode picks
whether frames are then relaxed against each other across the whole set
(cyclic, the default) or left as a single one-pass solve (forward). See
solve_n_frame(), solve_n_frame_joint() and solve_n_frame_cyclic().

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

          An additional SWAP move (--swaps, off by default) toggles an
          adjacent (y, y+1) pair together, catching improvements a lone
          flip can't reach. Measured worse final RMS than flips alone on
          every seed/metric tried, so it's off by default; left available
          for experimentation.
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
                                     [--smoothness auto|0.5] [--swaps]
                                     [--saturation 1.0] [--brightness 1.0]
                                     [--frames N] [--frame-scheme cumulative|joint]
                                     [--frame-mode cyclic|forward] [--outer-rounds 4]
                                     [--temporal-weight 0.0] [--max-luma-spread None]
                                     [--roll-scanlines]
                                     [--roll-band-height N] [--terse] [--quiet]

Screen is WIDTH x HEIGHT, configurable via --width/--height (default 48x198).
Source images are assumed to already be at the correct display aspect ratio
before this tool is called -- it does a plain resize, no letterboxing or
cropping. If you change --width, you likely need to adjust WIDTH_STRETCH's
underlying assumption too (see comment above WIDTH_STRETCH) to keep preview
PNGs' aspect ratio honest.
"""
import argparse
import itertools
import sys
import time
import numpy as np
from PIL import Image, ImageEnhance

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
# Atari 2600 NTSC palette: 16 hue rows x 8 luma columns (hue-major, luma-minor
# -- index == hue*8 + luma). Source: the Stella-derived "legacyNTSCfromStella"
# table, replacing an earlier luma-major/hue-minor chart.
# --------------------------------------------------------------------------
_NTSC_HEX = [
    "000000", "4a4a4a", "6f6f6f", "8e8e8e", "aaaaaa", "c0c0c0", "d6d6d6", "ececec",
    "484800", "69690f", "86861d", "a2a22a", "bbbb35", "d2d240", "e8e84a", "fcfc54",
    "7c2c00", "904811", "a26221", "b47a30", "c3903d", "d2a44a", "dfb755", "ecc860",
    "901c00", "a33915", "b55328", "c66c3a", "d5824a", "e39759", "f0aa67", "fcbc74",
    "940000", "a71a1a", "b83232", "c84848", "d65c5c", "e46f6f", "f08080", "fc9090",
    "840064", "97197a", "a8308f", "b846a2", "c659b3", "d46cc3", "e07cd2", "ec8ce0",
    "500084", "68199a", "7d30ad", "9246c0", "a459d0", "b56ce0", "c57cee", "d48cfc",
    "140090", "331aa3", "4e32b5", "6848c6", "7f5cd5", "956fe3", "a980f0", "bc90fc",
    "000094", "181aa7", "2d32b8", "4248c8", "545cd6", "656fe4", "7580f0", "8490fc",
    "001c88", "183b9d", "2d57b0", "4272c2", "548ad2", "65a0e1", "75b5ef", "84c8fc",
    "003064", "185080", "2d6d98", "4288b0", "54a0c5", "65b7d9", "75cceb", "84e0fc",
    "004030", "18624e", "2d8169", "429e82", "54b899", "65d1ae", "75e7c2", "84fcd4",
    "004400", "1a661a", "328432", "48a048", "5cba5c", "6fd26f", "80e880", "90fc90",
    "143c00", "355f18", "527e2d", "6e9c42", "87b754", "9ed065", "b4e775", "c8fc84",
    "303800", "505916", "6d762b", "88923e", "a0ab4f", "b7c25f", "ccd86e", "e0ec7c",
    "482c00", "694d14", "866a26", "a28638", "bb9f47", "d2b656", "e8cc63", "fce070",
]

def _hex_to_rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def _build_palette():
    byte_to_rgb = {}
    for hue in range(16):
        for luma in range(8):
            byte = (hue << 4) | (luma << 1)
            byte_to_rgb[byte] = _hex_to_rgb(_NTSC_HEX[hue * 8 + luma])
    bytes_sorted = sorted(byte_to_rgb)
    rgb_arr = np.array([byte_to_rgb[b] for b in bytes_sorted], dtype=np.float64)
    return bytes_sorted, rgb_arr


PALETTE_BYTES, PALETTE_RGB = _build_palette()   # 128 entries each

# (128,). TIA's raw hardware luma control field (0-7) per entry -- index %
# 8. NOT actual perceived brightness; see PALETTE_PERCEIVED_LUMA below.
PALETTE_LUMA = np.arange(128) % 8

# 128x128 matrix of squared RGB distances between every pair of palette
# entries -- the "transition cost" table for solve_colour_sequence()'s
# column-wide DP.
PALETTE_PAIRWISE_DIST2 = np.sum(
    (PALETTE_RGB[:, None, :] - PALETTE_RGB[None, :, :]) ** 2, axis=-1)

# Based on BT.601 luma (0.299/0.587/0.114), blue bumped to 0.25: BT.601's
# low blue weight assumes isolated channel error gets masked by surrounding
# detail, which doesn't hold for a whole discrete scanline colour (e.g. a
# flesh tone snapping to garish blue/purple because blue barely counted).
# 0.25 fixes that without costing overall RMS.
LUMA_WEIGHTS = np.array([0.299, 0.587, 0.25])
RGB_WEIGHTS = np.array([1.0, 1.0, 1.0])

# ITU-R BT.709 photopic coefficients -- real eye sensitivity (green >> red
# >> blue). Deliberately different from LUMA_WEIGHTS above, which is tuned
# for static hue-matching, not "how bright does this look." Use this for
# any flicker/brightness-swing measurement.
PHOTOPIC_WEIGHTS = np.array([0.2126, 0.7152, 0.0722])

# (128,). Actual perceived brightness per palette entry (PHOTOPIC_WEIGHTS
# applied to real RGB), vs. PALETTE_LUMA's raw hardware control value.
# These diverge a lot: entries sharing one PALETTE_LUMA value can differ
# 60-80+ units here depending on hue, so constraining PALETTE_LUMA doesn't
# constrain what a viewer actually sees flicker on. --max-luma-spread uses
# this array.
PALETTE_PERCEIVED_LUMA = PALETTE_RGB @ PHOTOPIC_WEIGHTS


def get_weights(metric):
    if metric == "luma":
        return LUMA_WEIGHTS
    if metric == "rgb":
        return RGB_WEIGHTS
    raise ValueError(f"unknown metric {metric!r}")



# Channel-spread (max-min, 0-255 scale) below which a colour counts as
# "achromatic" for the cross-frame temporal-stability term; see
# achromatic_factor() and the temporal_ref/temporal_weight params on
# solve_colour_sequence().
ACHROMATIC_SATURATION_THRESHOLD = 40.0

# Scale for how quickly the cross-frame temporal-stability term
# (solve_colour_sequence()'s reuse_cost/reuse_ok) backs off as reusing
# temporal_ref's exact colour costs more fit error. Opt-in (temporal_weight
# =0 by default). Tuned for donald.jpg's white-belly flicker case; not
# validated on other photographic content.
TEMPORAL_REUSE_COST_SCALE = 3000.0


# Scale multiplying --smoothness (0-1ish) into the per-row-pair transition
# cost in solve_colour_sequence(). A row's fit-cost gap between best and
# second-best candidate is ~100-300 (weighted RGB units squared); a typical
# pair of different palette entries is ~26000 apart, so this needs to be a
# small fraction of 1. Past roughly smoothness 0.25-0.5 at this scale the
# DP snaps an entire column to flat grey (no isolated run of true-colour
# rows can pay back its transition cost). Calibrated to keep the full
# --smoothness 0-1 range inside the safe regime.
SMOOTHNESS_DP_SCALE = 0.15

# Phase-2 colour refinement uses a Jacobi-style batch update (every row's
# correction computed from one shared start-of-round snapshot) rather than
# sequential Gauss-Seidel, so the DP sees internally-consistent costs
# within a round. A full-strength batch update on this coupled system can
# 2-cycle (round N's choice shifts state enough that round N+1 flips
# straight back, forever). Damping the correction before the DP sees it is
# the standard fix, breaking the 2-cycle by under-shooting each round.
PHASE2_RELAXATION = 0.5


# (128,). Per-entry channel spread (max-min) of the NTSC palette above.
PALETTE_SPREAD = PALETTE_RGB.max(axis=1) - PALETTE_RGB.min(axis=1)

# Weight on a bias term in _fit_cost_table() penalising a palette candidate
# for being less saturated than the row's target. Plain weighted-RGB
# distance regularly ranks flat grey as the closest match to a moderately-
# saturated skin tone -- not a near-tie, grey wins outright, because the
# 128 (hue, luma) points don't include a softly-saturated warm hue at every
# luma level. A viewer reads "gone grey" as a much worse defect than
# "slightly wrong saturation," so this bias corrects for it: candidates
# already at least as saturated as the target pay no penalty.
CHROMA_BIAS_SCALE = 1.0

# Ceiling on the target spread fed into the bias above. continuous_targets
# isn't always in-gamut: in the cumulative scheme, frames after the first
# fit a RESIDUAL, and that arithmetic can produce per-channel spread well
# past what the palette can reach. Uncapped, the bias would then chase the
# most saturated palette entry regardless of hue. Capping at a real photo's
# typical on-pixel spread keeps the bias doing its one job.
CHROMA_BIAS_SPREAD_CAP = 60.0


def _fit_cost_table(continuous_targets, weights, valid_mask=None, chroma_bias_scale=None):
    """(HEIGHT, 128) matrix of weighted squared distance from each row's own
    continuous target colour to every palette candidate -- the per-row
    "how good is each candidate" cost half of solve_colour_sequence()'s
    objective. Includes the CHROMA_BIAS_SCALE saturation-deficit penalty so
    flat grey can't out-compete a correctly-hued candidate on raw distance.

    chroma_bias_scale overrides the module-level CHROMA_BIAS_SCALE (None =
    use the module default). solve_n_frame() passes 0.0 for frames 1..n-1:
    those fit a RESIDUAL, not a real photographed colour, and the bias
    would chase the residual's arithmetic spread toward an arbitrary hue.
    Only trustworthy against frame 0's genuine target.

    valid_mask (optional, one bool per row): False rows (no on-pixels this
    round) get flat zero cost -- nothing real is being fit there."""
    if chroma_bias_scale is None:
        chroma_bias_scale = CHROMA_BIAS_SCALE
    diff = PALETTE_RGB[None, :, :] - continuous_targets[:, None, :]
    cost = np.sum(weights * diff ** 2, axis=-1)
    if chroma_bias_scale != 0.0:
        target_spread = continuous_targets.max(axis=1) - continuous_targets.min(axis=1)
        target_spread = np.minimum(target_spread, CHROMA_BIAS_SPREAD_CAP)
        deficit = np.maximum(0.0, target_spread[:, None] - PALETTE_SPREAD[None, :])
        cost = cost + chroma_bias_scale * deficit ** 2
    if valid_mask is not None:
        cost[~valid_mask] = 0.0
    return cost


def solve_colour_sequence(fit_cost, transition_scale, temporal_ref_colours=None, temporal_weight=0.0,
                           max_luma_spread=None):
    """Choose one palette index per row, for the WHOLE column at once, by
    exact dynamic programming (Viterbi). Minimises, over the entire column
    jointly:

        sum_y fit_cost[y, idx_y]
      + sum_y transition_scale * ||palette[idx_y] - palette[idx_{y-1}]||^2

    Solved exactly, not greedily: a long run of a colour that's a poor fit
    for most rows in it is only chosen if it's genuinely cheapest overall,
    so it can't drift arbitrarily far from any row's true colour the way a
    greedy chain can. One flat transition_scale suffices -- the DP already
    weighs each row's real fit-cost gap between candidates directly.

    `temporal_ref_colours`/`temporal_weight`: a cross-FRAME stability bias
    added to fit_cost before the DP runs. Biases a row toward reusing an
    earlier frame's colour on that row when it's near-achromatic (see
    achromatic_factor()) and reuse is cheap (reuse_cost/reuse_ok below) --
    white/grey rows have the least hue information pinning an
    independently-solved frame to the same rung, so they flicker most.

    `max_luma_spread` (default None): unrelated to temporal_weight -- not a
    bias toward reusing the exact colour, but a hard cap on how far a
    candidate's PERCEIVED BRIGHTNESS (PHOTOPIC_WEIGHTS) may sit from
    temporal_ref_colours[y]'s. Excluded candidates get cost +inf; any hue
    is still allowed, just not one that swings brightness past the cap.
    Falls back to unfiltered for a row if every candidate would be
    excluded (shouldn't happen, but safe rather than crash)."""
    height, n_pal = fit_cost.shape
    cost = fit_cost.copy()
    if temporal_ref_colours is not None and temporal_weight > 0:
        for y in range(height):
            tref = temporal_ref_colours[y]
            d_own = fit_cost[y]
            d_best = d_own.min()
            ref_idx = int(np.argmin(np.sum((PALETTE_RGB - tref) ** 2, axis=1)))
            reuse_cost = d_own[ref_idx] - d_best
            reuse_ok = TEMPORAL_REUSE_COST_SCALE / (TEMPORAL_REUSE_COST_SCALE + reuse_cost)
            eff_temporal = temporal_weight * achromatic_factor(tref) * reuse_ok
            if eff_temporal > 0:
                cost[y] = cost[y] + eff_temporal * np.sum((PALETTE_RGB - tref) ** 2, axis=1)

    if temporal_ref_colours is not None and max_luma_spread is not None:
        ref_luma = temporal_ref_colours @ PHOTOPIC_WEIGHTS               # (height,)
        luma_gap = np.abs(PALETTE_PERCEIVED_LUMA[None, :] - ref_luma[:, None])  # (height, 128)
        excluded = luma_gap > max_luma_spread
        all_excluded = excluded.all(axis=1)
        excluded[all_excluded] = False   # safety fallback -- see docstring
        cost = np.where(excluded, np.inf, cost)

    dp = np.empty((height, n_pal))
    back = np.empty((height, n_pal), dtype=np.int32)
    dp[0] = cost[0]
    trans_base = transition_scale * PALETTE_PAIRWISE_DIST2   # (prev, cur)
    idx_range = np.arange(n_pal)
    for y in range(1, height):
        trans = trans_base + dp[y - 1][:, None]
        bp = np.argmin(trans, axis=0)
        back[y] = bp
        dp[y] = cost[y] + trans[bp, idx_range]
    idxs = np.empty(height, dtype=np.int32)
    idxs[-1] = int(np.argmin(dp[-1]))
    for y in range(height - 2, -1, -1):
        idxs[y] = back[y + 1, idxs[y + 1]]
    return idxs


def achromatic_factor(rgb, threshold=ACHROMATIC_SATURATION_THRESHOLD):
    """1.0 for a fully grey/white colour (max channel == min channel),
    ramping linearly down to 0.0 once the channel spread reaches
    `threshold`. Gates the cross-frame temporal-stability term in
    solve_colour_sequence() so it only fires for near-achromatic rows --
    those are the rows most prone to visible frame-to-frame flicker (see
    solve_colour_sequence()'s docstring)."""
    spread = float(np.max(rgb) - np.min(rgb))
    return max(0.0, min(1.0, 1.0 - spread / threshold))


def load_target(path, saturation=1.0, brightness=1.0):
    """Resample to the screen's native WIDTHxHEIGHT. The source is assumed
    to already be at the correct display aspect ratio (see WIDTH_STRETCH)
    -- this does a plain resize, no letterboxing/cropping.

    saturation/brightness (both default 1.0 = no change) are applied to
    the full-resolution source BEFORE resizing, via PIL's ImageEnhance.
    The 128-colour palette plus binary on/off plus vertical blend against
    black neighbours structurally pulls the reconstructed image's
    perceived brightness/saturation down from the source's, independent of
    frame count -- rather than compensate on the physical display, this
    boosts the input before the optimiser ever sees it, so its target is
    already closer to what needs to land on screen."""
    img = Image.open(path).convert("RGB")
    if saturation != 1.0:
        img = ImageEnhance.Color(img).enhance(saturation)
    if brightness != 1.0:
        img = ImageEnhance.Brightness(img).enhance(brightness)
    img = img.resize((WIDTH, HEIGHT), Image.LANCZOS)
    return np.asarray(img, dtype=np.float64)  # (HEIGHT, WIDTH, 3)


def estimate_background_fraction(target, luma_threshold=24.0):
    """Fraction of pixels in the resized target that are near-black
    (off-canvas/background). Purely informational -- see auto_smoothness()."""
    luma = target.mean(axis=-1)
    return float((luma < luma_threshold).mean())


def auto_smoothness(target, ceiling=0.5, verbose=True):
    """Recommended --smoothness value for --smoothness auto. Always
    returns `ceiling` (default 0.5) -- the actual adaptation to image
    content happens per-row inside solve_colour_sequence()'s DP, not at
    the whole-image level, since a single image can contain both large
    decisive flat-fill regions and ambiguous near-tied ones that would
    need opposite whole-image smoothness values. Prints the background
    (near-black) pixel fraction as a diagnostic only; it doesn't affect
    the returned value."""
    bg_frac = estimate_background_fraction(target)
    if verbose:
        print(f"[smoothness] background fraction = {bg_frac:.1%} (informational only); "
              f"using ceiling = {ceiling}", flush=True)
    return ceiling


def make_kernel(sigma, radius):
    d = np.arange(-radius, radius + 1)
    w = np.exp(-(d.astype(np.float64) ** 2) / (2.0 * sigma * sigma))
    return {int(k): float(v) for k, v in zip(d, w)}


def solve(target, sigma=1.0, radius=3, rounds=10, sweeps=3, seed=0,
          metric="luma", swaps=False, smoothness=0.5, verbose=True, label="",
          temporal_ref_colours=None, temporal_weight=0.0, chroma_bias_scale=None,
          fixed_color_idx=None, max_luma_spread=None, swing_weight=0.0,
          sibling_frames=None):
    """fixed_color_idx (optional, one palette index per row): skip both the
    colour-selection init AND the per-round colour-refinement phase
    entirely, using this sequence as-is throughout -- only the on/off
    (which pixels in each row are lit) mask is solved. Used by the joint
    multi-frame solve (solve_n_frame_joint()), which decides every frame's
    colour sequence together up front (see its docstring for why) and
    only needs solve() for its on/off-mask machinery from here on.

    `swing_weight`/`sibling_frames` (default 0.0/None, off): penalises this
    frame for choosing a per-pixel RGB value -- across all n_frames -- that
    swings far (full weighted-RGB distance via werr(), NOT luma alone) from
    sibling frames' current value at that pixel. Luma alone is not enough:
    a grey and a warm orange can have near-identical photopic luma while
    being nothing alike (found on pirate.webp -- a luma-only version of
    this penalty left a grey/orange row-to-row alternation completely
    unpenalised, luma gap ~7 vs RGB distance ~97, and had zero effect at
    any weight). Two real-hardware artefacts share this one cause (a
    pixel's displayed colour varying frame to frame) and get penalised in
    the two places colour/on-off are actually decided:
      - phase 1 (DBS flip sweep): on/off swing -- a pixel on in only some
        frames strobes full colour/black every cycle (~50% of on-canvas
        pixels affected, unmitigated, measured on real test images).
      - phase 2 (colour DP): same-colour-everywhere swing -- e.g. the
        `cumulative` scheme's residual-chasing picks a different colour per
        frame for the same row even with every frame fully on, reading as
        a static bright/busy band rather than flicker.
    Both phases cost swing via werr(), the same weighted-squared-RGB-
    distance function fit_cost itself uses, so swing_weight is on the same
    natural scale as the fit-cost terms it's traded against -- no separate
    scale constant to calibrate.
    sibling_frames is a list of (color_idx, on) for every OTHER frame's
    CURRENT state; only meaningful when called with sibling_frames from
    solve_n_frame_cyclic()'s outer-round loop, where every other frame's
    state already exists."""
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

    # Initialise colours via one whole-column DP pass, not a greedy per-row
    # scan. Per-row target is the average of non-near-black pixels only: a
    # row that's mostly black canvas with a small saturated subject would
    # otherwise average to a confident-but-wrong dark grey, and on/off can
    # co-adapt to that bad initial colour before refinement corrects it.
    # Falls back to the whole-row average for all-background rows.
    if fixed_color_idx is not None:
        color_idx = np.array(fixed_color_idx, dtype=np.int64).copy()
    else:
        likely_on_avgs = np.zeros((HEIGHT, 3), dtype=np.float64)
        for y in range(HEIGHT):
            row = target[y]
            likely_on = row.sum(axis=-1) > 30.0
            likely_on_avgs[y] = row[likely_on].mean(axis=0) if likely_on.any() else row.mean(axis=0)
        transition_scale = smoothness * SMOOTHNESS_DP_SCALE
        color_idx = solve_colour_sequence(
            _fit_cost_table(likely_on_avgs, weights, chroma_bias_scale=chroma_bias_scale),
            transition_scale,
            temporal_ref_colours=temporal_ref_colours,
            temporal_weight=temporal_weight,
            max_luma_spread=max_luma_spread)
    colour = PALETTE_RGB[color_idx]
    sibling_raw_rgb = None
    if swing_weight > 0 and sibling_frames:
        # Full RGB, not luma: two colours can have near-identical photopic
        # luma (e.g. a grey and a warm orange) while being nothing alike --
        # found on pirate.webp, where a luma-only version of this penalty
        # left a grey/orange row-to-row alternation completely unpenalised
        # (luma gap ~7, RGB distance ~97) and had zero effect at any
        # weight. werr() is reused so swing cost lands in the same units
        # as fit_cost's own candidate-vs-target distance, rather than a
        # separately-scaled quantity that has to be guessed and recalibrated.
        sibling_raw_rgb = np.zeros((len(sibling_frames), HEIGHT, WIDTH, 3), dtype=np.float64)
        for k, (sib_idx, sib_on) in enumerate(sibling_frames):
            sib_rgb_on = PALETTE_RGB[sib_idx]   # (HEIGHT,3)
            sibling_raw_rgb[k] = np.where(sib_on[:, :, None], sib_rgb_on[:, None, :], 0.0)

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

    # NOTE: `label` arrives already bracketed by callers (solve_best_of
    # passes tags like "[frame0]" or "[frame0] seed 2/3"), so it's used
    # as-is here rather than wrapped in another set of brackets.
    tag = f"{label} " if label else ""
    t_start = time.monotonic()
    if verbose:
        print(f"{tag}round 0 (init): weighted SSE = {total_werror():.1f}", flush=True)

    for rnd in range(1, rounds + 1):
        # ---------------- phase 1: on/off refinement (DBS, per column) --
        for x in range(WIDTH):
            # Alternate flip-convergence and a swap-sweep until neither
            # improves anything. Still a CAPPED loop, not true
            # while-until-convergence: measured worst case across zph/
            # donald/parrot needed 9 alternations, 20 is a >2x safety
            # margin (85-90% of columns converge in the first alternation
            # anyway). Re-measure if you change swaps/metric/kernel.
            for _outer in range(20):
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
                        if sibling_raw_rgb is not None:
                            zero3 = np.zeros(3)
                            cur_raw = col if cur_on else zero3
                            new_raw = zero3 if cur_on else col
                            sib = sibling_raw_rgb[:, y, x]              # (n_sib, 3)
                            cur_swing_cost = werr(sib - cur_raw).max()
                            new_swing_cost = werr(sib - new_raw).max()
                            delta_total += swing_weight * (new_swing_cost - cur_swing_cost)
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

        # ---------------- phase 2: colour refinement (whole-column DP) --
        # Skipped when fixed_color_idx was given: the caller
        # (solve_n_frame_joint()) already decided every row's colour for
        # every frame jointly, and re-optimizing one frame in isolation
        # here would undo that.
        #
        # Every row's ideal continuous colour (closed-form correction
        # against the start-of-round blended state) is computed from one
        # shared snapshot before anything is applied -- Jacobi-style, not
        # sequential Gauss-Seidel, so the DP sees every row's true,
        # uncorrupted fit cost in the same pass rather than a shifting
        # target as earlier rows' fresh choices leak in. Rows with no
        # on-pixels this round get a flat don't-care cost.
        if fixed_color_idx is None:
            continuous_targets = colour.copy()
            valid_mask = np.zeros(HEIGHT, dtype=bool)
            for y in range(HEIGHT):
                xs_on = np.nonzero(on[y])[0]
                if len(xs_on) == 0:
                    continue
                valid_mask[y] = True
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
                continuous_targets[y] = colour[y] + PHASE2_RELAXATION * correction

            fit_cost = _fit_cost_table(continuous_targets, weights, valid_mask=valid_mask,
                                        chroma_bias_scale=chroma_bias_scale)
            if swing_weight > 0 and sibling_raw_rgb is not None:
                # Colour-choice swing: unlike the phase-1 flip penalty
                # (single pixel), a row's colour applies to every on-pixel
                # in it at once, so this is meaned rather than summed over
                # the row's on-pixels -- keeps swing_weight's effective
                # strength comparable between the two phases. Uses werr()
                # (full weighted-RGB distance), same as fit_cost itself, so
                # the two terms are naturally on the same scale -- no
                # separate fudge-factor constant to guess/recalibrate.
                for y in range(HEIGHT):
                    xs_on = np.nonzero(on[y])[0]
                    if len(xs_on) == 0:
                        continue
                    sib_row = sibling_raw_rgb[:, y, xs_on]                # (n_sib, k, 3)
                    diff = PALETTE_RGB[:, None, None, :] - sib_row[None, :, :, :]  # (128,n_sib,k,3)
                    costs = np.sum(weights * diff ** 2, axis=-1)          # (128, n_sib, k)
                    swing_cost = costs.max(axis=1).mean(axis=1)           # (128,) worst sibling, meaned over on-pixels
                    fit_cost[y] += swing_weight * swing_cost
            transition_scale = smoothness * SMOOTHNESS_DP_SCALE
            new_color_idx = solve_colour_sequence(fit_cost, transition_scale,
                                                   temporal_ref_colours=temporal_ref_colours,
                                                   temporal_weight=temporal_weight,
                                                   max_luma_spread=max_luma_spread)

            # Apply every row's change as an additive delta against the SAME
            # start-of-round blended state used above (order doesn't matter --
            # each row's delta is independent superposition onto blended[]).
            for y in range(HEIGHT):
                if new_color_idx[y] == color_idx[y]:
                    continue
                xs_on = np.nonzero(on[y])[0]
                new_colour = PALETTE_RGB[new_color_idx[y]]
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
                color_idx[y] = new_color_idx[y]

        if verbose:
            elapsed = time.monotonic() - t_start
            print(f"{tag}round {rnd}/{rounds}: weighted SSE = {total_werror():.1f}  [{elapsed:.1f}s elapsed]", flush=True)

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
        if verbose:
            print(f"{tag}starting seed {s+1}/{seeds}...", flush=True)
        result = solve(target, seed=s, verbose=verbose, label=label, **kwargs)
        werror = result[-1]
        if best is None or werror < best[-1]:
            best = result
            best_seed = s
    if verbose and seeds > 1:
        print(f"{tag}best result: seed {best_seed + 1}/{seeds} "
              f"(weighted SSE = {best[-1]:.1f}, RMS = {best[-2]:.2f})", flush=True)
    return best


# Weight/cap for the anti-grey chroma bias in solve_colour_triplets(),
# same idea as CHROMA_BIAS_SCALE/CHROMA_BIAS_SPREAD_CAP but applied to a
# candidate AVERAGE of up to 3 palette entries. Separate constants since
# this always compares against a genuine target, never a residual.
JOINT_CHROMA_BIAS_SCALE = 1.0
JOINT_CHROMA_BIAS_SPREAD_CAP = 60.0


def solve_colour_triplets(row_targets, weights, n_frames=3,
                           chroma_bias_scale=JOINT_CHROMA_BIAS_SCALE,
                           chroma_cap=JOINT_CHROMA_BIAS_SPREAD_CAP,
                           k_a=16, k_partner=6):
    """For every row, choose a MULTISET of n_frames real palette entries
    (repeats allowed) whose AVERAGE best matches that row's target colour --
    the joint alternative to solve_n_frame()'s cumulative-residual scheme.

    solve_n_frame() fits frame 1+ against a RESIDUAL, which isn't a real
    in-gamut colour -- an awkward residual can collapse a whole frame to
    flat grey or chase a wrong saturated hue. This function sidesteps that
    by never computing a residual: for n_frames == 3 (fixed project-wide)
    it searches three families of real achievable averages per row and
    keeps whichever scores best:

      - single: one palette entry, used all 3 frames
      - 2:1: one entry twice, a second entry once
      - 1:1:1: three distinct entries, once each

    Every candidate is an average of REAL palette entries, so there's no
    out-of-gamut residual to chase -- grey only wins if it's genuinely the
    best match (JOINT_CHROMA_BIAS_SCALE penalises it otherwise, as
    CHROMA_BIAS_SCALE does for the single-frame case).

    No penalty discourages the 2:1/1:1:1 colours from being far apart:
    temporal dithering only helps when they ARE far apart. Per-frame column
    continuity is handled by assign_triplet_to_frames().

    Returns (chosen, cost): chosen is (HEIGHT, n_frames) int array of
    palette indices (unordered), cost is (HEIGHT,) achieved cost."""
    if n_frames != 3:
        raise ValueError("solve_colour_triplets() is hardcoded for n_frames == 3 "
                          "(frame count is fixed project-wide)")
    H = row_targets.shape[0]

    def chroma_bias(avg_rgb, target_spread_capped):
        avg_spread = avg_rgb.max(axis=-1) - avg_rgb.min(axis=-1)
        deficit = np.maximum(0.0, target_spread_capped - avg_spread)
        return chroma_bias_scale * deficit ** 2

    target_spread_capped = np.minimum(
        row_targets.max(axis=-1) - row_targets.min(axis=-1), chroma_cap)

    diff = PALETTE_RGB[None, :, :] - row_targets[:, None, :]
    fit_single = np.sum(weights * diff ** 2, axis=-1)                    # (H,128)
    cost_single = fit_single + chroma_bias(PALETTE_RGB[None, :, :], target_spread_capped[:, None])
    best_single_idx = np.argmin(cost_single, axis=1)
    best_cost = cost_single[np.arange(H), best_single_idx]
    best_combo = np.stack([best_single_idx] * 3, axis=1)                 # (H,3)

    # Candidate "first" colours for the 2:1 and 1:1:1 searches: the top-K
    # single-match candidates per row (cheap prefilter; the true best combo
    # essentially always has at least one of its members near a strong
    # single-match candidate in practice).
    order = np.argsort(cost_single, axis=1)[:, :k_a]                     # (H,k_a)

    # ---- 2:1 split: candidate A appears twice, B once ----
    for k in range(k_a):
        A_idx = order[:, k]
        A_rgb = PALETTE_RGB[A_idx]
        ideal_B = 3 * row_targets - 2 * A_rgb
        dB = np.sum(weights * (ideal_B[:, None, :] - PALETTE_RGB[None, :, :]) ** 2, axis=-1)
        partner_order = np.argsort(dB, axis=1)[:, :k_partner]
        for p in range(k_partner):
            B_idx = partner_order[:, p]
            B_rgb = PALETTE_RGB[B_idx]
            avg = (2 * A_rgb + B_rgb) / 3.0
            fit = np.sum(weights * (avg - row_targets) ** 2, axis=-1)
            total = fit + chroma_bias(avg, target_spread_capped)
            better = total < best_cost
            best_cost = np.where(better, total, best_cost)
            best_combo[better, 0] = A_idx[better]
            best_combo[better, 1] = A_idx[better]
            best_combo[better, 2] = B_idx[better]

    # ---- 1:1:1 triplet: three distinct entries, greedy 2-level search ----
    for k in range(k_a):
        A_idx = order[:, k]
        A_rgb = PALETTE_RGB[A_idx]
        remaining = 3 * row_targets - A_rgb
        ideal_B = remaining / 2.0
        dB = np.sum(weights * (ideal_B[:, None, :] - PALETTE_RGB[None, :, :]) ** 2, axis=-1)
        B_cand = np.argsort(dB, axis=1)[:, :k_partner]
        for p in range(k_partner):
            B_idx = B_cand[:, p]
            B_rgb = PALETTE_RGB[B_idx]
            ideal_C = remaining - B_rgb
            dC = np.sum(weights * (ideal_C[:, None, :] - PALETTE_RGB[None, :, :]) ** 2, axis=-1)
            C_idx = np.argmin(dC, axis=1)
            C_rgb = PALETTE_RGB[C_idx]
            avg = (A_rgb + B_rgb + C_rgb) / 3.0
            fit = np.sum(weights * (avg - row_targets) ** 2, axis=-1)
            total = fit + chroma_bias(avg, target_spread_capped)
            better = total < best_cost
            best_cost = np.where(better, total, best_cost)
            best_combo[better, 0] = A_idx[better]
            best_combo[better, 1] = B_idx[better]
            best_combo[better, 2] = C_idx[better]

    return best_combo, best_cost


def assign_triplet_to_frames(triplets):
    """Row-by-row, assign each row's (already chosen, unordered) 3 palette
    indices to frame slots 0/1/2, picking whichever of the (up to 6)
    permutations keeps each frame slot closest to its OWN previous row's
    colour. solve_colour_triplets() decides WHAT 3 colours a row should use
    with no notion of "which frame"; without this, row-to-row ordering
    would be arbitrary and each frame's column would jump around for no
    reason. Cheap exhaustive search (<=6 permutations/row), costs nothing
    in fit quality (all permutations of a multiset score identically).

    Returns (HEIGHT, 3) int array: assigned[y, i] is frame i's palette
    index at row y."""
    import itertools
    H = triplets.shape[0]
    assigned = np.zeros((H, 3), dtype=np.int64)
    assigned[0] = triplets[0]  # no previous row to match at row 0
    prev_rgb = PALETTE_RGB[triplets[0]]  # (3,3), one row per frame slot
    perms = list(itertools.permutations(range(3)))
    for y in range(1, H):
        cur_idx = triplets[y]
        cur_rgb = PALETTE_RGB[cur_idx]  # (3,3), unordered
        best_perm, best_cost = None, np.inf
        for perm in perms:
            permuted = cur_rgb[list(perm)]
            cost = float(np.sum((permuted - prev_rgb) ** 2))
            if cost < best_cost:
                best_cost, best_perm = cost, perm
        assigned[y] = cur_idx[list(best_perm)]
        prev_rgb = PALETTE_RGB[assigned[y]]
    return assigned


# Scale multiplying --smoothness (0-1ish) into the transition cost for the
# per-frame column-smoothing pass (solve_n_frame_joint()'s
# smooth_frame_column()). solve_colour_triplets() decides each row
# independently, leaving row-to-row jitter within a single frame's column;
# this second DP pass anchors each row's fit cost to the jointly-chosen
# colour (never a residual). Same sharp-cliff collapse risk as
# SMOOTHNESS_DP_SCALE, calibrated the same way: default --smoothness 0.5
# gives transition_scale ~0.3.
JOINT_SMOOTHING_DP_SCALE = 0.6


def smooth_frame_column(assigned_idx, weights, transition_scale):
    """Second DP pass smoothing ONE frame's already-jointly-chosen colour
    sequence against row-to-row jitter (see JOINT_SMOOTHING_DP_SCALE).
    Anchors each row's fit cost to the colour already picked by
    solve_colour_triplets()/assign_triplet_to_frames() (never a residual),
    then runs the same whole-column Viterbi DP used elsewhere."""
    anchor_rgb = PALETTE_RGB[assigned_idx]                      # (H,3)
    fit_cost = np.sum(weights * (PALETTE_RGB[None, :, :] - anchor_rgb[:, None, :]) ** 2, axis=-1)
    return solve_colour_sequence(fit_cost, transition_scale)


def solve_n_frame_joint(target, n_frames=3, seeds=3, sigma=1.0, radius=3,
                         rounds=10, sweeps=3, metric="luma", smoothness=0.5,
                         outer_rounds=1, verbose=True, **kwargs):
    """The joint alternative to solve_n_frame() -- see
    solve_colour_triplets()'s docstring for the full rationale (the old
    scheme's residual-chasing could collapse a whole frame to flat grey or
    a wrong saturated hue; this never constructs a residual).

    Every frame's colour SEQUENCE is decided together, per row, from the
    current per-row target (solve_colour_triplets() + assign_triplet_to_
    frames() below), then smoothed per frame (smooth_frame_column(),
    controlled by `smoothness`) to reduce row-to-row jitter -- then each
    frame is solved independently only for its on/off mask, via
    solve(..., fixed_color_idx=...), since that part doesn't interact
    across frames. `outer_rounds` (default 3) repeats this using the REAL
    achieved blended average each round to correct next round's per-row
    target (see comment above), applied equally across all frames' target
    so no single frame is ever left to fix a gap alone."""
    if n_frames != 3:
        raise ValueError("solve_n_frame_joint() is hardcoded for n_frames == 3 "
                          "(frame count is fixed project-wide)")
    verbose_inner = kwargs.pop("verbose", verbose)
    weights = get_weights(metric)

    likely_on = target.sum(axis=-1) > 30.0                      # (HEIGHT, WIDTH)
    likely_on_avgs = np.zeros((HEIGHT, 3), dtype=np.float64)
    for y in range(HEIGHT):
        row = target[y]
        mask = likely_on[y]
        likely_on_avgs[y] = row[mask].mean(axis=0) if mask.any() else row.mean(axis=0)

    # ---- outer refinement: the crude row-average target above ignores
    # vertical blending and which pixels actually end up on -- the gap
    # solve()'s phase-2 DP closes for a single frame, done here jointly.
    # After solving all n_frames on/off masks against the current per-row
    # targets, measure the actual achieved average, fold the remaining
    # error back into next round's per-row target, and redo the (still
    # fully joint, never-a-per-frame-residual) triplet search. The
    # correction is applied EQUALLY to all frames' shared target every
    # round, so this doesn't reintroduce the cumulative scheme's
    # flat-grey/wrong-hue collapse -- every round still goes through
    # solve_colour_triplets()'s chroma-bias-corrected search.
    cur_target = likely_on_avgs.copy()
    color_idxs, ons, blendeds = None, None, None
    t_start = time.monotonic()
    for outer in range(max(1, outer_rounds)):
        triplets, _ = solve_colour_triplets(cur_target, weights, n_frames=n_frames)
        assigned = assign_triplet_to_frames(triplets)  # (HEIGHT, 3)

        transition_scale = smoothness * JOINT_SMOOTHING_DP_SCALE
        if transition_scale > 0:
            for i in range(n_frames):
                assigned[:, i] = smooth_frame_column(assigned[:, i], weights, transition_scale)

        color_idxs, ons, blendeds = [], [], []
        for i in range(n_frames):
            if verbose:
                print(f"=== outer {outer+1}/{outer_rounds}: solving frame {i+1}/{n_frames} on/off "
                      f"mask (seeds={seeds}) [{time.monotonic()-t_start:.1f}s elapsed total] ===",
                      flush=True)
            result = solve_best_of(target, seeds=seeds, sigma=sigma, radius=radius,
                                    rounds=rounds, sweeps=sweeps, metric=metric,
                                    tag=f"[outer{outer+1} frame{i}] ", verbose=verbose_inner,
                                    fixed_color_idx=assigned[:, i], **kwargs)
            color_idx_i, on_i, blended_i, rmse_i, werr_i = result
            color_idxs.append(color_idx_i)
            ons.append(on_i)
            blendeds.append(blended_i)

        if outer + 1 < outer_rounds:
            achieved = sum(blendeds) / n_frames
            achieved_row_avg = np.zeros((HEIGHT, 3), dtype=np.float64)
            for y in range(HEIGHT):
                mask = likely_on[y]
                achieved_row_avg[y] = achieved[y][mask].mean(axis=0) if mask.any() else achieved[y].mean(axis=0)
            cur_target = likely_on_avgs + (likely_on_avgs - achieved_row_avg)
            if verbose:
                partial_rmse = (float(np.sum((target - achieved) ** 2)) / (HEIGHT * WIDTH * 3)) ** 0.5
                print(f"[outer {outer+1}/{outer_rounds}] RMS = {partial_rmse:.2f}  "
                      f"[{time.monotonic()-t_start:.1f}s elapsed]", flush=True)

    averaged = sum(blendeds) / n_frames
    final_rgb_sse = float(np.sum((target - averaged) ** 2))
    final_rmse = (final_rgb_sse / (HEIGHT * WIDTH * 3)) ** 0.5

    if verbose:
        print(f"{n_frames}-frame joint RMS (unweighted RGB): {final_rmse:.2f}", flush=True)

    return {
        "color_idxs": color_idxs, "ons": ons, "blendeds": blendeds,
        "rmse_first": None, "averaged": averaged, "final_rmse": final_rmse,
    }


def solve_n_frame(target, n_frames=2, seeds=3, sigma=1.0, radius=3, **kwargs):
    """Solve n_frames frames that, cycled (frame0, frame1, ..., frame(n-1),
    frame0, ...) one per video frame, temporally average (via persistence
    of vision, equal 1/n weight each) to a better match of the target than
    any single frame alone.

    Each frame is solved with the exact same machinery as the single-frame
    case -- only the target array changes, frame by frame. CUMULATIVE
    scheme: frame i is solved as if it were the *last* frame of an
    (i+1)-frame sequence, closing whatever gap frames 0..i-1 left behind:

        cumulative = 0                             # sum of committed frames so far
        for i in 0..n-1:
            this_target = (i+1)*target - cumulative    # close the gap fully, assuming i is last
            solve frame i to fit this_target -> blended_i
            cumulative += blended_i

    This makes the sequence prefix-compatible: frame i's target depends
    only on frames 0..i-1 and i, never on total frame count. A 3-frame
    run's frame0/frame1 are byte-identical to a standalone 2-frame run's
    (same seeds) -- a 3rd correcting frame can be bolted on later without
    regenerating earlier ones. (Redistributing the remaining gap evenly
    across frames still to come is not prefix-compatible and measured no
    better on final RMS.)

    Frame 0's target is (0+1)*target - 0 == target exactly -- the ordinary
    single-frame fit, and its own RMS is a fair "stopped after 1 frame"
    baseline.

    `temporal_weight` (default 0, off) adds a cross-frame stability term:
    frames 1..n-1 are biased toward reusing frame 0's colour on a given
    row, but only where that colour is near-achromatic (see
    solve_colour_sequence()'s temporal_ref/temporal_weight docs -- stops
    white/grey areas flickering between adjacent palette rungs, since
    achromatic content has the largest frame-to-frame swing: no hue axis
    to help hit a target brightness, only ~8 discrete grey rungs).
    Anchored to frame 0 specifically (not "the previous frame") so this
    stays prefix-compatible.

    `max_luma_spread` (default None, off): see solve_colour_sequence()'s
    docstring. Unlike temporal_weight (a bias), this hard-excludes any
    candidate on rows 1..n-1 whose PERCEIVED BRIGHTNESS (PHOTOPIC_WEIGHTS)
    sits further than the cap from frame 0's on that row.
    """
    if n_frames < 1:
        raise ValueError("n_frames must be >= 1")
    verbose = kwargs.get("verbose", True)
    temporal_weight = kwargs.pop("temporal_weight", 0.0)

    cumulative = np.zeros_like(target)
    color_idxs, ons, blendeds = [], [], []
    rmse_first = None
    frame0_colours = None
    t_start = time.monotonic()
    for i in range(n_frames):
        if verbose:
            print(f"=== solving frame {i+1}/{n_frames} (seeds={seeds}) "
                  f"[{time.monotonic()-t_start:.1f}s elapsed total] ===", flush=True)
        this_target = (i + 1) * target - cumulative
        tref = frame0_colours if i > 0 else None
        # The chroma-anti-grey bias (_fit_cost_table's docstring) is only
        # trustworthy against a genuine photographed target, not a
        # residual, so it's off for frames 1..n-1. Also forced off for
        # frame 0 when n_frames > 1: fitting frame 0 more accurately
        # leaves a harder residual for frame 1, and this cumulative
        # scheme's cross-frame balance is too fragile for a single-frame
        # fit-cost tweak to safely fix (needs joint solving instead, see
        # solve_n_frame_joint()). Caller can still override
        # chroma_bias_scale explicitly; single-frame calls via
        # solve_best_of()/solve() keep the bias on by default.
        frame_kwargs = dict(kwargs)
        if n_frames > 1 and "chroma_bias_scale" not in frame_kwargs:
            frame_kwargs["chroma_bias_scale"] = 0.0
        result = solve_best_of(this_target, seeds=seeds, sigma=sigma, radius=radius,
                                tag=f"[frame{i}] ", temporal_ref_colours=tref,
                                temporal_weight=temporal_weight, **frame_kwargs)
        color_idx_i, on_i, blended_i, rmse_i, werr_i = result
        color_idxs.append(color_idx_i)
        ons.append(on_i)
        blendeds.append(blended_i)
        if i == 0:
            rmse_first = rmse_i
            frame0_colours = PALETTE_RGB[color_idx_i]
        cumulative = cumulative + blended_i

        if verbose:
            partial_avg = cumulative / (i + 1)
            partial_rmse = (float(np.sum((target - partial_avg) ** 2)) / (HEIGHT * WIDTH * 3)) ** 0.5
            print(f"after {i+1}/{n_frames} frame(s): running-average RMS = {partial_rmse:.2f}", flush=True)

    averaged = sum(blendeds) / n_frames
    final_rgb_sse = float(np.sum((target - averaged) ** 2))
    final_rmse = (final_rgb_sse / (HEIGHT * WIDTH * 3)) ** 0.5

    if verbose:
        print(f"1-frame RMS would have been: {rmse_first:.2f}", flush=True)
        print(f"{n_frames}-frame alternating RMS (unweighted RGB): {final_rmse:.2f}", flush=True)

    return {
        "color_idxs": color_idxs, "ons": ons, "blendeds": blendeds,
        "rmse_first": rmse_first, "averaged": averaged, "final_rmse": final_rmse,
    }


def compute_frame_jump(color_idxs, ons, height, width):
    """Mean cyclic (wraparound included) frame-to-frame RMS difference in
    raw displayed pixels (colour if on, else black) -- the flicker-
    magnitude metric. If much bigger than the final time-averaged RMS,
    most of what's happening is high-contrast strobing, not gentle
    blending."""
    n = len(color_idxs)
    raws = []
    for i in range(n):
        colour = PALETTE_RGB[color_idxs[i]]
        raw = np.zeros((height, width, 3), dtype=np.float64)
        raw[ons[i]] = colour[np.nonzero(ons[i])[0]]
        raws.append(raw)
    jumps = []
    for i in range(n):
        j = (i + 1) % n
        diff = raws[i] - raws[j]
        jumps.append((np.sum(diff ** 2) / (height * width * 3)) ** 0.5)
    return float(np.mean(jumps))


def solve_n_frame_cyclic(target, n_frames=3, outer_rounds=4, seeds=3,
                          sigma=1.0, radius=3, verbose=True, terse=False,
                          frame_scheme="cumulative", **kwargs):
    """N-frame solve using CYCLIC (bidirectional) relaxation across frames,
    instead of solve_n_frame()'s one-way forward/causal residual chase.

    PROBLEM WITH THE PLAIN FORWARD SCHEME: solve_n_frame() is strictly
    causal -- frame i's target depends only on frames 0..i-1, nothing is
    ever revisited, and there's no "wraparound" (frame N-1 back to frame
    0) during solving. On a real multi-frame result the frame-to-frame
    raw-pixel RMS jump can come out larger than the final time-averaged
    RMS -- a real flicker risk on hardware (same mechanism as
    sprite-multiplexing flicker on real Atari 2600s).

    CYCLIC ALTERNATIVE: treat "sum of all N frames == N*target" as a
    symmetric, cyclic constraint with no privileged starting frame, and
    enforce it with Gauss-Seidel-style coordinate descent ACROSS FRAMES:
        1. Initial guess for all N frames via the existing one-pass
           solve_n_frame().
        2. Repeat for `outer_rounds` passes: for each frame i (0..N-1),
           other_sum = sum of all OTHER frames' current blended output
           (wraparound included once a full pass has happened -- frame 0
           reacts to frame N-1's current state too); this_target =
           N*target - other_sum; re-solve frame i against this_target.
        3. Stop after `outer_rounds` passes -- no established plateau
           point yet like the inner --rounds has; watch printed per-round
           RMS to judge convergence.
    NOT guaranteed to converge monotonically -- each frame's re-solve is a
    full independent local search, so outer rounds can oscillate. Per-round
    RMS and the frame-jump metric are printed/returned so that's visible.

    terse=False (default): full per-DBS-round SSE progress. terse=True:
    only frame-solve start and outer-round-finish -- a middle ground
    between full detail and verbose=False.

    frame_scheme="cumulative" (default): solve_n_frame() initial pass +
    outer-round residual refinement. Meaningfully lower RMS than "joint",
    but can collapse a whole frame to flat grey (or a wrong hue) on an
    awkward residual -- check the per-frame _raw.png previews. "joint"
    can't produce that collapse but reads flatter/less vibrant overall --
    a real, still-open trade-off.

    frame_scheme="joint": initialise with solve_n_frame_joint() and RETURN
    THAT DIRECTLY, skipping the outer-round refinement loop. That loop is
    per-frame residual-chasing -- exactly what solve_n_frame_joint() was
    built to avoid (see solve_colour_triplets()'s docstring) -- so running
    it on top of an already-good joint result would reintroduce the same
    collapse/wrong-hue risk. outer_rounds is ignored (with a note) here.

    `swing_weight` (default 0.0, off): only takes effect inside the
    outer-round loop below, where every frame's CURRENT on/off state is
    available to pass as sibling_frames to solve()'s coverage penalty
    (see its docstring) -- the initial one-pass solve_n_frame() call has
    no siblings solved yet, so it always runs uncorrected."""
    if n_frames < 2:
        raise ValueError("n_frames must be >= 2 -- use --frames 1 for a single static frame")

    height, width = HEIGHT, WIDTH
    inner_verbose = verbose and not terse
    temporal_weight = kwargs.pop("temporal_weight", 0.0)
    swing_weight = kwargs.pop("swing_weight", 0.0)
    swing_metric_weights = get_weights(kwargs.get("metric", "luma"))

    def score(s):
        avg = s / n_frames
        return (float(np.sum((target - avg) ** 2)) / (height * width * 3)) ** 0.5

    def swing_cost(idxs, on_masks):
        """Mean per-pixel worst-case pairwise werr() distance between all
        n_frames' raw displayed RGB -- the SAME quantity/units solve()'s
        swing_weight penalty minimises internally (see its docstring), so
        round_selector below can add swing_weight * this directly, with no
        separate scale constant to guess. 0.0 at swing_weight == 0 makes no
        difference (never called), but computing it that way rather than
        gating the WHOLE selector on `swing_weight > 0` matters: an earlier
        version switched the selector to a different, unrelated metric
        (frame_jump) for ANY nonzero swing_weight regardless of magnitude,
        so even a vanishingly small weight (with negligible effect inside
        solve() itself) could still discard a genuinely better round --
        found via outer_rounds=0 vs 1 at swing_weight=1e-9 keeping the
        worse (init) state instead of round 1's real improvement."""
        raws = np.stack([np.where(on[:, :, None], PALETTE_RGB[idx][:, None, :], 0.0)
                          for idx, on in zip(idxs, on_masks)], axis=0)  # (n_frames,H,W,3)
        worst = np.zeros((height, width))
        for i in range(n_frames):
            for j in range(i + 1, n_frames):
                d = np.sum(swing_metric_weights * (raws[i] - raws[j]) ** 2, axis=-1)
                worst = np.maximum(worst, d)
        return float(worst.mean())

    if frame_scheme == "joint":
        if n_frames != 3:
            raise ValueError("frame_scheme='joint' currently only supports n_frames == 3 "
                              "(frame count is fixed project-wide anyway)")
        if verbose:
            print(f"=== joint pass: {n_frames}-frame colour sequences solved together ===",
                  flush=True)
            if outer_rounds:
                print(f"(outer_rounds={outer_rounds} ignored: the joint scheme doesn't use "
                      f"outer-round residual refinement -- see solve_n_frame_cyclic()'s "
                      f"docstring)", flush=True)
            if swing_weight:
                print(f"(swing_weight={swing_weight} ignored: only takes effect inside "
                      f"the outer-round loop, which the joint scheme skips)", flush=True)
        result = solve_n_frame_joint(target, n_frames=n_frames, seeds=seeds,
                                      sigma=sigma, radius=radius, verbose=inner_verbose,
                                      **kwargs)
        if verbose:
            print(f"[joint] RMS = {result['final_rmse']:.2f}", flush=True)
        return result

    # See solve_n_frame() for the temporal_weight rationale (white/grey
    # flicker fix, opt-in, default 0). Anchored to frame 0's CURRENT
    # colour throughout, even though frame 0 itself gets re-solved every
    # outer round here (unlike the forward scheme) -- reusing "whatever
    # frame 0 currently holds" needs no special first-round case.

    # ---- initial guess: reuse the one-pass forward scheme as a cheap
    # starting point (already satisfies "average == target" approximately);
    # outer rounds let every frame react to every other frame's current
    # state instead of freezing each one the moment it's first solved. ----
    if verbose:
        print(f"=== initial pass: one-pass forward solve, {n_frames} frame(s) ===", flush=True)
    init = solve_n_frame(target, n_frames=n_frames, seeds=seeds,
                          sigma=sigma, radius=radius, verbose=inner_verbose,
                          temporal_weight=temporal_weight, **kwargs)
    color_idxs = list(init["color_idxs"])
    ons = list(init["ons"])
    blendeds = list(init["blendeds"])
    cur_sum = sum(blendeds)

    t_start = time.monotonic()
    if verbose:
        print(f"[init, one-pass forward] RMS = {score(cur_sum):.2f}  [{time.monotonic()-t_start:.1f}s elapsed]",
              flush=True)

    # Each outer round is a full independent re-solve per frame, so this
    # relaxation isn't guaranteed to improve monotonically -- snapshot the
    # best-scoring state seen so far and return that. Selector is
    # round_rmse + swing_weight * swing_cost(...): continuous in
    # swing_weight and exactly equal to plain RMS selection at
    # swing_weight == 0, so it can't silently discard a round's fit
    # quality over an arbitrarily tiny swing improvement (a fixed
    # scale-blind switch to a different metric for ANY nonzero
    # swing_weight was tried first and found to do exactly that -- see
    # swing_cost()'s docstring). frame_jump is still computed and printed
    # per round as a diagnostic; it no longer drives selection.
    best_score = score(cur_sum)
    best_jump = compute_frame_jump(color_idxs, ons, height, width)
    best_swing = swing_cost(color_idxs, ons) if swing_weight > 0 else 0.0
    best_selector = best_score + swing_weight * best_swing
    best_state = (list(color_idxs), list(ons), list(blendeds), cur_sum.copy())
    best_label = "init"

    round_durations = []
    for outer in range(1, outer_rounds + 1):
        round_t0 = time.monotonic()
        for i in range(n_frames):
            if verbose:
                print(f"--- outer round {outer}/{outer_rounds}, solving frame {i+1}/{n_frames} "
                      f"(seeds={seeds}) [{time.monotonic()-t_start:.1f}s elapsed total] ---", flush=True)
            other_sum = cur_sum - blendeds[i]
            this_target = n_frames * target - other_sum
            tref = PALETTE_RGB[color_idxs[0]] if i > 0 else None
            # Every frame here (including frame 0, unlike the forward
            # scheme's one-shot solve) is fit against a RESIDUAL once
            # inside outer-round refinement. The anti-grey chroma bias
            # (_fit_cost_table's docstring) isn't trustworthy against a
            # residual, so it's forced off for every frame unless the
            # caller explicitly overrode it.
            outer_kwargs = dict(kwargs)
            if "chroma_bias_scale" not in outer_kwargs:
                outer_kwargs["chroma_bias_scale"] = 0.0
            # sibling_frames = every OTHER frame's CURRENT (color_idx, on)
            # state, for solve()'s swing_weight penalty (see its
            # docstring) -- only every frame has already been solved at
            # least once (from the init pass), so this is always populated,
            # unlike solve_n_frame()'s one-shot forward scheme.
            sibling_frames = [(color_idxs[j], ons[j]) for j in range(n_frames) if j != i]
            result = solve_best_of(this_target, seeds=seeds, sigma=sigma, radius=radius,
                                    tag=f"[outer{outer} frame{i}] ", verbose=inner_verbose,
                                    temporal_ref_colours=tref, temporal_weight=temporal_weight,
                                    swing_weight=swing_weight, sibling_frames=sibling_frames,
                                    **outer_kwargs)
            color_idx_i, on_i, blended_i, rmse_i, werr_i = result
            cur_sum = cur_sum - blendeds[i] + blended_i
            color_idxs[i] = color_idx_i
            ons[i] = on_i
            blendeds[i] = blended_i

        round_durations.append(time.monotonic() - round_t0)
        round_rmse = score(cur_sum)
        round_jump = compute_frame_jump(color_idxs, ons, height, width)
        round_swing = swing_cost(color_idxs, ons) if swing_weight > 0 else 0.0
        round_selector = round_rmse + swing_weight * round_swing
        if round_selector < best_selector:
            best_selector = round_selector
            best_score = round_rmse
            best_jump = round_jump
            best_state = (list(color_idxs), list(ons), list(blendeds), cur_sum.copy())
            best_label = f"outer round {outer}"
        if verbose:
            avg_round = sum(round_durations) / len(round_durations)
            remaining_rounds = outer_rounds - outer
            eta = avg_round * remaining_rounds
            elapsed = time.monotonic() - t_start
            print(f"[outer round {outer}/{outer_rounds}] RMS = {round_rmse:.2f}  "
                  f"frame_jump = {round_jump:.2f}  "
                  f"[{elapsed:.1f}s elapsed, ~{eta:.0f}s remaining at current pace]", flush=True)

    color_idxs, ons, blendeds, cur_sum = best_state
    final_rmse = best_score
    averaged = cur_sum / n_frames
    if verbose and best_label != f"outer round {outer_rounds}":
        print(f"[cyclic] kept {best_label}'s result (RMS = {final_rmse:.2f}, "
              f"frame_jump = {best_jump:.2f}) -- later rounds didn't improve on it by the "
              f"selection metric (relaxation isn't guaranteed monotonic; see "
              f"solve_n_frame_cyclic()'s docstring)", flush=True)
    return {
        "color_idxs": color_idxs, "ons": ons, "blendeds": blendeds,
        "final_rmse": final_rmse, "averaged": averaged,
    }


def roll_seam_row_count(height, band_height, radius):
    """How many rows have a "mixed" vertical-blend neighbourhood under
    roll_scanlines() at the given band_height -- rows whose y-radius..
    y+radius window spans more than one roll-phase band, mixing row data
    from more than one original solved frame at a single instant. See
    roll_scanlines()'s docstring."""
    seam_rows = 0
    for y in range(height):
        lo, hi = max(0, y - radius), min(height - 1, y + radius)
        phases = {yy // band_height for yy in range(lo, hi + 1)}
        if len(phases) > 1:
            seam_rows += 1
    return seam_rows


def roll_scanlines(color_idxs, ons, band_height, radius=3):
    """Re-derive N "rolled" composite frames from N already-solved frames,
    so that which underlying solved frame a BAND of scanlines shows is
    staggered, instead of every scanline flipping to the same frame at the
    same instant.

    Rolling must happen in BANDS of `band_height` consecutive rows, not
    single rows: rolled[k][y] = original[(k + y // band_height) % N][y].
    solve()'s vertical blend has a real physical radius (default 3), and a
    row's solved colour/on-off pattern is only a good answer TOGETHER WITH
    its solved neighbours from the SAME frame. Rolling at 1-row granularity
    would swap in a neighbour's row from a different original frame
    (solved against a different residual target) for nearly every row --
    a systemic wrong-colours defect, not a rare seam artifact.

    Banding fixes this: any row more than `radius` rows from a band
    boundary has an entirely self-consistent blend window; only rows
    within `radius` of a boundary straddle two frames and pay that cost.
    Larger band_height means fewer boundaries but coarser decorrelation --
    see roll_seam_row_count() to quantify for your actual
    --height/--radius. No band_height eliminates the boundary cost while
    still changing anything; it just trades how much of it you pay for
    how much decorrelation.

    Per-row full-cycle coverage (temporal average, `averaged` preview,
    final RMS) is unaffected by band_height -- for any fixed row y, as k
    runs 0..N-1, (k + y // band_height) % N still visits every value
    0..N-1 exactly once. Only WITHIN-INSTANT spatial consistency changes.

    No-ops (returns inputs unchanged) for N<2."""
    n = len(color_idxs)
    if n < 2:
        return color_idxs, ons
    height = color_idxs[0].shape[0]
    rolled_color_idxs = [np.empty(height, dtype=color_idxs[0].dtype) for _ in range(n)]
    rolled_ons = [np.empty_like(ons[0]) for _ in range(n)]
    for k in range(n):
        for y in range(height):
            src = (k + y // band_height) % n
            rolled_color_idxs[k][y] = color_idxs[src][y]
            rolled_ons[k][y] = ons[src][y]
    return rolled_color_idxs, rolled_ons


def _scanline_type_block():
    """The ScanLine struct + screen-dimension #defines, emitted verbatim
    (byte-for-byte identical, own include guard distinct from any
    per-file guard) into every generated header. One shared ScanLine type
    across all generated images; only data arrays/tables get a per-image
    prefix. Because the text is identical every time, #including two
    generated headers in one translation unit is safe (second is skipped
    by the guard). Assumes a fixed --width/--height project-wide; mixing
    headers with different --width fails loudly at compile time
    (initialiser-count mismatch), not silent corruption."""
    return f"""#ifndef ATARI_SCANLINE_TYPE_H
#define ATARI_SCANLINE_TYPE_H

#define SCREEN_WIDTH  {WIDTH}
#define SCREEN_HEIGHT {HEIGHT}
#define SCREEN_BYTES_PER_ROW {BYTES_PER_ROW}

typedef struct {{
    uint8_t colour;
    uint8_t bits[SCREEN_BYTES_PER_ROW];
}} ScanLine;

#endif /* ATARI_SCANLINE_TYPE_H */
"""


def _c_string_escape(s):
    """Escape a string for embedding inside a C string literal
    (backslashes and double quotes only -- params strings are plain ASCII
    key=value text, never containing newlines)."""
    return s.replace("\\", "\\\\").replace('"', '\\"')


def build_params_string(**kwargs):
    """Build a "key=value key=value ..." string of the RESOLVED parameters
    used for a run, embedded in the generated header as <PREFIX>_PARAMS.
    Callers should pass actual effective values (e.g. resolved smoothness
    float, not "auto") so the define reflects what actually ran."""
    return " ".join(f"{k}={v}" for k, v in kwargs.items())


def pack_bits(bit_row):
    out = bytearray(BYTES_PER_ROW)
    for col, bit in enumerate(bit_row):
        if bit:
            byte_i = col // 8
            bit_i = 7 - (col % 8)
            out[byte_i] |= (1 << bit_i)
    return bytes(out)


def write_c_files(prefix, color_idx, on, params=None):
    header_path = f"{prefix}.h"
    source_path = f"{prefix}.c"
    base = prefix.split("/")[-1]
    # Sanitised to a valid C identifier fragment; prefixes the one data
    # symbol this file exports (array_name) and the per-file include
    # guard -- not the ScanLine type, shared verbatim across every
    # generated file (see _scanline_type_block()).
    ident_base = base.replace("-", "_").replace(" ", "_")
    guard = ident_base.upper() + "_H"
    array_name = f"{ident_base}_screen"
    params_define = ""
    if params is not None:
        params_define = f'#define {ident_base.upper()}_PARAMS "{_c_string_escape(params)}"\n\n'

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
 * on/off bitmap (packed MSB-first, column 0 = bit 7 of bits[0]) for that
 * single scanline. No cross-frame rolling -- single static frame; the
 * vertical colour blending is a property of the display (composite
 * bleed / scanline proximity), not of your display code.
 *
 * ScanLine is defined once and shared verbatim across every file this
 * tool generates -- only {array_name}, the actual data, is prefixed
 * per-image, so multiple images can be linked into one binary without
 * symbol collisions.
 */

{_scanline_type_block()}
{params_define}extern const ScanLine {array_name}[SCREEN_HEIGHT];

#endif /* {guard} */
""")

    with open(source_path, "w") as f:
        f.write(f'#include "{base}.h"\n\n')
        f.write(f"const ScanLine {array_name}[SCREEN_HEIGHT] = {{\n")
        for y in range(HEIGHT):
            bits = pack_bits(on[y])
            bits_fmt = ", ".join(f"0x{b:02X}" for b in bits)
            f.write(f"    {{ 0x{PALETTE_BYTES[color_idx[y]]:02X}, {{{bits_fmt}}} }}, /* row {y} */\n")
        f.write("};\n")

    return header_path, source_path


def write_c_files_n_frame(prefix, color_idxs, ons, rolled=False, roll_band_height=None,
                           params=None, frame_scheme=None):
    n_frames = len(color_idxs)
    header_path = f"{prefix}.h"
    source_path = f"{prefix}.c"
    base = prefix.split("/")[-1]
    # Sanitised to a valid C identifier fragment -- reused for the include
    # guard and as a prefix on the one symbol this file exports, so
    # multiple generated images can link into the same binary without
    # name collisions.
    ident_base = base.replace("-", "_").replace(" ", "_")
    guard = ident_base.upper() + "_H"
    table_name = f"{ident_base}_screen_frames"
    # Per-file, not SCREEN_NUM_FRAMES: unlike SCREEN_WIDTH/HEIGHT/
    # BYTES_PER_ROW (project-wide constants, see _scanline_type_block()),
    # NUM_FRAMES genuinely varies per image (a 2-frame and 3-frame image
    # can coexist in one project), so a shared name would clash across
    # two #included headers. Prefixing with this file's identifier fixes
    # that.
    num_frames_define = f"{ident_base.upper()}_NUM_FRAMES"
    params_define = ""
    if params is not None:
        params_define = f'#define {ident_base.upper()}_PARAMS "{_c_string_escape(params)}"\n\n'

    if rolled:
        frame_doc = f""" * ScanLine is defined once and shared verbatim across every file this
 * tool generates. The {n_frames} individual frame arrays are file-local
 * (static) -- the only thing this translation unit exports is
 * {table_name}, a table of {n_frames} pointers in cycle order. Index it
 * by a running frame counter (mod {num_frames_define}), same as the
 * unrolled scheme.
 *
 * IMPORTANT: --roll-scanlines was used (band height {roll_band_height}), so
 * these {n_frames} arrays are NOT the raw per-frame solves -- they are
 * already-rolled composites. Array k's row y holds original solved frame
 * (k + y // {roll_band_height}) % {n_frames}'s row y data: every
 * {roll_band_height}-row BAND of the screen is staggered to a different
 * original frame instead of the whole screen flipping in lockstep. Over
 * one full {n_frames}-frame cycle every row still visits all {n_frames}
 * original frames' data once each (pure reordering, not a re-solve), so
 * the perceived static image is unchanged; frame-to-frame change is
 * spread across bands instead of hitting the whole screen at once. Rows
 * within a band's interior have a fully self-consistent vertical-blend
 * neighbourhood; rows near a band boundary straddle two original frames'
 * data and show a locally-wrong blend there -- unavoidable, quantified
 * cost, not a bug. See roll_scanlines() in atari_scanline_blend.py and
 * roll_seam_row_count() to quantify the boundary cost for your own
 * --height/--radius/--roll-band-height combination.
 */"""
    else:
        frame_doc = f""" * ScanLine is defined once and shared verbatim across every file this
 * tool generates. The {n_frames} individual frame arrays are file-local
 * (static) -- the only thing this translation unit exports is
 * {table_name}, a table of {n_frames} pointers in cycle order. Index it
 * by a running frame counter (mod {num_frames_define}): cycle frame 0, 1,
 * ..., {n_frames - 1}, back to 0, forever. Only frame0 was solved to look
 * reasonable on its own; every later frame corrects whatever residual
 * error the frames before it left behind, so the 1/{n_frames}-weighted
 * temporal average lands closer to the source image than fewer frames
 * could manage. Do not display any frame after frame0 on its own
 * expecting it to look like the picture -- each is a correction term,
 * not a picture in its own right.
 */"""

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
{frame_doc}

#define {num_frames_define} {n_frames}

{_scanline_type_block()}
{params_define}/* e.g. {table_name}[frame_counter % {num_frames_define}] */
extern const ScanLine * const {table_name}[{num_frames_define}];

#endif /* {guard} */
""")

    with open(source_path, "w") as f:
        f.write(f'#include "{base}.h"\n\n')
        for i in range(n_frames):
            frame_name = f"screen_frame{i}"
            color_idx, on = color_idxs[i], ons[i]
            # static: file-local only. Reference via the {table_name}
            # pointer table (see .h), not by this name directly -- keeps
            # these names reusable across every generated image without
            # clashing at link time.
            f.write(f"static const ScanLine {frame_name}[SCREEN_HEIGHT] = {{\n")
            for y in range(HEIGHT):
                bits = pack_bits(on[y])
                bits_fmt = ", ".join(f"0x{b:02X}" for b in bits)
                f.write(f"    {{ 0x{PALETTE_BYTES[color_idx[y]]:02X}, {{{bits_fmt}}} }}, /* row {y} */\n")
            f.write("};\n\n")

        f.write(f"const ScanLine * const {table_name}[{num_frames_define}] = {{\n")
        for i in range(n_frames):
            f.write(f"    screen_frame{i},\n")
        f.write("};\n")

    return header_path, source_path


def save_preview(arr, path, upscale=None, width_stretch=None):
    """Render at `upscale` in both axes, then stretch width by an extra
    `width_stretch` factor to correct for the Atari's non-square pixels
    (each playfield pixel is several TV colour-clocks wide).

    upscale/width_stretch default to the CURRENT module-level UPSCALE/
    WIDTH_STRETCH, looked up at call time -- not bound as literal default
    argument values, which would go stale after main() retargets
    WIDTH_STRETCH for a custom --width."""
    if upscale is None:
        upscale = UPSCALE
    if width_stretch is None:
        width_stretch = WIDTH_STRETCH
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), mode="RGB")
    w = int(round(arr.shape[1] * upscale * width_stretch))
    h = int(round(arr.shape[0] * upscale))
    img = img.resize((w, h), Image.NEAREST)
    img.save(path)


def _smoothness_arg(s):
    """argparse type for --smoothness: accepts the literal string 'auto'
    (any case) or a plain float."""
    if s.strip().lower() == "auto":
        return "auto"
    try:
        return float(s)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"--smoothness must be 'auto' or a number, got {s!r}")


def main():
    # Force line-buffered stdout. Without this, redirecting/piping output
    # (e.g. `... > log.txt`) fully-buffers stdout, so a long run can look
    # like total silence for minutes. print() calls also pass flush=True
    # as a belt-and-braces measure.
    try:
        sys.stdout.reconfigure(line_buffering=True)
    except AttributeError:
        pass  # very old Python without reconfigure(); flush=True on prints still covers it

    ap = argparse.ArgumentParser(description="Convert an image into blended Atari 2600 scanlines.")
    ap.add_argument("image")
    ap.add_argument("-o", "--output-prefix", default=None)
    ap.add_argument("--width", type=int, default=48,
                    help="screen width in pixels/columns (default 48). Columns "
                         "are solved independently; mainly affects runtime and "
                         "horizontal detail.")
    ap.add_argument("--height", type=int, default=198,
                    help="screen height in scanlines (default 198, the Atari's "
                         "visible NTSC scanline count).")
    ap.add_argument("--sigma", type=float, default=1.0, help="Gaussian blend falloff (default 1.0)")
    ap.add_argument("--radius", type=int, default=3, help="max scanline distance blended (default 3)")
    ap.add_argument("--rounds", type=int, default=10, help="outer optimisation rounds (default 10)")
    ap.add_argument("--sweeps", type=int, default=3, help="DBS sweeps per column per round (default 3)")
    ap.add_argument("--metric", choices=["luma", "rgb"], default="luma",
                    help="error metric driving the optimiser (default luma-weighted)")
    ap.add_argument("--seeds", type=int, default=3,
                    help="random restarts, keep the best (default 3)")
    ap.add_argument("--smoothness", type=_smoothness_arg, default="auto",
                    help="strength (0-1ish) of a whole-column row-to-row consistency "
                         "preference: solves each scanline's colour in one Viterbi/DP pass "
                         "over all rows (see solve_colour_sequence()), not row-by-row, so "
                         "fit cost is a single global objective. Pushed too high, an entire "
                         "column can snap to one flat colour outright rather than degrading "
                         "gradually (SMOOTHNESS_DP_SCALE keeps the full 0-1 range inside the "
                         "safe regime). 'auto' (default) = recommended value 0.5; 0 disables "
                         "the mechanism (pure per-row nearest-palette-match).")
    ap.add_argument("--swaps", action="store_true",
                    help="enable adjacent-pixel swap moves in addition to flips (off by "
                         "default: consistently converges worse than flips alone -- see "
                         "comments in solve(). Experimental, not recommended.")
    ap.add_argument("--frames", type=int, default=1,
                    help="1 (default): a single static frame. N>1: also solve N-1 further "
                         "'correcting' frames that, cycled with the first, temporally "
                         "average (persistence of vision) to a closer match than fewer "
                         "frames could manage. Roughly N times the runtime.")
    ap.add_argument("--frame-scheme", choices=["joint", "cumulative"],
                    default="cumulative",
                    help="only meaningful with --frames > 1 ('joint' currently only "
                         "supports 3). 'cumulative' (default): frame 0 solved directly "
                         "against the real target, frame 1 against frame 0's leftover "
                         "residual, frame 2 against what's left -- full iterative "
                         "closed-form colour refinement per frame, meaningfully lower RMS "
                         "than 'joint'. Known failure mode: an awkward residual can collapse "
                         "a whole frame to flat grey or a wrong saturated hue -- check the "
                         "per-frame _raw.png previews; try 'joint' if you see that. 'joint': "
                         "every frame's colour sequence decided TOGETHER, per row, as the "
                         "best achievable average of up to 3 real palette entries (see "
                         "solve_n_frame_joint()/solve_colour_triplets()) -- can't collapse "
                         "that way, but reads flatter/less vibrant overall.")
    ap.add_argument("--frame-mode", choices=["cyclic", "forward"], default="cyclic",
                    help="only meaningful with --frames > 1. 'cyclic' (default): after the "
                         "initial --frame-scheme solve, run --outer-rounds passes of "
                         "Gauss-Seidel-style relaxation ACROSS frames -- each re-solved "
                         "against a residual from every OTHER frame's current state, "
                         "wraparound included. Fixes a real problem with the one-pass "
                         "scheme: frame-to-frame raw-pixel RMS jump can exceed the final "
                         "time-averaged RMS -- a real flicker risk on hardware (see "
                         "solve_n_frame_cyclic()'s docstring). Only applies under "
                         "--frame-scheme cumulative; under 'joint' the outer-round loop is "
                         "skipped entirely (would reintroduce the collapse/wrong-hue risk "
                         "'joint' exists to avoid), so 'cyclic'/'forward' behave identically "
                         "there. 'forward': the original one-pass scheme, no cross-frame "
                         "relaxation -- kept for comparison/debugging.")
    ap.add_argument("--outer-rounds", type=int, default=4,
                    help="only used by --frame-mode cyclic with --frame-scheme cumulative "
                         "(ignored otherwise, with a note). Number of full cyclic relaxation "
                         "passes across all frames; cost scales linearly with this.")
    ap.add_argument("--terse", action="store_true",
                    help="only meaningful with --frame-mode cyclic. Show frame-start and "
                         "outer-round-finish progress but suppress per-DBS-round SSE spam -- "
                         "a middle ground between full detail (default) and --quiet.")
    ap.add_argument("--temporal-weight", type=float, default=0.0,
                    help="only meaningful with --frames > 1. Cross-FRAME penalty biasing "
                         "frames 1..N-1 toward reusing frame 0's colour on a given row, only "
                         "where that colour is near-grey and reuse costs little fit error "
                         "(see solve_colour_sequence()'s temporal_ref docs). White/near-white "
                         "regions have the largest frame-to-frame swing (pure luminance, no "
                         "hue to mask it -- vision is more sensitive to luminance flicker "
                         "than chrominance). Default 0 (off): costs real accuracy on "
                         "photographic content, where hair/highlight greys genuinely need to "
                         "differ frame to frame. Try 2.0 for flat/cel-art content with large "
                         "uniform white areas; avoid for photographic/gradient material.")
    ap.add_argument("--max-luma-spread", type=float, default=None,
                    help="only meaningful with --frames > 1. Unlike --temporal-weight (a "
                         "bias), this hard-caps how far frames 1..N-1's colour on a row may "
                         "sit from frame 0's PERCEIVED BRIGHTNESS (PHOTOPIC_WEIGHTS, not raw "
                         "palette luma). Any hue still allowed. Default None (off); try "
                         "30-60.")
    ap.add_argument("--swing-weight", type=float, default=0.0,
                    help="only takes effect under --frame-mode cyclic (ignored, with a note, "
                         "otherwise). Penalises a pixel's displayed RGB colour swinging far "
                         "between frames (full weighted-RGB distance, not luma alone -- a grey "
                         "and a warm orange can share near-identical luma while looking nothing "
                         "alike), whichever of two ways that happens: (1) a pixel on in only "
                         "SOME of the N frames, strobing full colour/black every cycle (~50%% of "
                         "on-canvas pixels affected, unmitigated, measured on real test images); "
                         "(2) a row getting a different colour per frame while fully on every "
                         "frame -- reads as a static bright/busy band, not flicker, and is what "
                         "--frame-scheme cumulative's residual-chasing tends to do where local "
                         "target brightness changes fast. Default 0.0 (off). Very sensitive: "
                         "unlike most weights here this isn't a 0-1ish dial -- on pirate.webp "
                         "(48x128, radius=5) 0.0001 was a no-op, 0.5-1.0 gave a real, correct-hue "
                         "reduction in cross-frame RGB swing (~35-50%%) for a real RMS cost "
                         "(~5-17%%), and cost climbed steeply past that. Try 0.5-1.0 and check the "
                         "actual result -- this parameter has been wrong twice already; verify, "
                         "don't assume.")
    ap.add_argument("--roll-scanlines", action="store_true",
                    help="only meaningful with --frames > 1. Post-process the N solved "
                         "frames into N 'rolled' composites where each --roll-band-height-row "
                         "BAND of composite k is taken from a different original solved "
                         "frame, instead of the whole screen switching frames in lockstep. "
                         "Mathematically inert on the time-averaged image (pure reordering -- "
                         "see roll_scanlines()); spreads frame-to-frame change across bands "
                         "instead of hitting the whole screen at once. Rolling at 1-row "
                         "granularity would break colours almost everywhere, since the "
                         "vertical blend radius (default 3) means a row's solved colour is "
                         "only correct next to its OWN solved neighbours -- hence banding. "
                         "See --roll-band-height for the size trade-off. Doesn't change any "
                         "RMS/jump metric this tool prints, only spatial correlation -- judge "
                         "the result by eye on real hardware.")
    ap.add_argument("--roll-band-height", type=int, default=None,
                    help="only meaningful with --roll-scanlines. Rows that roll together as "
                         "one unit (default: 8x --radius, minimum 16). Rows more than "
                         "--radius from a band boundary have a fully self-consistent "
                         "vertical-blend neighbourhood; rows within --radius of a boundary "
                         "straddle two frames and show a locally-wrong blend -- smaller "
                         "band_height means finer decorrelation but more boundary rows "
                         "paying that cost; larger means coarser decorrelation, cleaner "
                         "picture. Tool prints the real affected-row count when used -- read "
                         "it before trusting the result.")
    ap.add_argument("--saturation", type=float, default=1.0,
                    help="saturation multiplier applied to the source image before resizing "
                         "(default 1.0 = unchanged; >1 boosts, <1 mutes). The reconstructed "
                         "image reads washed out even at --frames 1 -- structural to fitting "
                         "a photo through a 128-colour palette plus binary on/off plus "
                         "vertical blend against black neighbours, which pulls saturation "
                         "down. Compensate on the input rather than the display; start around "
                         "1.3-1.6 and compare against real hardware.")
    ap.add_argument("--brightness", type=float, default=1.0,
                    help="brightness multiplier applied to the source image before resizing "
                         "(default 1.0 = unchanged), same rationale as --saturation.")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    if args.width < 1 or args.height < 1:
        ap.error("--width and --height must be positive")

    # Retarget the whole module at the requested screen size. Every function
    # below reads these as globals looked up at call time, so this must
    # happen before load_target/solve/etc run.
    global WIDTH, HEIGHT, BYTES_PER_ROW, WIDTH_STRETCH
    WIDTH = args.width
    HEIGHT = args.height
    BYTES_PER_ROW = (WIDTH + 7) // 8
    WIDTH_STRETCH = 4 * (40 / WIDTH)
    prefix = args.output_prefix
    if prefix is None:
        base = args.image.rsplit("/", 1)[-1].rsplit(".", 1)[0]
        prefix = base + "_scanline"

    target = load_target(args.image, saturation=args.saturation, brightness=args.brightness)

    if args.smoothness == "auto":
        smoothness_value = auto_smoothness(target, verbose=not args.quiet)
    else:
        smoothness_value = args.smoothness

    solve_kwargs = dict(seeds=args.seeds, sigma=args.sigma, radius=args.radius,
                         rounds=args.rounds, sweeps=args.sweeps, metric=args.metric,
                         swaps=args.swaps, smoothness=smoothness_value, verbose=not args.quiet,
                         temporal_weight=args.temporal_weight, max_luma_spread=args.max_luma_spread,
                         swing_weight=args.swing_weight)

    # Resolved (not raw-argparse-sentinel) values only -- e.g. smoothness_value
    # is the actual float used, not "auto"; roll_band_height (added below
    # once resolved) is the actual band height, not None. See
    # build_params_string().
    base_params = dict(image=args.image, width=WIDTH, height=HEIGHT, sigma=args.sigma,
                        radius=args.radius, rounds=args.rounds, sweeps=args.sweeps,
                        metric=args.metric, seeds=args.seeds, smoothness=smoothness_value,
                        swaps=args.swaps, saturation=args.saturation, brightness=args.brightness,
                        frames=args.frames, temporal_weight=args.temporal_weight,
                        frame_scheme=args.frame_scheme, frame_mode=args.frame_mode,
                        outer_rounds=args.outer_rounds if args.frames > 1 else None)

    if args.frames == 1:
        color_idx, on, blended, rmse, werror = solve_best_of(target, **solve_kwargs)

        params_str = build_params_string(**base_params)
        header_path, source_path = write_c_files(prefix, color_idx, on, params=params_str)

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
        if args.frame_scheme == "joint" and args.frames != 3:
            ap.error("--frame-scheme joint currently only supports --frames 3 "
                     "(frame count is fixed project-wide anyway); pass "
                     "--frame-scheme cumulative to use another frame count")

        if args.swing_weight and args.frame_mode != "cyclic" and not args.quiet:
            print(f"(--swing-weight={args.swing_weight} ignored: only takes effect "
                  f"under --frame-mode cyclic, see solve()'s docstring)", flush=True)

        if args.frame_mode == "cyclic":
            result = solve_n_frame_cyclic(target, n_frames=args.frames,
                                           outer_rounds=args.outer_rounds,
                                           terse=args.terse,
                                           frame_scheme=args.frame_scheme,
                                           **solve_kwargs)
        elif args.frame_scheme == "joint":
            result = solve_n_frame_joint(target, n_frames=args.frames, **solve_kwargs)
        else:
            result = solve_n_frame(target, n_frames=args.frames, **solve_kwargs)
        color_idxs, ons = result["color_idxs"], result["ons"]

        roll_band_height = None
        if args.roll_scanlines:
            roll_band_height = args.roll_band_height if args.roll_band_height is not None \
                else max(16, 8 * args.radius)
            seam_rows = roll_seam_row_count(HEIGHT, roll_band_height, args.radius)
            color_idxs, ons = roll_scanlines(color_idxs, ons, roll_band_height, radius=args.radius)
            if not args.quiet:
                print(f"Rolled scanlines: band height {roll_band_height} rows. "
                      f"{seam_rows}/{HEIGHT} rows ({100*seam_rows/HEIGHT:.0f}%) have a "
                      f"mixed-frame blend window near a band boundary and will show a "
                      f"locally-wrong blend there -- see roll_scanlines() docstring.",
                      flush=True)

        params_str = build_params_string(**base_params, roll_scanlines=args.roll_scanlines,
                                          roll_band_height=roll_band_height)
        header_path, source_path = write_c_files_n_frame(prefix, color_idxs, ons,
                                                          rolled=args.roll_scanlines,
                                                          roll_band_height=roll_band_height,
                                                          params=params_str,
                                                          frame_scheme=args.frame_scheme)

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
            if args.roll_scanlines:
                note = "  (rolled composite -- diagonal patchwork of all N solves by design)"
            elif args.frame_scheme == "cumulative":
                note = "" if i == 0 else "  (correction layer, not a picture on its own)"
            else:
                note = ""  # joint scheme: every frame is a real, directly-computed frame
            print(f"Wrote {raw_path}  (raw unblended TIA pixels, frame {i}){note}")

        if result.get("rmse_first") is not None:
            print(f"1-frame RMS would have been: {result['rmse_first']:.2f}")
        rms_label = "cyclic" if args.frame_mode == "cyclic" else "alternating"
        print(f"{args.frames}-frame {rms_label} RMS (unweighted RGB, 0-255 scale): {result['final_rmse']:.2f}")

        # Rolling is a pure reordering (per-pixel change magnitude is the
        # same either way; only spatial correlation changes, which this
        # scalar metric can't show), so it's fine to compute post-roll.
        jump = compute_frame_jump(color_idxs, ons, HEIGHT, WIDTH)
        print(f"Mean frame-to-frame raw-pixel RMS jump: {jump:.2f}  (flicker-magnitude "
              f"metric -- compare to the RMS above; if it's much bigger, most of what's "
              f"happening is high-contrast strobing, not gentle blending)")


if __name__ == "__main__":
    main()
