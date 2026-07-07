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

# 128x128 matrix of squared RGB distances between every pair of palette
# entries -- the "transition cost" table for solve_colour_sequence()'s
# column-wide DP. Computed once; never changes (palette is fixed).
PALETTE_PAIRWISE_DIST2 = np.sum(
    (PALETTE_RGB[:, None, :] - PALETTE_RGB[None, :, :]) ** 2, axis=-1)

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


nearest_palette_index_smooth.__doc__ += """

SUPERSEDED as of solve_colour_sequence() below -- solve() no longer calls
this function. Kept only for reference/backward compatibility. Root
problem found on a real photographic portrait (not cel-art): this
function's neighbour penalty only ever compares a row to its immediate
neighbour's ALREADY-CHOSEN colour, applied greedily top-to-bottom. That
has no memory of how far a chain of small, locally-reasonable nudges has
drifted from any individual row's own true colour -- confirmed directly:
long runs of 10-30 consecutive rows locking into a flat grey consensus
(a "band"), even though no single row in the run was decisively grey on
its own. Lowering `smoothness` shortens the runs but reintroduces the
original chattering-hue defect this function was built to fix -- every
value tried traded one visible defect for the other, never eliminating
both. solve_colour_sequence() replaces the greedy per-row chain with an
exact whole-column optimum (dynamic programming / Viterbi), which
structurally cannot drift arbitrarily far from any row's own fit cost,
because every row's real cost is counted in the objective, not just its
relative offset from an already-drifted neighbour."""


# Scale multiplying --smoothness (0-1ish) into the per-row-pair transition
# cost used by solve_colour_sequence(). Calibrated empirically against
# zph.png (a real photographic portrait) by directly measuring the two
# quantities this trades off: a typical row's fit-cost gap between its
# best and second-best palette candidate is ~100-300 (weighted RGB units
# squared); a typical pair of DIFFERENT palette entries is ~26000 apart
# (PALETTE_PAIRWISE_DIST2). So transition_scale needs to be a small
# fraction of 1 for the DP to actually weigh fit cost against transition
# cost, rather than one swamping the other.
#
# This calibration exposed a real, structural property of the whole-column
# DP that the old per-row greedy mechanism did NOT have: past a fairly
# sharp threshold (empirically, SMOOTHNESS_DP_SCALE times smoothness=0.5
# above roughly 0.25-0.5 on zph.png), the DP doesn't degrade gracefully --
# it snaps an ENTIRE frame's whole 128-row column to a single flat grey,
# because at that point no isolated run of true-colour rows anywhere in
# the column can pay back its own entry+exit transition cost, so "never
# leave the achromatic chain" becomes the global optimum outright. This is
# a genuine phase transition (confirmed: identical output across
# SMOOTHNESS_DP_SCALE=100 through 4000, all fully collapsed; the old
# placeholder value of 4000 was deep in this collapsed regime the whole
# time it went untested). It is arguably worse than the old mechanism's
# defect (which never fully consumed 128/128 rows in one frame), so this
# constant is deliberately calibrated to sit well inside the safe,
# fit-cost-respecting regime across the tool's full --smoothness 0-1
# range, not just at the default of 0.5. Confirmed safe (no collapse,
# balanced cross-frame spread, RMS within ~2% of the smoothness=0
# baseline) at --smoothness 0.0/0.2/0.5/0.8/1.0 on zph.png, and re-checked
# against donald.jpg (see calibration notes below) to confirm the original
# cel-art fix -- flat jacket, no outline chatter -- still holds.
SMOOTHNESS_DP_SCALE = 0.15

# Phase-2 colour refinement recomputes every row's ideal continuous target
# from a single shared start-of-round snapshot (Jacobi-style batch), then
# solves the whole column at once. That's required for the DP's cost table
# to be internally consistent within a round, but a *full-strength* batch
# update on a coupled system (each row's blend affects, and is affected by,
# its neighbours) can 2-cycle: round N's column-wide choice shifts the
# blended state enough that round N+1's fresh correction favours flipping
# straight back, forever. Confirmed on real data (zph.png): weighted SSE
# alternated between two values every single round instead of decreasing,
# with the "final" answer arbitrarily depending on whether `rounds` happened
# to be odd or even. Damping the correction before it's handed to the DP
# (a standard fix for Jacobi-relaxation oscillation) breaks the 2-cycle by
# under-shooting each round instead of matching the full correction, so
# consecutive rounds nudge in the same direction rather than swapping.
PHASE2_RELAXATION = 0.5


