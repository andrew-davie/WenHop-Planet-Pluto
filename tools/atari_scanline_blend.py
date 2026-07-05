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
                                     [--smoothness auto|0.5] [--swaps]
                                     [--frames 1|2]

Screen is WIDTH x HEIGHT, configurable via --width/--height (default 48x198).
Source images are assumed to already be at the correct display aspect ratio
before this tool is called -- it does a plain resize, no letterboxing or
cropping. If you change --width, you likely need to adjust WIDTH_STRETCH's
underlying assumption too (see comment above WIDTH_STRETCH) to keep preview
PNGs' aspect ratio honest.
"""
import argparse
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

# Loosely based on ITU-R BT.601 luma coefficients (0.299/0.587/0.114) --
# used to weight R/G/B error so the optimiser spends its on/off budget on
# differences the eye actually notices, rather than treating all three
# channels as equally important. The blue weight is bumped well above the
# literal BT.601 value (0.114 -> 0.25): confirmed on real hardware (and
# reproduced/diagnosed here) that the literal coefficient makes blue error
# nearly free in the palette-snap step, which doesn't matter for BT.601's
# original purpose (continuous-tone video, where an isolated channel error
# gets masked by surrounding detail) but is a real problem here, where a
# single wrong hue is a whole discrete, isolated scanline colour with
# nothing to mask it -- e.g. a scanline that should read as flesh tone
# snapping to a garish blue/purple palette entry because that entry
# matched R/G slightly better and blue barely counted against it. Verified
# directly: reproduced the exact anomalous rows on a real photo, confirmed
# the palette snap was choosing the literal luma-weighted argmin correctly
# (not a bug in the snap logic), and confirmed raising the blue weight to
# 0.25 eliminates the anomaly on every test image tried (lena, parrot,
# reef) while leaving overall RMS the same or slightly better, not worse.
LUMA_WEIGHTS = np.array([0.299, 0.587, 0.25])
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


SMOOTHNESS_MARGIN_SCALE = 600.0

# Channel-spread (max-min, 0-255 scale) below which a colour counts as
# "achromatic" for the cross-frame temporal-stability term below; see
# achromatic_factor() and the temporal_ref/temporal_weight params on
# nearest_palette_index_smooth().
ACHROMATIC_SATURATION_THRESHOLD = 40.0

# Same-units-as-d scale for how quickly the temporal-stability term backs
# off as reusing temporal_ref's exact colour costs more fit error; see the
# "reuse_cost"/"reuse_ok" computation in nearest_palette_index_smooth().
# temporal_weight defaults to 0 (the whole mechanism is inert) precisely
# because no single scale value here was found safe for every image --
# 500 cost lena real RMS for a small donald win, 3000 gave donald its best
# measured improvement but fully reproduced lena's regression at
# temporal_weight>0. Since this is opt-in (temporal_weight=0 by default,
# see solve()), tuned for the case it's meant for: 3000 is the value
# validated against donald's white-belly flicker specifically. Don't
# assume it's safe on photographic content -- it measurably isn't.
TEMPORAL_REUSE_COST_SCALE = 3000.0

def nearest_palette_index_smooth(colour, weights, neighbours, smoothness,
                                  margin_scale=SMOOTHNESS_MARGIN_SCALE,
                                  temporal_ref=None, temporal_weight=0.0):
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
    error actually justifies one.

    `smoothness` is only ever applied at a fraction of its face value: this
    line's own weighted distance `d` is sorted, and the gap between the
    best and second-best palette candidates (the "margin", in the same
    squared-distance units as `d` itself) sets how much of `smoothness` is
    actually used -- full strength when the margin is near zero (a genuine
    near-tie, exactly the case this was built for), decaying toward zero
    as the margin grows (a line that already has a clear, decisive best
    match on its own).

    This replaces an earlier version that applied `smoothness` at face
    value on every line regardless of margin, and a version after that
    which tried to guess a single smoothness value for an entire image
    from its background-fill fraction (auto_smoothness(), now unused).
    Both were wrong for the same underlying reason: an image can contain
    BOTH regimes at once. Directly confirmed on a cel-art test image (a
    Donald Duck cutout on a black background) -- some rows are a large,
    decisive, saturated flat colour fill (jacket blue: margin in the
    thousands) while others are ambiguous near-flat shading/outline
    transitions only a few rows away (margin near zero) -- so ANY single
    smoothness constant for the whole image is wrong somewhere: high
    enough to fix the ambiguous rows and the decisive fill rows get
    dragged into a false, sticky consensus with a wrong neighbour and
    lock there (confirmed: solid blue jacket rendered flat grey); low
    enough to leave the decisive rows alone and the ambiguous rows revert
    to the original chattering-hue-noise defect (confirmed on real
    hardware: reintroduced wrong-hue speckling everywhere except the
    jacket once smoothness was dropped to fix the jacket). Scaling by
    each row's OWN margin fixes both at once without knowing anything
    about image content: donald.jpg outliers (see the outlier-row check
    used throughout this project's real-hardware debugging) went from
    56-58/128 per frame (smoothness=0, jacket correct but rest chattering)
    or effectively the whole jacket wrong (smoothness=0.5 flat) down to
    2-13/128 per frame at margin_scale=300, then 3-9/128 at margin_scale=600
    -- and lena/toystory/skin (the photographic test set) stayed at zero
    outliers throughout with RMS unchanged or slightly improved, not
    regressing, at every margin_scale tried. 600 (raised from an initial
    300) was needed to also clear a smaller but real second symptom: on
    the same donald image, a smooth blue-jacket-to-white-body transition
    (a gradient, not a hard edge) still let an isolated wrong hue
    (a saturated orange, 0x3A) through at 300 in the cumulative scheme's
    later correcting frames specifically -- 600 removed it. This is a
    tuned constant, not a derived one -- revisit if a future image breaks
    it, and don't assume 600 is a ceiling; keep raising it if a similar
    smooth-gradient region keeps producing isolated wrong-hue rows,
    checking lena/toystory/skin after each raise to confirm the
    photographic case still isn't paying a real cost.

    `temporal_ref`/`temporal_weight` are a separate, later addition
    addressing a different complaint: on a multi-frame sequence, white
    and near-grey rows were reported as visibly flickerier on real
    hardware than coloured ones, even after outlier rows (wrong hue
    entirely) were fixed. Measured directly: white/near-white regions
    had the LARGEST absolute frame-to-frame colour swing of any region
    tested (~181 RGB-units, vs ~160 for a saturated blue jacket and ~135
    for yellow), because achromatic content has only ~8 discrete grey
    palette rungs to hit a target brightness with -- no hue axis to help
    -- so independently-solved frames tend to hop between adjacent rungs
    to average out right. That swing is also perceptually worse than a
    same-sized swing elsewhere: human vision is well-documented to be
    far more sensitive to LUMINANCE flicker than to CHROMINANCE flicker
    at a given temporal rate (the physical basis for NTSC/PAL using much
    less colour bandwidth than luminance bandwidth without looking
    worse) -- so a grey rung-hop reads as real flicker where a similar
    hue wobble in a saturated area mostly wouldn't.

    The fix is a second cross-*frame* (not cross-row) penalty: pass the
    colour an earlier-solved frame already committed to this same row as
    `temporal_ref`, and a row that's currently being fit to something
    near-grey will be pulled toward reusing that exact rung rather than
    drifting to an adjacent one.

    First attempt gated this purely on achromatic_factor(temporal_ref)
    and it was a real regression, not just a smaller win than hoped:
    tested on lena (2-frame), RMS went 27.4 -> 34.5 even at a fairly low
    temporal_weight, and stayed there however low the achromatic
    threshold was set. Root cause: lena has meaningfully-sized grey
    regions (hair highlights etc) that are grey *and* need frame 1 to
    commit to a genuinely different shade than frame 0 to properly close
    the residual gap -- that's the entire reason a second frame exists.
    Forcing those rows to reuse frame 0's rung stopped frame 1 correcting
    them at all. Donald's white belly is a different case: a huge, flat,
    near-uniform region where many adjacent grey rungs are all roughly
    equally good fits for either frame, so reusing frame 0's pick costs
    almost nothing -- but "achromatic" alone can't tell these two cases
    apart.

    Second attempt gated on the SAME margin used for the spatial
    smoothness term (best-vs-second-best distance among this row's own
    candidates), reasoning that a near-tie means reuse is free. Also
    didn't fix lena -- because that's the wrong question. A tie among
    this row's own candidates says nothing about how far frame 0's
    SPECIFIC colour is from any of them; lena's hair highlights can have
    several closely-tied candidates that are ALL far from frame 0's
    (different, also-reasonable) pick, so margin alone still let a
    costly reuse through.

    What actually works: directly compute what it costs to reuse
    temporal_ref instead of this row's own best answer -- the distance
    from `colour` to temporal_ref specifically, minus the distance to the
    true best candidate. Near zero cost means temporal_ref IS basically
    the best answer anyway (donald's belly: reuse is free). A large cost
    means temporal_ref is a poor fit here regardless of what else is tied
    (lena's hair: frame 1 genuinely needs to differ), and the term backs
    off. Verified: lena's RMS returned to its temporal_weight=0 baseline
    (this is now near-inert on lena, as it should be) while donald's
    belly fix is unaffected (its reuse cost is near zero, so the term
    fires at full strength there)."""
    d = np.sum(weights * (PALETTE_RGB - colour) ** 2, axis=1)
    if smoothness > 0 and neighbours:
        part = np.partition(d, 1)
        margin = part[1] - part[0]
        effective_smoothness = smoothness * (margin_scale / (margin_scale + margin))
        for nb in neighbours:
            d = d + effective_smoothness * np.sum((PALETTE_RGB - nb) ** 2, axis=1)
    if temporal_ref is not None and temporal_weight > 0:
        d_own = np.sum(weights * (PALETTE_RGB - colour) ** 2, axis=1)
        d_best = d_own.min()
        ref_idx = int(np.argmin(np.sum((PALETTE_RGB - temporal_ref) ** 2, axis=1)))
        reuse_cost = d_own[ref_idx] - d_best
        reuse_ok = TEMPORAL_REUSE_COST_SCALE / (TEMPORAL_REUSE_COST_SCALE + reuse_cost)
        eff_temporal = temporal_weight * achromatic_factor(temporal_ref) * reuse_ok
        if eff_temporal > 0:
            d = d + eff_temporal * np.sum((PALETTE_RGB - temporal_ref) ** 2, axis=1)
    return int(np.argmin(d))


