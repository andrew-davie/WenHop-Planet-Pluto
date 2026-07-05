#!/usr/bin/env python3
"""
atari_scanline_blend_cyclic.py -- N-frame temporal-blend solver using
CYCLIC (bidirectional) relaxation instead of one-way residual chasing.

This is a SEPARATE tool from atari_scanline_blend.py. It imports that
module for everything that doesn't change (palette, per-frame DBS +
closed-form colour solve, blend model, C/PNG output format) and only
replaces how the N frames of a temporal sequence are coordinated.

THE PROBLEM WITH THE ORIGINAL N-FRAME SCHEME
---------------------------------------------
atari_scanline_blend.py's solve_n_frame() is strictly forward/causal:
frame i's target only depends on frames already solved before it
(0..i-1). Frame 0 is solved with zero knowledge that a last frame will
eventually need to close the loop back to it, nothing is ever revisited
once solved, and there's no notion of "wraparound" (frame N-1 back to
frame 0) during solving at all -- only in the final display cycle.

Separately: measuring an actual 4-frame result showed the frame-to-frame
raw pixel RMS jump (~66) is *larger* than the final time-averaged RMS
error (~20) and nearly as large as the whole image's own dynamic range.
That's a real risk of visible flicker on hardware (same mechanism as
well-known sprite-multiplexing flicker on real Atari 2600s), not just a
theoretical concern.

THE CYCLIC ALTERNATIVE
-----------------------
Treat "sum of all N frames == N*target" as a symmetric, cyclic
constraint with no privileged starting frame, and enforce it with
Gauss-Seidel-style coordinate descent ACROSS FRAMES (the same kind of
alternating local-search convergence the base tool already uses within
a single frame for DBS + colour refinement, just one level up):

    1. Get an initial guess for all N frames (reuse the existing
       one-pass solve_n_frame() from the base module -- a perfectly
       good starting point, just not a converged one).
    2. Repeat for --outer-rounds passes:
         for each frame i (0..N-1), in order:
             other_sum = sum of all OTHER frames' current blended output
                         (this includes frames on BOTH sides of i once
                         at least one full pass has happened, wraparound
                         included -- frame 0 gets to react to frame N-1's
                         current state, not just frame N-1 reacting to it)
             this_target = N*target - other_sum   # what frame i needs to
                                                    # contribute for the
                                                    # full cyclic average
                                                    # to hit target exactly
             re-solve frame i against this_target, replacing its old
             colour/on-off state.
    3. Stop after --outer-rounds passes (this is a brand-new algorithm --
       there's no established plateau point yet the way there is for the
       base tool's inner --rounds; watch the printed per-round RMS and
       judge for yourself whether it's still improving).

This is NOT guaranteed to converge monotonically -- each frame's
re-solve is a full independent local search (DBS + colour refinement),
not a small nudge, so it's possible for outer rounds to oscillate rather
than settle, the same way adding swap-moves inside a single frame's
solve turned out to sometimes make things worse rather than better. The
tool prints per-round RMS specifically so that's visible rather than
hidden, and reports the frame-to-frame jump metric so the accuracy vs.
flicker trade-off can be judged directly from real numbers.

USAGE
-----
    python3 atari_scanline_blend_cyclic.py input.png [-o OUTPUT_PREFIX]
                                            --frames N [--outer-rounds 4]
                                            [--width 48] [--height 198]
                                            [--sigma 1.0] [--radius 3]
                                            [--rounds 8] [--sweeps 3]
                                            [--metric luma] [--seeds 2]
                                            [--smoothness 0.01] [--quiet]
"""
import argparse
import sys
import time
import numpy as np

import atari_scanline_blend as base