PALETTE_SPREAD = PALETTE_RGB.max(axis=1) - PALETTE_RGB.min(axis=1)  # (128,)

# Weight on a bias term added in _fit_cost_table() that penalises a
# palette candidate for being LESS saturated (smaller max-min channel
# spread) than the row's own target colour, proportional to the square of
# that saturation shortfall. Root cause this exists to fix, found by
# decoding actual per-row fit costs on zph.png (a real photographic
# portrait): plain weighted-RGB squared distance regularly ranks a FLAT
# GREY palette entry as the closest match to a moderately-saturated warm
# skin tone (e.g. target (145,107,86), spread 59) -- not a near-tie, grey
# won outright (511 vs 613 for the best chromatic candidate, and that gap
# held under unweighted RGB distance too, 1797 vs 1918). This is a real
# palette-gamut property, not a bug in the target or the metric weights:
# the 128 available (hue, luma) points on real NTSC/TIA hardware just
# don't include a softly-saturated warm hue at every luma level, so grey
# can legitimately minimise total squared channel error even though it
# throws away the hue entirely. Squared-RGB-error is a bad perceptual
# proxy for that specific trade-off: a viewer reads "skin tone gone grey"
# as a much worse defect than "skin tone slightly the wrong saturation",
# but plain distance doesn't know that -- it'll happily pick "no hue at
# all" over "correct hue, imperfect saturation" whenever grey's total
# error happens to be lower. This bias term corrects for it directly:
# candidates that are already at least as saturated as the target pay no
# penalty (deficit clamps to 0), so it never overrides a genuinely-better
# chromatic match or forces colour onto rows that are genuinely neutral
# in the source (their own target_spread is already small, so there's
# nothing for cand_spread to fall short of). Calibrated against zph.png
# (eliminates the flat-grey skin-tone bands entirely at this value, RMS
# cost ~8%) and checked against donald.jpg (does not force colour onto
# genuinely-grey/white regions -- background, highlights -- confirmed by
# checking that no row with a near-neutral SOURCE target gets pushed
# above spread 40 by this term).
CHROMA_BIAS_SCALE = 1.0

# Ceiling on the target spread used by the bias above. Needed because
# continuous_targets isn't always a real, in-gamut colour: in a multi-frame
# solve, frames after the first are fit against a RESIDUAL ("what's still
# needed after already-committed frames"), computed as (i+1)*target -
# cumulative -- and that arithmetic routinely produces per-channel values
# outside 0-255 (confirmed on zph.png's frame 1: R ranged -80 to +294) with
# nominal spread well past anything the palette can actually reach (up to
# 182, vs. the palette's own largest spread of 168). Feeding that
# uncapped into the bias above backfires: it doesn't just discourage grey,
# it discourages EVERYTHING except the most saturated entries in the whole
# palette, regardless of whether that hue is anywhere near correct --
# confirmed directly, this produced an obvious, wrong magenta cast across
# the reconstructed 3-frame average that wasn't there before. Capping the
# spread fed into the deficit calculation at a real photographic image's
# typical on-pixel spread (well under the palette's max) keeps the bias
# doing its one job -- stop grey from beating a moderately-saturated real
# colour -- without also chasing an out-of-gamut residual's inflated,
# not-really-a-colour spread number.
CHROMA_BIAS_SPREAD_CAP = 60.0