def achromatic_factor(rgb, threshold=ACHROMATIC_SATURATION_THRESHOLD):
    """1.0 for a fully grey/white colour (max channel == min channel),
    ramping linearly down to 0.0 once the channel spread reaches
    `threshold`. Used to gate the cross-frame temporal-stability term in
    nearest_palette_index_smooth() so it only fires for near-achromatic
    rows -- see that function's temporal_ref/temporal_weight params for
    why grey specifically needs this and colour doesn't."""
    spread = float(np.max(rgb) - np.min(rgb))
    return max(0.0, min(1.0, 1.0 - spread / threshold))


def load_target(path, saturation=1.0, brightness=1.0):
    """Resample to the screen's native WIDTHxHEIGHT. The source is assumed
    to already be at the correct display aspect ratio (see WIDTH_STRETCH)
    -- this does a plain resize, no letterboxing/cropping.

    saturation/brightness (both default 1.0 = no change) are applied to
    the full-resolution source BEFORE resizing, via PIL's ImageEnhance --
    a compensating push in the other direction from real hardware, since
    the 128-colour palette plus binary on/off plus vertical blend against
    black neighbours structurally pulls the reconstructed image's
    perceived brightness/saturation down from the source's (confirmed on
    real hardware: identical washed-out look at --frames 1, so it's not a
    multi-frame temporal-averaging artifact -- it's inherent to fitting a
    photographic image through this constrained representation). Rather
    than accept that, or rely on adjusting the physical display (which
    the user explicitly wants to avoid touching), boost the input before
    the optimiser ever sees it, so its target is already closer to what
    needs to land on screen."""
    img = Image.open(path).convert("RGB")
    if saturation != 1.0:
        img = ImageEnhance.Color(img).enhance(saturation)
    if brightness != 1.0:
        img = ImageEnhance.Brightness(img).enhance(brightness)
    img = img.resize((WIDTH, HEIGHT), Image.LANCZOS)
    return np.asarray(img, dtype=np.float64)  # (HEIGHT, WIDTH, 3)