def compute_frame_jump(color_idxs, ons, height, width):
    """Mean cyclic (wraparound included) frame-to-frame RMS difference in
    actual raw displayed pixels (colour if on, else black) -- the real
    physical stimulus at any given screen dot across the frame cycle, and
    therefore the relevant flicker-magnitude metric. Compare this against
    the final time-averaged RMS: if the jump is much bigger than the
    final RMS, most of what's happening is high-contrast strobing, not
    gentle blending."""
    n = len(color_idxs)
    raws = []
    for i in range(n):
        colour = base.PALETTE_RGB[color_idxs[i]]
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
                          sigma=1.0, radius=3, verbose=True, terse=False, **kwargs):
    """terse=False (default): full progress -- every DBS round of every
    frame-solve prints its running SSE, so a long run never goes silent
    for more than a fraction of a second. terse=True: only announce each
    frame-solve starting and each outer round finishing (no per-DBS-round
    spam) -- a quieter middle ground between full detail and --quiet."""
    if n_frames < 2:
        raise ValueError("n_frames must be >= 2 -- use the base tool directly for a single frame")

    height, width = base.HEIGHT, base.WIDTH
    inner_verbose = verbose and not terse

    # ---- initial guess: reuse the existing one-pass forward scheme as a
    # perfectly reasonable, cheap starting point (it already satisfies the
    # "average == target" constraint approximately; we're just going to
    # let every frame react to every other frame's current state instead
    # of freezing each one the moment it's first solved). ----
    if verbose:
        print(f"=== initial pass: one-pass forward solve, {n_frames} frame(s) ===", flush=True)
    init = base.solve_n_frame(target, n_frames=n_frames, seeds=seeds,
                               sigma=sigma, radius=radius, verbose=inner_verbose, **kwargs)
    color_idxs = list(init["color_idxs"])
    ons = list(init["ons"])
    blendeds = list(init["blendeds"])
    cur_sum = sum(blendeds)

    def score(s):
        avg = s / n_frames
        return (float(np.sum((target - avg) ** 2)) / (height * width * 3)) ** 0.5

    t_start = time.monotonic()
    if verbose:
        print(f"[init, one-pass forward] RMS = {score(cur_sum):.2f}  [{time.monotonic()-t_start:.1f}s elapsed]",
              flush=True)

    round_durations = []
    for outer in range(1, outer_rounds + 1):
        round_t0 = time.monotonic()
        for i in range(n_frames):
            if verbose:
                print(f"--- outer round {outer}/{outer_rounds}, solving frame {i+1}/{n_frames} "
                      f"(seeds={seeds}) [{time.monotonic()-t_start:.1f}s elapsed total] ---", flush=True)
            other_sum = cur_sum - blendeds[i]
            this_target = n_frames * target - other_sum
            result = base.solve_best_of(this_target, seeds=seeds, sigma=sigma, radius=radius,
                                         tag=f"[outer{outer} frame{i}] ", verbose=inner_verbose, **kwargs)
            color_idx_i, on_i, blended_i, rmse_i, werr_i = result
            cur_sum = cur_sum - blendeds[i] + blended_i
            color_idxs[i] = color_idx_i
            ons[i] = on_i
            blendeds[i] = blended_i

        round_durations.append(time.monotonic() - round_t0)
        if verbose:
            avg_round = sum(round_durations) / len(round_durations)
            remaining_rounds = outer_rounds - outer
            eta = avg_round * remaining_rounds
            elapsed = time.monotonic() - t_start
            print(f"[outer round {outer}/{outer_rounds}] RMS = {score(cur_sum):.2f}  "
                  f"[{elapsed:.1f}s elapsed, ~{eta:.0f}s remaining at current pace]", flush=True)

    final_rmse = score(cur_sum)
    averaged = cur_sum / n_frames
    return {
        "color_idxs": color_idxs, "ons": ons, "blendeds": blendeds,
        "final_rmse": final_rmse, "averaged": averaged,
    }