def _fit_cost_table(continuous_targets, weights, valid_mask=None, chroma_bias_scale=None):
    """(HEIGHT, 128) matrix of weighted squared distance from each row's own
    continuous target colour (continuous_targets[y], one RGB triple per
    row) to every one of the 128 palette candidates -- the per-row "how
    good is each candidate" cost half of solve_colour_sequence()'s
    objective. Includes a saturation-deficit penalty (see CHROMA_BIAS_SCALE's
    comment) so a flat-grey candidate can't out-compete a correctly-hued one
    purely on raw squared-distance grounds.

    chroma_bias_scale overrides the module-level CHROMA_BIAS_SCALE for this
    call (None, the default, means "use CHROMA_BIAS_SCALE"). solve_n_frame()
    passes 0.0 for every frame after the first: frames 1..n-1 are fit
    against a mathematical RESIDUAL, not a real photographed colour (see its
    docstring and CHROMA_BIAS_SPREAD_CAP's comment for why that matters) --
    confirmed directly that applying this bias to a residual target chases
    the largest available spread in whatever direction the residual
    arithmetic happens to point, which need not be a sensible hue at all
    (a real run produced an obvious, wrong magenta cast in frames 1-2 of a
    photographic portrait, even with CHROMA_BIAS_SPREAD_CAP in place). The
    bias is only trustworthy against a genuine, directly-photographed
    target, which is exactly frame 0.

    valid_mask (optional, one bool per row): rows where this is False
    (typically: no on-pixels this round, so nothing currently visible
    depends on that row's colour) get a flat zero cost across every
    candidate -- the column-wide solve is then free to pick whatever's
    cheapest for that row's neighbours there, since nothing real is being
    fit at that row anyway."""
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


def solve_colour_sequence(fit_cost, transition_scale, temporal_ref_colours=None, temporal_weight=0.0):
    """Choose one palette index per row, for the WHOLE column at once, by
    exact dynamic programming (Viterbi) instead of the greedy per-row scan
    nearest_palette_index_smooth() used to do. Minimises, over the entire
    column jointly:

        sum_y fit_cost[y, idx_y]
      + sum_y transition_scale * ||palette[idx_y] - palette[idx_{y-1}]||^2

    i.e. exactly the same kind of trade-off the old mechanism was
    approximating (this row's own fit vs. staying close to its neighbour),
    but solved exactly rather than greedily. This is the direct fix for
    the banding defect nearest_palette_index_smooth() could produce: a
    greedy chain only ever compares against an already-committed
    neighbour and can drift indefinitely as long as each individual step
    looks locally cheap, with no accounting for how far the whole run has
    strayed from any row's own true colour. The DP cannot do this --
    every row's real fit_cost is counted in the total, in full, no matter
    how long a "consensus" run around it is, so a long run of a colour
    that's a poor fit for most of the rows in it is only ever chosen if
    it's genuinely the cheapest whole-column answer, not an artifact of
    left-to-right propagation.

    A useful side effect, verified rather than assumed: this needs no
    separate per-row margin-based adaptive scaling the way the old
    mechanism did (see SMOOTHNESS_MARGIN_SCALE) -- a row with a large gap
    between its best and second-best candidate naturally resists being
    pulled off that best candidate here too, because the DP is weighing
    the REAL fit_cost difference between candidates directly, not a
    heuristic proxy for it. One flat transition_scale is enough.

    `temporal_ref_colours`/`temporal_weight`: same cross-FRAME (not
    cross-row) mechanism as before (see nearest_palette_index_smooth's
    docstring for the full reuse-cost rationale) -- applied here as a
    per-row additive bias to fit_cost before running the DP, since it
    compares each row to a fixed external reference colour, not to its
    neighbours, so it doesn't interact with the transition term at all.
    """
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
          temporal_ref_colours=None, temporal_weight=0.0, chroma_bias_scale=None,
          fixed_color_idx=None):
    """fixed_color_idx (optional, one palette index per row): skip both the
    colour-selection init AND the per-round colour-refinement phase
    entirely, using this sequence as-is throughout -- only the on/off
    (which pixels in each row are lit) mask is solved. Used by the joint
    multi-frame solve (solve_n_frame_joint()), which decides every frame's
    colour sequence together up front (see its docstring for why) and
    only needs solve() for its on/off-mask machinery from here on."""
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

    # --- initialise colours: one whole-column DP pass, not a per-row scan -
    # (see solve_colour_sequence() -- this replaces the old greedy
    # nearest_palette_index_smooth() sweep, which could drift into long,
    # false-consensus grey bands on real photographic content).
    #
    # The per-row target is the average of that row's non-near-black
    # pixels only, not the whole row. Found by direct inspection of a real
    # failure: on a row that's mostly black canvas with a small
    # saturated-colour subject (e.g. 15 of 48 columns are a blue glove,
    # the rest black background), the WHOLE-row average is dark and
    # desaturated enough that it's a genuinely decisive (large-margin)
    # match for a dark grey palette entry -- not a close tie with blue, a
    # confident win for the wrong answer. The later per-round
    # colour-refinement step already fits on-pixels only (correctly), but
    # a bad enough initial guess can still get stuck (the on/off pattern
    # co-adapts to the wrong initial colour before refinement gets a
    # chance to correct it). Excluding near-black pixels from the AVERAGE
    # up front -- a cheap proxy for "pixels likely to end up on" -- gives
    # the initial guess a fair start instead of diluting it with
    # background. Falls back to the whole-row average for genuinely
    # all-background rows (nothing to exclude to).
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
            temporal_weight=temporal_weight)
    colour = PALETTE_RGB[color_idx]

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

        # ---------------- phase 2: colour refinement (whole-column DP) --
        # Skipped entirely when fixed_color_idx was given -- the caller
        # (solve_n_frame_joint()) already decided every row's colour for
        # every frame jointly, and re-optimizing any one frame's colours in
        # isolation here would undo that: it's exactly the same "solve one
        # frame's colours against whatever's left over" mistake that made
        # the old cumulative-residual scheme fragile (see solve_n_frame()
        # and CHROMA_BIAS_SPREAD_CAP's comments for the full history).
        #
        # (weighting cancels out of the per-channel normal equations here --
        # each channel is solved independently either way -- so this closed
        # form is unchanged; only the DP palette choice below needs the
        # weights.)
        #
        # First pass: compute every row's own ideal continuous colour
        # (the closed-form correction against the CURRENT, start-of-round
        # blended state) WITHOUT snapping or updating anything yet -- a
        # Jacobi-style snapshot, not the old Gauss-Seidel sequential
        # apply. This is what lets the DP see every row's true,
        # uncorrupted fit cost in the same pass: under the old scheme,
        # each row's colour choice fed immediately into blended[], so by
        # the time row y+5 was processed its "correction" already
        # reflected rows 0..y's fresh choices -- fine for a single greedy
        # snap, but exactly the kind of shifting target that let a long
        # run drift row by row. Rows with no on-pixels this round get a
        # flat (don't-care) cost -- nothing visible depends on their
        # colour, so the DP is free to pick whatever's cheapest for its
        # neighbours there.
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
            transition_scale = smoothness * SMOOTHNESS_DP_SCALE
            new_color_idx = solve_colour_sequence(fit_cost, transition_scale,
                                                   temporal_ref_colours=temporal_ref_colours,
                                                   temporal_weight=temporal_weight)

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