def estimate_background_fraction(target, luma_threshold=24.0):
    """Fraction of pixels in the resized target that are near-black
    (off-canvas/background). Kept for inspection/debugging; no longer
    used by the smoothness mechanism itself -- see auto_smoothness()."""
    luma = target.mean(axis=-1)
    return float((luma < luma_threshold).mean())


def auto_smoothness(target, ceiling=0.5, bg_threshold=0.3, floor=0.0, verbose=True):
    """RETIRED as the active smoothness mechanism -- kept only so old calls
    don't break, and because the history here is instructive. Always
    returns `ceiling` now (default 0.5) rather than doing background-based
    detection; the real fix lives in nearest_palette_index_smooth()'s
    per-row margin scaling instead. Read on for why this approach itself
    had to be abandoned, not just retuned.

    This function's original idea: classify the WHOLE image as either
    "photographic" (needs smoothness=0.5 to stop flesh-tone-style hue
    chatter) or "sparse cel-art cutout on black" (needs smoothness=0,
    because a Donald Duck test image showed 0.5 locking a large flat blue
    region into flat grey), using background-pixel fraction as the
    classifier. That fixed the Donald Duck jacket -- and then broke on
    the very same image: with smoothness forced to ~0 for the whole
    frame, the jacket was correct but every ambiguous near-flat shading
    and outline region elsewhere in that same picture reverted to the
    original chattering-hue defect (confirmed on real hardware: 56-58 out
    of 128 rows per frame were hue-family outliers). Donald Duck isn't
    uniformly one regime or the other -- it contains large, decisive,
    saturated flat-fill rows (margin in the thousands) directly next to
    ambiguous near-tied ones (margin near zero), sometimes a handful of
    rows apart. No single whole-image constant, however cleverly chosen,
    can be right for both at once. The fix had to move from "one number
    per image" to "one number per row, based on that row's own fit
    quality" -- see nearest_palette_index_smooth()."""
    bg_frac = estimate_background_fraction(target)
    if verbose:
        print(f"[smoothness] background fraction = {bg_frac:.1%} (informational only -- "
              f"per-row margin scaling in nearest_palette_index_smooth() does the real "
              f"work now); using ceiling = {ceiling}", flush=True)
    return ceiling