def main():
    # See atari_scanline_blend.py's main() for why: without this, piping or
    # redirecting output can make Python fully-buffer stdout, so a long run
    # looks completely silent for minutes even though it's working.
    try:
        sys.stdout.reconfigure(line_buffering=True)
    except AttributeError:
        pass

    ap = argparse.ArgumentParser(description="Cyclic (bidirectional) N-frame Atari scanline solver.")
    ap.add_argument("image")
    ap.add_argument("-o", "--output-prefix", default=None)
    ap.add_argument("--width", type=int, default=48)
    ap.add_argument("--height", type=int, default=198)
    ap.add_argument("--sigma", type=float, default=1.0)
    ap.add_argument("--radius", type=int, default=3)
    ap.add_argument("--rounds", type=int, default=8,
                     help="inner DBS rounds per single-frame solve (default 8)")
    ap.add_argument("--sweeps", type=int, default=3)
    ap.add_argument("--metric", choices=["luma", "rgb"], default="luma")
    ap.add_argument("--seeds", type=int, default=2)
    ap.add_argument("--smoothness", type=float, default=0.01)
    ap.add_argument("--frames", type=int, default=3, help="number of frames in the cyclic sequence (>=2, default 3)")
    ap.add_argument("--outer-rounds", type=int, default=4,
                     help="how many full cyclic relaxation passes across all frames (default 4). "
                          "Each pass re-solves every frame against the residual left by all OTHER "
                          "frames' CURRENT state (both directions, wraparound included -- this is "
                          "the whole point of this tool vs. the base one). Cost scales linearly "
                          "with this. Brand-new algorithm, no established plateau point yet -- "
                          "watch the printed per-round RMS and judge for yourself.")
    ap.add_argument("--quiet", action="store_true", help="suppress all progress output")
    ap.add_argument("--terse", action="store_true",
                     help="show frame-start and outer-round-finish progress (with elapsed/ETA "
                          "timing) but suppress the per-DBS-round SSE spam within each frame-solve. "
                          "A middle ground between full detail (default) and --quiet.")
    args = ap.parse_args()

    if args.width < 1 or args.height < 1:
        ap.error("--width and --height must be positive")
    if args.frames < 2:
        ap.error("--frames must be >= 2 for this tool (it only exists to coordinate multiple frames)")

    base.WIDTH = args.width
    base.HEIGHT = args.height
    base.BYTES_PER_ROW = (args.width + 7) // 8
    base.WIDTH_STRETCH = 4 * (40 / args.width)

    prefix = args.output_prefix
    if prefix is None:
        b = args.image.rsplit("/", 1)[-1].rsplit(".", 1)[0]
        prefix = b + "_cyclic"

    target = base.load_target(args.image)

    solve_kwargs = dict(rounds=args.rounds, sweeps=args.sweeps, metric=args.metric,
                         smoothness=args.smoothness)

    overall_t0 = time.monotonic()
    result = solve_n_frame_cyclic(target, n_frames=args.frames, outer_rounds=args.outer_rounds,
                                   seeds=args.seeds, sigma=args.sigma, radius=args.radius,
                                   verbose=not args.quiet, terse=args.terse, **solve_kwargs)

    color_idxs, ons = result["color_idxs"], result["ons"]
    header_path, source_path = base.write_c_files_n_frame(prefix, color_idxs, ons)

    recon_path = f"{prefix}_reconstructed.png"
    base.save_preview(result["averaged"], recon_path)
    print(f"Wrote {header_path}", flush=True)
    print(f"Wrote {source_path}", flush=True)
    print(f"Wrote {recon_path}  (the {args.frames}-frame cyclically-averaged perceived result)", flush=True)

    for i in range(args.frames):
        raw_i = np.zeros((base.HEIGHT, base.WIDTH, 3), dtype=np.float64)
        for y in range(base.HEIGHT):
            raw_i[y][ons[i][y]] = base.PALETTE_RGB[color_idxs[i][y]]
        raw_path = f"{prefix}_frame{i}_raw.png"
        base.save_preview(raw_i, raw_path)
        print(f"Wrote {raw_path}  (raw unblended TIA pixels, frame {i})", flush=True)

    jump = compute_frame_jump(color_idxs, ons, base.HEIGHT, base.WIDTH)
    total_elapsed = time.monotonic() - overall_t0
    print(f"Final cyclic {args.frames}-frame RMS (unweighted RGB, 0-255 scale): {result['final_rmse']:.2f}", flush=True)
    print(f"Mean cyclic frame-to-frame raw-pixel RMS jump: {jump:.2f}  "
          f"(flicker-magnitude metric -- compare to the RMS above)", flush=True)
    print(f"Total solve time: {total_elapsed:.1f}s", flush=True)


if __name__ == "__main__":
    main()
    