# Weight and cap for the anti-grey chroma bias used by
# solve_colour_triplets() below -- same idea and same units as
# CHROMA_BIAS_SCALE/CHROMA_BIAS_SPREAD_CAP (see their comments), just
# applied to a CANDIDATE AVERAGE (of up to 3 real palette entries) instead
# of a single candidate. Kept as separate constants because this is
# comparing against a genuine, always-real-photo target (never a residual
# -- see solve_n_frame_joint()'s docstring for why that distinction is the
# whole point of this function), so there's no reason its calibration has
# to match the single-frame constants exactly, even though empirically the
# same values worked well in testing.
JOINT_CHROMA_BIAS_SCALE = 1.0
JOINT_CHROMA_BIAS_SPREAD_CAP = 60.0


def solve_colour_triplets(row_targets, weights, n_frames=3,
                           chroma_bias_scale=JOINT_CHROMA_BIAS_SCALE,
                           chroma_cap=JOINT_CHROMA_BIAS_SPREAD_CAP,
                           k_a=16, k_partner=6):
    """For every row, choose a MULTISET of n_frames real palette entries
    (repeats allowed) whose AVERAGE best matches that row's own target
    colour -- the joint alternative to the old cumulative-residual scheme.

    Root problem this replaces: the old scheme (solve_n_frame()) solves
    frame 0 directly against the real target, then frame 1 against
    whatever's LEFT OVER (a residual, (i+1)*target - cumulative), then
    frame 2 against what's left after that. A residual is not a real,
    in-gamut colour -- confirmed directly on zph.png, one frame's residual
    ranged from -80 to +294 per channel -- so a frame fit against it is
    fitting something that may not resemble any achievable colour at all.
    Every attempt to patch that up locally (anti-grey bias on later frames,
    capping the residual's nominal spread, referencing the real photo's
    spread instead of the residual's) either let a whole frame collapse to
    flat grey or chase an out-of-gamut residual into a wrong, saturated hue
    (confirmed: an obvious magenta cast on a real photo). The problem was
    architectural, not a tuning problem: solving frames SEQUENTIALLY, each
    one blind to what the others will need, has no way to avoid painting
    one of them into a corner.

    This function sidesteps that entirely by never computing a residual in
    the first place. For n_frames == 3 specifically (frame count is fixed
    project-wide), it searches three families of REAL, achievable averages
    per row and keeps whichever actually scores best against the target:

      - single: one palette entry, used all 3 frames (repeated 3x)
      - 2:1: one entry twice, a second entry once
      - 1:1:1: three distinct entries, once each

    Every candidate considered is the average of REAL palette entries, so
    there is no out-of-gamut residual for the search to chase -- if grey
    genuinely IS the best-matching single entry (a genuinely neutral source
    row), it will still win fairly; it just can no longer win by default
    against a moderately-saturated real colour purely because the palette
    lacks a softly-saturated match at that hue (the JOINT_CHROMA_BIAS_SCALE
    term below corrects for that raw-distance quirk, exactly as
    CHROMA_BIAS_SCALE does for the single-frame case -- see its comment).

    A separate "flicker" penalty (discouraging the 2:1/1:1:1 options'
    constituent colours from being far apart) was prototyped and rejected:
    at any weight strong enough to matter it either did nothing (weak) or
    collapsed EVERY row to the single-colour option (strong enough to
    actually constrain anything) -- there is no middle ground, because
    genuine temporal dithering only ever helps when the constituent colours
    ARE far apart (that's what makes their average reach a target no
    single palette entry can). Frame-to-frame jump this large is already a
    known, already-accepted property of this whole cycling technique (see
    the "flicker-magnitude metric" printed by the CLI, historically ~70 RMS
    units even under the old scheme) -- what actually matters for keeping
    it from looking broken is per-FRAME column continuity (handled by
    assign_triplet_to_frames(), not here) and eliminating flat-frame
    collapse (handled by the chroma bias above), not raw distance between
    the frames.

    Returns (chosen, cost): chosen is (HEIGHT, n_frames) int array of
    palette indices (unordered -- caller assigns them to frame slots), cost
    is (HEIGHT,) achieved weighted-fit-plus-bias cost."""
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
    # single-match candidate in practice -- confirmed on real test images
    # that widening K further doesn't change the outcome).
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
    colour. solve_colour_triplets() decides WHAT 3 colours a row should
    use without any notion of "which frame" -- left arbitrary, row-to-row
    ordering would make each frame's own column jump around for no reason
    beyond how the search happened to list its answer, on top of the jump
    the technique genuinely needs. This is a cheap post-process (an
    exhaustive search over <=6 permutations per row) that costs nothing in
    fit quality (all 6 permutations of the same multiset score identically
    against the target) but keeps each frame's column as internally
    coherent as the chosen colours allow.

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