def make_kernel(sigma, radius):
    d = np.arange(-radius, radius + 1)
    w = np.exp(-(d.astype(np.float64) ** 2) / (2.0 * sigma * sigma))
    return {int(k): float(v) for k, v in zip(d, w)}


def solve(target, sigma=1.0, radius=3, rounds=10, sweeps=3, seed=0,
          metric="luma", swaps=False, smoothness=0.5, verbose=True, label="",
          temporal_ref_colours=None, temporal_weight=0.0):
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
    #
    # The average is taken over non-near-black pixels only, not the whole
    # row. Found by direct inspection of a real failure: on a row that's
    # mostly black canvas with a small saturated-colour subject (e.g. 15
    # of 48 columns are a blue glove, the rest black background), the
    # WHOLE-row average is dark and desaturated enough that it's a
    # genuinely decisive (large-margin) match for a dark grey palette
    # entry -- not a close tie with blue, a confident win for the wrong
    # answer. That's a different failure from the near-tie/chatter defect
    # smoothness (and its per-row margin scaling, see
    # nearest_palette_index_smooth) exists to fix, and scaling smoothness
    # by margin can't rescue it: the row's own fit genuinely isn't
    # ambiguous, it's just being asked the wrong question. The later
    # per-round colour-refinement step already fits on-pixels only
    # (correctly), but a bad enough initial guess can still get stuck
    # (the on/off pattern co-adapts to the wrong initial colour before
    # refinement gets a chance to correct it). Excluding near-black
    # pixels from the AVERAGE up front -- a cheap proxy for "pixels likely
    # to end up on" -- gives the initial guess a fair start instead of
    # diluting it with background. Falls back to the whole-row average
    # for genuinely all-background rows (nothing to exclude to).
    color_idx = np.zeros(HEIGHT, dtype=np.int32)
    colour = np.zeros((HEIGHT, 3), dtype=np.float64)
    for y in range(HEIGHT):
        row = target[y]
        likely_on = row.sum(axis=-1) > 30.0
        avg = row[likely_on].mean(axis=0) if likely_on.any() else row.mean(axis=0)
        neighbours = [colour[y - 1]] if y > 0 else []
        tref = temporal_ref_colours[y] if temporal_ref_colours is not None else None
        color_idx[y] = nearest_palette_index_smooth(avg, weights, neighbours, smoothness,
                                                      temporal_ref=tref, temporal_weight=temporal_weight)
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
            tref = temporal_ref_colours[y] if temporal_ref_colours is not None else None
            new_idx = nearest_palette_index_smooth(new_continuous, weights, neighbours, smoothness,
                                                     temporal_ref=tref, temporal_weight=temporal_weight)
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
    changes, frame by frame. This is a CUMULATIVE scheme: frame i is always
    solved as if it were the *last* frame of an (i+1)-frame sequence, i.e.
    to fully close whatever gap the already-committed frames 0..i-1 left
    behind, targeting a running average of exactly `target` after i+1
    frames:

        cumulative = 0                             # sum of committed frames so far
        for i in 0..n-1:
            this_target = (i+1)*target - cumulative    # close the gap fully, assuming i is last
            solve frame i to fit this_target -> blended_i
            cumulative += blended_i

    This makes the sequence prefix-compatible: frame i's target depends
    only on frames 0..i-1 and i itself, never on how many frames come
    after it. So a 3-frame run's frame0 and frame1 are byte-identical to a
    standalone 2-frame run's (same seeds) -- you can ship a 2-frame result
    now and bolt on a 3rd correcting frame later without regenerating or
    recompiling the earlier ones. An earlier version of this function
    instead redistributed the remaining gap evenly across all frames still
    to come (so frame1 of a 3-frame run only closed half the gap, saving
    the rest for frame2); that version is NOT prefix-compatible, since
    frame1's target depended on the total frame count. Tested both on a
    real photo at matching settings and their final RMS was statistically
    indistinguishable (within seed noise, ~0.2% apart, sign flipping
    depending on seed) -- so this scheme's extensibility is free, not a
    quality trade-off.

    Frame 0's target is (0+1)*target - 0 == target exactly, so it's just
    the ordinary single-frame fit (and its own RMS-vs-target is a fair
    "what if we'd stopped after 1 frame" baseline).

    `temporal_weight` (default 0, off) adds a cross-frame stability term:
    frames 1..n-1 are biased toward reusing frame 0's already-committed
    colour on a given row, but ONLY where that colour is near-achromatic
    (see nearest_palette_index_smooth()'s temporal_ref/temporal_weight
    docs for the full rationale -- this exists to stop white/grey areas
    flickering between adjacent palette rungs across the cycling frames,
    a defect confirmed on real hardware and measurably the largest
    frame-to-frame colour swing of any region tested). Anchoring
    everything to frame 0 specifically (not "the previous frame") keeps
    this prefix-compatible: frame 0 is identical regardless of n_frames,
    so every later frame's anchor is too.
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
        result = solve_best_of(this_target, seeds=seeds, sigma=sigma, radius=radius,
                                tag=f"[frame{i}] ", temporal_ref_colours=tref,
                                temporal_weight=temporal_weight, **kwargs)
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