# Scale multiplying --smoothness (0-1ish) into the transition cost used by
# the per-frame column-smoothing pass below (solve_n_frame_joint()'s
# smooth_frame_column()). Needed because solve_colour_triplets() decides
# every row completely independently -- nothing links row y's choice to
# row y+1's -- and that leaves real row-to-row jitter WITHIN a single
# frame's own column (confirmed directly on zph.png: mean row-to-row RGB
# jump ~43, comparable in magnitude to the jump BETWEEN frames that the
# whole technique already accepts). Visually this reads as a fine,
# incoherent speckle rather than the coarser, more deliberate-looking
# dithering the old scheme produced -- reported directly as looking "bland
# and almost monochrome" on a real render, even though per-pixel
# saturation metrics came out equal or higher than the old scheme (the
# fine alternation between different hues gets partially averaged out by
# the eye's own spatial integration, the same mechanism this tool
# deliberately exploits ACROSS frames, just happening unintentionally
# WITHIN one frame's column here).
#
# This smoothing runs as a second DP pass per frame, anchoring each row's
# fit cost to the jointly-chosen colour (never a residual) with a
# transition cost identical in form to solve_colour_sequence()'s. Same
# sharp-cliff behaviour as SMOOTHNESS_DP_SCALE was found here too (any of
# 20-200 collapsed every column to one flat colour, fit_rmse ballooning
# from ~4 to ~50+) -- calibrated instead in the same much smaller range
# that worked there, chosen so the default --smoothness 0.5 lands at a
# transition_scale of ~0.3: cuts the mean row-to-row jump roughly in half
# (43 -> ~23) for a moderate, bounded fit-cost increase, without fully
# flattening any column.
JOINT_SMOOTHING_DP_SCALE = 0.6


def smooth_frame_column(assigned_idx, weights, transition_scale):
    """Second DP pass smoothing ONE frame's already-jointly-chosen colour
    sequence against its own row-to-row jitter (see JOINT_SMOOTHING_DP_SCALE's
    comment). Anchors each row's fit cost to the colour
    solve_colour_triplets()/assign_triplet_to_frames() already picked for
    it (never a residual -- this can only pull a row's colour towards
    another REAL palette entry, weighted against how far that would move
    it from its jointly-optimal pick), then runs the same whole-column
    Viterbi DP used elsewhere in this file."""
    anchor_rgb = PALETTE_RGB[assigned_idx]                      # (H,3)
    fit_cost = np.sum(weights * (PALETTE_RGB[None, :, :] - anchor_rgb[:, None, :]) ** 2, axis=-1)
    return solve_colour_sequence(fit_cost, transition_scale)


def solve_n_frame_joint(target, n_frames=3, seeds=3, sigma=1.0, radius=3,
                         rounds=10, sweeps=3, metric="luma", smoothness=0.5,
                         outer_rounds=1, verbose=True, **kwargs):
    """The joint alternative to solve_n_frame() -- see
    solve_colour_triplets()'s docstring for the full rationale (the old
    scheme's residual-chasing could collapse a whole frame to flat grey or
    a wrong saturated hue; this never constructs a residual in the first
    place).

    Every frame's colour SEQUENCE is decided together, per row, from the
    current per-row target (solve_colour_triplets() + assign_triplet_to_
    frames() below), then smoothed per frame (smooth_frame_column(),
    controlled by `smoothness`) to reduce the row-to-row jitter that
    per-row-independent joint choices otherwise leave -- then each frame is
    solved independently only for its on/off mask (which pixels in each row
    are lit), via solve(..., fixed_color_idx=...), since that part of the
    problem genuinely doesn't interact across frames the way colour choice
    does. `outer_rounds` (default 3) repeats this using the REAL achieved
    blended average each round to correct next round's per-row target --
    see the comment above this function for why that's needed (the
    one-shot crude row-average target otherwise leaves real accuracy on
    the table) and why it's still safe (the correction is shared equally
    across all frames' target every round, never handed to one frame to
    fix alone)."""
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
    # vertical blending and which pixels actually end up on -- exactly the
    # gap solve()'s phase-2 DP closes for a single frame, using the REAL
    # achieved blended state each round. Do the same thing here, jointly:
    # after solving all n_frames frames' on/off masks against the current
    # per-row targets, measure the actual achieved average, fold the
    # remaining error back into next round's per-row target, and redo the
    # (still fully joint, never-a-per-frame-residual) triplet search. This
    # closed a real accuracy gap found on zph.png (a real photographic
    # portrait, reported as looking "bland and almost monochrome" without
    # this): the one-shot crude-average version left final RMS ~40% worse
    # than the old (architecturally fragile) scheme's; this halves that
    # gap in 2 extra rounds. Unlike the old scheme's residual-chasing, the
    # correction here is applied EQUALLY to all n_frames frames' shared
    # target every round, not handed to one frame alone to fix by itself --
    # confirmed this does not reintroduce the flat-grey/wrong-hue collapse
    # (still goes through solve_colour_triplets()'s chroma-bias-corrected
    # search every round, so grey still has to win fairly, and no single
    # frame is ever singled out to close an awkward leftover alone).
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
        # The chroma-anti-grey bias (see _fit_cost_table's docstring) is
        # only trustworthy against a genuine photographed target, not a
        # residual, so it's never applied to frames 1..n-1 (those always
        # fit a residual). It's ALSO forced off for frame 0 here whenever
        # n_frames > 1, even though frame 0's own target is the direct
        # photo -- confirmed directly that turning it on for frame 0 alone
        # still regresses the OTHER frames: making frame 0 fit its target
        # more accurately (less grey) leaves a different, harder residual
        # for frame 1 to close, and frame 1 responded by collapsing to
        # near-total flat grey across 111/128 rows (worse than the
        # original defect this bias was built to fix). The multi-frame
        # cumulative-residual scheme's cross-frame balance is fragile in
        # a way a per-row fit-cost tweak on just one frame can't safely
        # touch -- fixing it properly needs the frames' colour budgets
        # solved jointly, not this greedy frame-by-frame scheme. So for
        # n_frames > 1 the bias defaults off everywhere, preserving the
        # verified-safe cross-frame balance, unless the caller explicitly
        # overrides chroma_bias_scale themselves (single-frame calls via
        # solve_best_of()/solve() directly are unaffected and keep the
        # bias on by default).
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