def roll_seam_row_count(height, band_height, radius):
    """How many rows have a "mixed" vertical-blend neighbourhood under
    roll_scanlines() at the given band_height -- i.e. rows whose
    y-radius..y+radius window spans more than one roll-phase band, and
    therefore mixes row data from more than one original solved frame at
    a single displayed instant. See roll_scanlines()'s docstring for why
    this number matters and isn't just informational trivia."""
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
    staggered, instead of every scanline in the whole screen flipping to
    the same frame at the same instant.

    THIS FUNCTION WAS WRONG IN AN EARLIER VERSION -- rolling at 1-row
    granularity (rolled[k][y] = original[(k + y) % N][y] for every single
    y) was shipped, tested "correct" against its own reordering formula,
    and reported as inert on the perceived average. All of that was true
    and verified -- and still resulted in visibly wrong colours on real
    hardware. Root cause, found by re-examining the actual display model
    rather than trusting the byte-level check alone: solve()'s vertical
    blend has a real physical radius (default 3, i.e. every displayed row
    is smeared with a Gaussian-weighted mix of the 3 rows above and below
    it -- composite bleed / scanline proximity, the whole basis this
    tool's colour choices are optimised against). A row's solved colour
    and on/off pattern are only a good answer TOGETHER WITH its solved
    neighbours from the SAME frame -- that's what the solver actually
    targeted. Swap in a neighbour's row from a DIFFERENT original frame
    (solved against a different residual target) and the physical blend
    the real hardware produces was never optimised for that combination
    at all. At 1-row granularity, with radius=3 (a 7-row window) and only
    3 possible frames, essentially every row on screen has at least one
    neighbour from a different frame within its blend window --
    confirmed directly: 128/128 rows have a mixed-frame blend window at
    band_height=1, for both radius=1 and radius=3. That's not a rare seam
    artifact, it's systemic -- which matches "colours are incorrect"
    being an immediate, general complaint rather than a subtle one.

    The fix: roll in BANDS of `band_height` consecutive rows, not single
    rows -- rolled[k][y] = original[(k + y // band_height) % N][y]. Any
    row more than `radius` rows away from a band boundary has an entirely
    self-consistent blend window (every row in it belongs to the same
    original frame, exactly matching what the solver assumed); only rows
    within `radius` of a boundary still straddle two frames and pay the
    same cost the old version paid everywhere. Larger band_height means
    fewer boundaries and fewer affected rows, at the cost of coarser
    (less spatially-scattered) decorrelation; see roll_seam_row_count()
    to quantify this trade-off for your actual --height/--radius rather
    than guessing, and the --roll-band-height help text for measured
    examples at height=128, radius=3 (1: 100%% of rows affected, 7: 84%%,
    14: 41%%, 32: 14%%, 64: 5%%). There is no band_height that gets this
    to zero while still changing anything -- some boundary cost is the
    unavoidable price of this technique; band_height just controls how
    much of it you pay for how much decorrelation.

    Per-row full-cycle coverage (and therefore the temporal average, the
    `averaged` preview, and the final RMS this tool reports) is unaffected
    by band_height -- for any fixed row y, as k runs 0..N-1, (k + y //
    band_height) % N still visits every value 0..N-1 exactly once. Only
    the WITHIN-INSTANT spatial consistency changes, which is exactly the
    thing that broke before.

    No-ops (returns the inputs unchanged) for N<2, since there's nothing
    to stagger with a single frame."""
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
    (byte-for-byte identical, using its OWN include guard distinct from
    any per-file guard) into every generated header. This is deliberate:
    every image this tool generates shares the same data format (one
    colour byte + one SCREEN_WIDTH-bit on/off bitmap per row), so there's
    exactly one ScanLine type, not one per image -- only the actual data
    arrays/tables get a per-image prefix. Because the text is identical
    every time, #including two generated headers in the same translation
    unit is safe: the first one defines ATARI_SCANLINE_TYPE_H and the
    type, the second is skipped by the guard rather than conflicting.
    This assumes a fixed --width/--height across a given project (true in
    practice); mixing headers generated with genuinely different --width
    will fail loudly at compile time (initialiser-count mismatch against
    whichever ScanLine layout won), not silently corrupt data."""
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
    base = prefix.split("/")[-1]
    # Sanitised to a valid C identifier fragment. Only used to prefix the
    # one data symbol this file exports (array_name below) and the
    # per-file include guard -- NOT the ScanLine type, which is shared
    # verbatim across every generated file (see _scanline_type_block()).
    ident_base = base.replace("-", "_").replace(" ", "_")
    guard = ident_base.upper() + "_H"
    array_name = f"{ident_base}_screen"

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
 *
 * ScanLine is defined once and shared verbatim across every file this
 * tool generates -- only {array_name}, the actual data, is prefixed
 * per-image, so multiple images can be linked into one binary without
 * symbol collisions while still sharing a single common data format.
 */

{_scanline_type_block()}
extern const ScanLine {array_name}[SCREEN_HEIGHT];

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


def write_c_files_n_frame(prefix, color_idxs, ons, rolled=False, roll_band_height=None):
    n_frames = len(color_idxs)
    header_path = f"{prefix}.h"
    source_path = f"{prefix}.c"
    base = prefix.split("/")[-1]
    # Sanitised to a valid C identifier fragment -- reused both for the
    # include guard and as a prefix on the one symbol this file actually
    # exports, so multiple generated images can be linked into the same
    # binary without name collisions.
    ident_base = base.replace("-", "_").replace(" ", "_")
    guard = ident_base.upper() + "_H"
    table_name = f"{ident_base}_screen_frames"

    if rolled:
        frame_doc = f""" * ScanLine is defined once and shared verbatim across every file this
 * tool generates. The {n_frames} individual frame arrays are file-local
 * (static) -- the only thing this translation unit exports is
 * {table_name}, a table of {n_frames} pointers in cycle order. Index it
 * by a running frame counter (mod SCREEN_NUM_FRAMES) exactly as usual --
 * the display-side code does not change at all versus the unrolled
 * scheme.
 *
 * IMPORTANT: --roll-scanlines was used (band height {roll_band_height}), so
 * these {n_frames} arrays are NOT the raw per-frame solves -- they are
 * already-rolled composites. Array k's row y holds original solved frame
 * (k + y // {roll_band_height}) %% {n_frames}'s row y data: every
 * {roll_band_height}-row BAND of the screen is staggered to a different
 * original frame, instead of the whole screen flipping between frames in
 * lockstep. Over one full {n_frames}-frame cycle every row still visits
 * all {n_frames} original frames' data once each (same 1/{n_frames}-weighted
 * temporal average as always -- this is a pure reordering, not a
 * re-solve), so the perceived static image is unchanged; what's
 * different is that frame-to-frame change is spread across bands instead
 * of hitting the whole screen at the same instant. Rows within a band's
 * interior have a fully self-consistent vertical-blend neighbourhood
 * (matching what the solver assumed); rows near a band boundary
 * straddle two original frames' data and will show a locally-wrong
 * blend there -- that's an unavoidable, quantified cost of this
 * technique, not a bug (an EARLIER version of this tool rolled at
 * 1-row granularity, which put ~100%% of rows in that "wrong blend"
 * state -- visibly broken colours, found and fixed). See
 * roll_scanlines() in atari_scanline_blend.py for the full history and
 * roll_seam_row_count() to quantify the boundary cost for your own
 * --height/--radius/--roll-band-height combination.
 */"""
    else:
        frame_doc = f""" * ScanLine is defined once and shared verbatim across every file this
 * tool generates. The {n_frames} individual frame arrays are file-local
 * (static) -- the only thing this translation unit exports is
 * {table_name}, a table of {n_frames} pointers in cycle order. Index it
 * by a running frame counter (mod SCREEN_NUM_FRAMES): cycle frame 0, 1,
 * ..., {n_frames - 1}, back to 0, forever. Only frame0 was solved to look
 * reasonable on its own; every later frame was solved to correct whatever
 * residual error the frames before it left behind, so that the
 * 1/{n_frames}-weighted temporal average the eye perceives from the full
 * cycle (combined with the usual vertical scanline blending within each
 * individual frame) lands closer to the source image than fewer frames
 * could manage. Do not display any frame after frame0 on its own
 * expecting it to look like the picture -- it won't; each is a
 * correction term, not a picture in its own right.
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

#define SCREEN_NUM_FRAMES {n_frames}

{_scanline_type_block()}
/* e.g. {table_name}[frame_counter % SCREEN_NUM_FRAMES] */
extern const ScanLine * const {table_name}[SCREEN_NUM_FRAMES];

#endif /* {guard} */
""")

    with open(source_path, "w") as f:
        f.write(f'#include "{base}.h"\n\n')
        for i in range(n_frames):
            frame_name = f"screen_frame{i}"
            color_idx, on = color_idxs[i], ons[i]
            # static: file-local only. Nothing outside this translation
            # unit should ever reference an individual frame array by
            # name -- go through the {table_name} pointer table instead
            # (see the .h). Keeps these names free to reuse identically
            # across every image this tool generates without clashing at
            # link time.
            f.write(f"static const ScanLine {frame_name}[SCREEN_HEIGHT] = {{\n")
            for y in range(HEIGHT):
                bits = pack_bits(on[y])
                bits_fmt = ", ".join(f"0x{b:02X}" for b in bits)
                f.write(f"    {{ 0x{PALETTE_BYTES[color_idx[y]]:02X}, {{{bits_fmt}}} }}, /* row {y} */\n")
            f.write("};\n\n")

        f.write(f"const ScanLine * const {table_name}[SCREEN_NUM_FRAMES] = {{\n")
        for i in range(n_frames):
            f.write(f"    screen_frame{i},\n")
        f.write("};\n")

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
    # Force line-buffered (or at least promptly-flushed) stdout. Without
    # this, redirecting/piping output (e.g. `... > log.txt`, or running
    # under a wrapper that isn't a TTY) makes Python fully-buffer stdout,
    # so nothing appears until the buffer fills or the process exits --
    # on a long thorough run that can look like total silence for many
    # minutes even though it's working. The individual print() calls also
    # pass flush=True as a belt-and-braces measure.
    try:
        sys.stdout.reconfigure(line_buffering=True)
    except AttributeError:
        pass  # very old Python without reconfigure(); flush=True on prints still covers it

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
    ap.add_argument("--smoothness", type=_smoothness_arg, default="auto",
                    help="CEILING (in squared-RGB-distance units) for a scanline "
                         "colour-choice penalty against its already-committed "
                         "neighbours; stops smooth photographic gradients turning "
                         "into jittery hue-shifting noise. History: 0.01 -> 0.05 -> "
                         "0.5, each bump forced by real-hardware evidence that the "
                         "previous value let 10-60%% of scanlines in a real photo "
                         "snap to a wildly wrong, saturated hue in near-flat, "
                         "low-saturation regions (confirmed by decoding actual "
                         "compiled hardware bytes, not a synthetic guess) -- and "
                         "because frames display *cyclically*, not pre-averaged, "
                         "those wrong rows strobe as a visible coloured line on a "
                         "real screen even though a static blended preview looks "
                         "fine. But 0.5 applied flat, every row, then broke a "
                         "different kind of content: on a flat cel-art cutout "
                         "(Donald Duck on a black background), it locked a large "
                         "solid blue jacket into flat grey. The real fix wasn't a "
                         "smarter constant -- 'auto' background-fraction detection "
                         "was tried and failed, because a single image (that same "
                         "Donald Duck picture) can contain a decisive flat-fill row "
                         "sitting a few rows from an ambiguous near-tied one, so no "
                         "single whole-image value is ever right everywhere. The "
                         "penalty is now scaled PER ROW by how tied that row's own "
                         "best-vs-second-best palette match is (see "
                         "nearest_palette_index_smooth()) -- full strength on a "
                         "genuine near-tie, fading to ~0 on a row that's already "
                         "decisive on its own, so this ceiling is safe to leave "
                         "high without the lock-in risk. 'auto' (default) just "
                         "means 'use the recommended ceiling, 0.5' -- the per-row "
                         "adaptation happens regardless of what you pass here. "
                         "Pass an explicit number to change the ceiling itself; "
                         "0 disables the mechanism entirely.")
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
    ap.add_argument("--temporal-weight", type=float, default=0.0,
                    help="only meaningful with --frames > 1. Cross-FRAME (not "
                         "cross-row) penalty that biases frames 1..N-1 toward "
                         "reusing frame 0's already-committed colour on a given "
                         "row, but only where that colour is near-grey AND "
                         "reusing it costs this row's own fit little (see "
                         "nearest_palette_index_smooth()'s temporal_ref docs). "
                         "Exists because white/near-white regions were found to "
                         "have the LARGEST frame-to-frame colour swing of any "
                         "region tested, and -- being pure luminance with no hue "
                         "to mask it -- read as visibly flickerier on real "
                         "hardware than a similar-sized swing in a coloured area "
                         "(human vision is well-documented to be far more "
                         "sensitive to luminance flicker than chrominance "
                         "flicker at a given rate). Default 0 (off): tested and "
                         "found a REAL cost on photographic content -- lena's "
                         "RMS got measurably worse (its hair/highlight greys "
                         "need frame 1 to genuinely differ from frame 0, not "
                         "just look stable) even after two rounds of trying to "
                         "gate this safely. Validated instead as an opt-in value "
                         "for flat/cel-art content with large uniform white "
                         "areas: try 2.0 for that case specifically (confirmed "
                         "on a Donald Duck test image: cut the white belly's "
                         "frame-to-frame swing by ~9%% and fixed a couple of "
                         "stray wrong-hue rows there as a side effect, at low "
                         "RMS cost on that image). Don't enable this for "
                         "photographic/gradient source material.")
    ap.add_argument("--roll-scanlines", action="store_true",
                    help="only meaningful with --frames > 1. Post-process the N solved "
                         "frames into N 'rolled' composites where each --roll-band-height-row "
                         "BAND of composite k is taken from a different original solved "
                         "frame, instead of every row on screen switching frames in lockstep. "
                         "Mathematically inert on the perceived time-averaged image (a pure "
                         "reordering -- see roll_scanlines()) but spreads frame-to-frame "
                         "change across bands instead of hitting the whole screen at the "
                         "same instant. IMPORTANT, found the hard way: rolling at 1-row "
                         "granularity (the first version of this flag) broke colours almost "
                         "everywhere, because this tool's vertical colour blend has a real "
                         "radius (default 3) -- a row's solved colour is only correct next "
                         "to ITS OWN solved neighbours, and 1-row rolling puts nearly every "
                         "row next to a different frame's neighbours instead. --roll-band-height "
                         "controls the size of the safe interior each band gets; see its help "
                         "for the measured trade-off. Doesn't change the magnitude reported by "
                         "any RMS/jump metric this tool prints -- only spatial correlation, "
                         "which a scalar number can't show. Judge the actual result by eye on "
                         "real hardware.")
    ap.add_argument("--roll-band-height", type=int, default=None,
                    help="only meaningful with --roll-scanlines. Number of consecutive rows "
                         "that roll together as one unit (default: chosen from --radius, "
                         "8x --radius, minimum 16). Rows more than --radius away from a band "
                         "boundary have a fully self-consistent vertical-blend neighbourhood "
                         "(same original frame throughout); rows within --radius of a boundary "
                         "straddle two frames and show a locally-wrong blend there -- smaller "
                         "band_height means more, smaller bands (finer decorrelation) but more "
                         "boundary rows paying that cost; larger means fewer, bigger bands "
                         "(coarser decorrelation, closer to no rolling at all) but a cleaner "
                         "picture. Measured at height=128, radius=3: band_height=1 -> 100%% of "
                         "rows affected (this is what broke colours before), 7 -> 84%%, "
                         "14 -> 41%%, 32 -> 14%%, 64 -> 5%%. This tool prints the real count "
                         "for your actual --height/--radius/--roll-band-height when the flag "
                         "is used -- don't guess, read that number before trusting the result.")
    ap.add_argument("--saturation", type=float, default=1.0,
                    help="saturation multiplier applied to the source image before resizing "
                         "(default 1.0 = unchanged; >1 boosts, <1 mutes). Real-hardware testing "
                         "showed the reconstructed image reads as washed out/desaturated even at "
                         "--frames 1 (i.e. it's not a multi-frame temporal-averaging artifact -- "
                         "it's structural to fitting a photo through a 128-colour palette plus "
                         "binary on/off plus vertical blend against black neighbours, which "
                         "systematically pulls saturation down). Rather than compensate on the "
                         "display, push the input the other way before the optimiser sees it. "
                         "Start around 1.3-1.6 and compare against your actual hardware.")
    ap.add_argument("--brightness", type=float, default=1.0,
                    help="brightness multiplier applied to the source image before resizing "
                         "(default 1.0 = unchanged), same rationale as --saturation.")
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

    target = load_target(args.image, saturation=args.saturation, brightness=args.brightness)

    if args.smoothness == "auto":
        smoothness_value = auto_smoothness(target, verbose=not args.quiet)
    else:
        smoothness_value = args.smoothness

    solve_kwargs = dict(seeds=args.seeds, sigma=args.sigma, radius=args.radius,
                         rounds=args.rounds, sweeps=args.sweeps, metric=args.metric,
                         swaps=args.swaps, smoothness=smoothness_value, verbose=not args.quiet,
                         temporal_weight=args.temporal_weight)

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

        header_path, source_path = write_c_files_n_frame(prefix, color_idxs, ons,
                                                          rolled=args.roll_scanlines,
                                                          roll_band_height=roll_band_height)

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
            else:
                note = "" if i == 0 else "  (correction layer, not a picture on its own)"
            print(f"Wrote {raw_path}  (raw unblended TIA pixels, frame {i}){note}")

        print(f"1-frame RMS would have been: {result['rmse_first']:.2f}")
        print(f"{args.frames}-frame alternating RMS (unweighted RGB, 0-255 scale): {result['final_rmse']:.2f}")


if __name__ == "__main__":
    main()