def _c_string_escape(s):
    """Escape a string for safe embedding inside a C string literal
    (backslashes and double quotes only -- the params strings this is used
    for are plain ASCII key=value text, never containing newlines)."""
    return s.replace("\\", "\\\\").replace('"', '\\"')


def build_params_string(**kwargs):
    """Build a single human-readable "key=value key=value ..." string of the
    RESOLVED parameters used for a run, for embedding in the generated header
    as <PREFIX>_PARAMS. Callers should pass the actual effective values (e.g.
    the resolved smoothness float, not the literal string "auto"; the
    resolved roll_band_height, not None) so the define is a faithful record
    of what actually ran, not of what was typed on the command line."""
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
    # Sanitised to a valid C identifier fragment. Only used to prefix the
    # one data symbol this file exports (array_name below) and the
    # per-file include guard -- NOT the ScanLine type, which is shared
    # verbatim across every generated file (see _scanline_type_block()).
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


def write_c_files_n_frame(prefix, color_idxs, ons, rolled=False, roll_band_height=None, params=None):
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
    # Per-file, not SCREEN_NUM_FRAMES -- that name was shared across every
    # generated file regardless of prefix, which is fine for SCREEN_WIDTH/
    # HEIGHT/BYTES_PER_ROW (see _scanline_type_block()'s docstring: those
    # are deliberately project-wide constants, assumed identical everywhere),
    # but NUM_FRAMES genuinely varies per generated image (a 2-frame image
    # and a 3-frame image can coexist in the same project), and each
    # generated header has its OWN include guard, so two such headers
    # #included together would redefine the same macro to different
    # values. Prefixing with this file's own identifier fixes that.
    num_frames_define = f"{ident_base.upper()}_NUM_FRAMES"
    params_define = ""
    if params is not None:
        params_define = f'#define {ident_base.upper()}_PARAMS "{_c_string_escape(params)}"\n\n'

    if rolled:
        frame_doc = f""" * ScanLine is defined once and shared verbatim across every file this
 * tool generates. The {n_frames} individual frame arrays are file-local
 * (static) -- the only thing this translation unit exports is
 * {table_name}, a table of {n_frames} pointers in cycle order. Index it
 * by a running frame counter (mod {num_frames_define}) exactly as usual --
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
 * by a running frame counter (mod {num_frames_define}): cycle frame 0, 1,
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

        f.write(f"const ScanLine * const {table_name}[{num_frames_define}] = {{\n")
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
                    help="strength (0-1ish) of a whole-column row-to-row "
                         "consistency preference, applied by solving every "
                         "scanline's colour in one shot (a dynamic-programming / "
                         "Viterbi pass over all rows at once -- see "
                         "solve_colour_sequence()), not row-by-row. History: this "
                         "used to be a CEILING on a per-row penalty against each "
                         "row's already-committed neighbour, applied greedily "
                         "top-to-bottom; that greedy version had no memory of how "
                         "far a chain of small, locally-reasonable nudges had "
                         "drifted from any individual row's true colour, and on a "
                         "real photographic portrait it could lock 10-30 "
                         "consecutive rows into a false flat-grey consensus even "
                         "though no single row in the run was decisively grey on "
                         "its own (confirmed by decoding actual compiled hardware "
                         "bytes). The whole-column solve fixes that structurally: "
                         "every row's real fit cost is counted in one global "
                         "objective, not just its offset from a possibly-already-"
                         "wrong neighbour, so it can't drift arbitrarily far from "
                         "any row's own true match. It has a different failure mode "
                         "instead, found and calibrated against: pushed too high, "
                         "it doesn't degrade gradually -- an entire frame's whole "
                         "column can snap to one flat colour outright, because at "
                         "that point no isolated run of true-colour rows anywhere "
                         "can pay back its own entry+exit transition cost. The "
                         "internal scale (SMOOTHNESS_DP_SCALE) is calibrated so the "
                         "full 0-1 range stays inside the safe, fit-cost-respecting "
                         "regime on real test images (a photographic portrait and "
                         "a flat-cel-art cutout) rather than anywhere near that "
                         "cliff. 'auto' (default) means 'use the recommended "
                         "value, 0.5'. Pass an explicit number to change it "
                         "directly; 0 disables the mechanism entirely (pure "
                         "per-row nearest-palette-match, independent of neighbours).")
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
    ap.add_argument("--frame-scheme", choices=["joint", "cumulative"], default="cumulative",
                    help="only meaningful with --frames > 1 (currently only 3 is "
                         "supported by 'joint'). 'cumulative' (default): frame 0 solved "
                         "directly against the real target, frame 1 against frame 0's "
                         "leftover residual, frame 2 against what's left after that -- "
                         "each frame gets full iterative closed-form colour refinement "
                         "(see solve()'s phase 2), which is where most of its accuracy "
                         "comes from (measured ~30-40%% lower RMS than 'joint' on real "
                         "test images). Known failure mode: an awkward-enough residual "
                         "can leave a WHOLE frame's worth of rows collapsed to flat grey, "
                         "or chase the residual into a wrong saturated hue if patched "
                         "with an anti-grey bias -- an architectural problem with solving "
                         "frames sequentially and blind to each other, confirmed on a "
                         "real photographic portrait. Check the per-frame _raw.png previews "
                         "for a whole frame reading as flat grey or an odd solid colour; "
                         "if you see that, try 'joint' instead. 'joint': every frame's "
                         "colour sequence decided TOGETHER, per row, as the best real "
                         "achievable average of up to 3 real palette entries (see "
                         "solve_n_frame_joint()/solve_colour_triplets()'s docstrings) -- "
                         "structurally can't produce that collapse/wrong-hue failure, "
                         "verified with real per-row colour dumps, but reported directly "
                         "on a real render as looking flatter/less vibrant overall than "
                         "'cumulative' (confirmed: real RMS cost, and two follow-up "
                         "attempts to close the gap -- smoothing the joint choice's own "
                         "row-to-row jitter, and iterative outer-round refinement against "
                         "the real achieved blend -- neither fixed it, so this is a real, "
                         "still-open trade-off, not a bug to tune away).")
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

    # Resolved (not raw-argparse-sentinel) values only -- e.g. smoothness_value
    # here is the actual float used, not the literal string "auto"; roll_band_height
    # (added below in the --frames>1 branch, once its own default is resolved)
    # is the actual band height used, not None. See build_params_string().
    base_params = dict(image=args.image, width=WIDTH, height=HEIGHT, sigma=args.sigma,
                        radius=args.radius, rounds=args.rounds, sweeps=args.sweeps,
                        metric=args.metric, seeds=args.seeds, smoothness=smoothness_value,
                        swaps=args.swaps, saturation=args.saturation, brightness=args.brightness,
                        frames=args.frames, temporal_weight=args.temporal_weight,
                        frame_scheme=args.frame_scheme)

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
        if args.frame_scheme == "joint":
            if args.frames != 3:
                ap.error("--frame-scheme joint currently only supports --frames 3 "
                         "(frame count is fixed project-wide anyway); pass "
                         "--frame-scheme cumulative to use another frame count")
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
                                                          params=params_str)

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

        if result["rmse_first"] is not None:
            print(f"1-frame RMS would have been: {result['rmse_first']:.2f}")
        print(f"{args.frames}-frame alternating RMS (unweighted RGB, 0-255 scale): {result['final_rmse']:.2f}")


if __name__ == "__main__":
    main()
