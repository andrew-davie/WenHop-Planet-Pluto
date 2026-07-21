# GRP0/GRP1 Time-Multiplexed Sprite System — Design

## What this covers, and what it doesn't

This session only had the ARM-side C code (this `main` folder) mounted. CDFJ+ carts are
built from two pieces: the ARM C code that fills RAM buffers, and a 4K bank of 6507
assembly that actually pokes TIA registers scanline-by-scanline (that's the "driver" —
`_gameLoop` etc. in `defines_dasm.h` are addresses inside it, and `bin/cdfj+.bin/elf/map`
are its compiled output). I don't have that assembly source in this session, so I
couldn't inspect what your current kernel loop can already do, and couldn't wire this in
end-to-end or verify it against Stella/hardware.

So this is a full design — the scheduling algorithm (real, working C, in
`spriteMux.c/h`), the per-scanline data format it hands off, and the exact 6502 kernel
pattern it expects on the other end — but the 6502 side is written as reference code for
you to drop into your kernel bank, not something I compiled or tested. If you connect
that folder/file I can go finish the integration for real.

## Hardware facts (verified, not assumed)

Verified against the Stella Programmer's Guide (via Bumbershoot Software's writeups and
the randomterrain.com/Andrew Davie tutorial mirror of it — sources at the bottom):

- **RESP0/RESP1** (coarse position): writing it resets the object to wherever the beam
  currently is. To hit an arbitrary X you WSYNC, then burn a *variable-length* delay loop
  (the classic 5-cycle/15-pixel `sbc #15 / bcs loop`), then write RESP0. That delay loop
  monopolizes the CPU for most of a scanline — there's no time left on that line to also
  service playfield/colour streams. **Given you need the playfield live and modifiable
  on every scanline of sprite display, RESP0/RESP1 are off the table entirely during the
  visible kernel.** They're still used once per frame, during vblank, for each lane's
  starting position — that's not a new idea, it's exactly what `drawPlayerSprite()`
  already does today via `RAM[_P0_X]`/`RAM[_P1_X]`.
- **HMP0/HMP1** (fine position): a 4-bit signed nibble, range −8..+7 color clocks
  (positive = move left, negative = move right — checked against the bit table, not
  assumed). Does nothing alone — only applied when you strobe **HMOVE**, which moves
  *every* enabled object at once. HMOVE must immediately follow a WSYNC, and the motion
  registers shouldn't be touched for ~24 cycles after.
- The standard discipline against HMOVE's "combing" artifact is to strobe it on *every*
  scanline with HMP0/HMP1 simply at 0 where nothing should move — this design does that
  unconditionally, so there's no special-casing between "moving" and "still" lines from
  the kernel's perspective, just different data.

## The one hard constraint no scheduling trick removes

**At most 2 distinct objects can be on any single scanline, ever** — you have exactly
two hardware players. Multiplexing only helps when objects are spread across
*different* scanlines within the frame. If more than 2 objects genuinely want the same
scanline band at once, something has to flicker or drop — no amount of clever scheduling
changes that, it just decides *which* one drops and makes sure it isn't always the same
one.

## Cost model

Repositioning is pure HMOVE crawl, full stop — no RESP during the visible kernel, per
the constraint above. Moving a lane from X to a new target X costs
`ceil(|dx| / 7)` scanlines of crawl (7, not 8, kept symmetric for simplicity — see
`spriteMux.c`'s `crawlLines()`). During those scanlines the lane's shape byte is 0 (it
draws nothing) while HMP0/HMP1 nudges it into place; **every one of those scanlines
still runs the full normal per-line sequence** — playfield streams, COLUP0/1, the other
lane — nothing is skipped. That's the whole point of dropping RESP: there is no longer a
scanline that costs you playfield time, only ones that cost a lane its own visibility
while it's between positions.

The one exception is each lane's *first* object of the frame, which is free (0 crawl
scanlines) — placed via the existing vblank RESP0/RESP1 mechanism, before the visible
kernel starts, same as `drawPlayerSprite()` does today.

This is a real cost, and it's steeper than a hypothetical RESP-snap model: a full
screen-width jump (dx≈159) costs `ceil(159/7)` = 23 scanlines of crawl. Distant,
Y-adjacent objects on the same lane genuinely may not fit — see the budget section.

## Scheduling algorithm

2-machine interval scheduling with a distance-dependent setup cost: "machines" are the
two hardware lanes (GRP0, GRP1), "jobs" are objects with a fixed `[yTop, yTop+height)`
window (fixed because Y is tied to the raster, not something you can time-shift), and
"setup cost" between consecutive jobs on the same lane is `ceil(|dx|/7)` scanlines (0 for
a lane's first job of the frame).

Implemented in `spriteMux.c` as greedy-by-Y with a starvation tiebreak — O(n log n) over
at most 32 objects, trivial on the ARM side:

1. Sort objects by `yTop`.
2. Walk them in order. For each object, check both lanes: does the lane have enough gap
   (`yTop - laneReadyScanline`) to cover the crawl distance to this object's X? If both
   qualify, take whichever needs less crawl. If only one qualifies, use it. If neither
   does, the object is dropped this frame and its starvation counter increments.
3. Tiebreak by starvation, not object order: when the walk has a genuine choice (both
   lanes fit, or two objects contend for the same `yTop`), the one that's gone longest
   without being shown wins. This is the flicker mitigation — without it, whatever loses
   the first frame tends to keep losing forever, which reads as "broken," not
   "flickering." With it, drops rotate.

This is a heuristic, not an optimal solver — a true optimum here is closer to a vehicle
routing problem (minimize total unmet demand across both lanes given movement cost), and
that's not worth ARM cycles for a cosmetic scheduling problem. If it's leaving obvious
wins on the table in practice, the next increment I'd reach for is a small lookahead —
try both lane assignments for the next 2-3 objects and keep whichever ordering starves
fewer things — rather than a full solver.

## Data handed to the kernel

Same shape as every other kernel in this codebase — RAM buffers, one byte per scanline,
consumed by a fixed-timing 6507 loop. `spriteMux.c` fills 6 streams via caller-supplied
RAM offsets (`MuxBuffers` in `spriteMux.h`):

```
grp0[_SCANLINES]     // shape byte for lane 0 this line (0 = blank / mid-crawl)
grp1[_SCANLINES]     // shape byte for lane 1
colup0[_SCANLINES]
colup1[_SCANLINES]
hmp0[_SCANLINES]     // pre-shifted HMOVE nibble, 0 = no motion this line
hmp1[_SCANLINES]
```

`grp0/grp1/colup0/colup1` are literally the same shape of stream you already have wired
up as `_DS_GAME_GRP0A`/`_DS_GAME_GRP1A`/`_DS_GAME_COLUP0`/`_DS_GAME_COLUP1` — the mux
system can feed those existing streams directly if it's replacing `drawPlayerSprite()`
for a given frame, no new stream slots needed there.

`hmp0`/`hmp1` are the only genuinely new streams — 2 offsets, and on the kernel side, 2
more fixed-cost register pokes per scanline. Nothing here needs a per-scanline branch:
every scanline runs the identical instruction sequence below, just with HMP0/HMP1
sometimes nonzero. That's a much smaller, easier-to-budget ask than the RESP-based
version of this design (which needed a conditional branch and a variable-length routine
on reposition lines) — flagging that the earlier draft of this doc got that wrong before
you caught it.

## 6502 kernel loop (reference pattern, not yet wired into your driver)

Every scanline runs the same sequence, unconditionally — no branching between "normal"
and "repositioning" lines, because there's no such distinction anymore:

```asm
; --- every scanline, uniformly ---
        sta     WSYNC
        sta     HMOVE           ; applies HMP0/HMP1 queued by the previous line

        lda     grp0,y          ; y = current scanline index
        sta     GRP0
        lda     grp1,y
        sta     GRP1
        lda     colup0,y
        sta     COLUP0
        lda     colup1,y
        sta     COLUP1

        lda     hmp0,y          ; queue *this* crawl step, applied by *next* HMOVE
        sta     HMP0
        lda     hmp1,y
        sta     HMP1
```

That's the entire per-scanline addition on top of what you already do for
GRP0A/GRP1A/COLUP0/COLUP1 — 2 more loads, 2 more stores, always the same shape. The
vblank-side placement (once per frame, per lane, for whichever object is first on that
lane) is exactly `drawPlayerSprite()`'s existing `RAM[_P0_X] = pX` pattern — `muxSchedule()`
writes those same two bytes at the end of scheduling, nothing new needed there either.

## Budget reality check on "32 objects at the same time"

"At the same time" only really applies within the hard 2-per-scanline cap — 32 objects
existing *somewhere on screen in the same frame* is the real target, and with pure-HMOVE
repositioning, how comfortable that is depends heavily on how far apart in X your
Y-adjacent objects tend to be, not just their count or height.

`_SCANLINES` is 198 here. Two extremes:

- **Objects that are already roughly where the previous one on that lane left off**
  (small dx, e.g. a cluster of creatures walking near each other): crawl cost is small
  or zero, and capacity is dominated by height, same as before — ~22 objects/lane at
  8px tall, ~44 across both lanes. 32 fits comfortably.
- **Objects scattered across the full X range, Y-adjacent** (e.g. one creature far left,
  the next far right, both wanting nearby scanlines): each full-width transition costs
  up to 23 scanlines of crawl. A lane doing several of those in a row eats its budget
  fast — a handful of far-apart transitions can burn most of a 198-line frame on one
  lane alone.

The aggregate numbers above are an average-case ceiling, not a guarantee, precisely
because of the 2-per-scanline cap plus this distance sensitivity. If your 32 objects
tend to be spatially clustered when they're also temporally (Y-)close, you're in the
comfortable case. If they're scattered across X and still want similar Y — e.g. a wide
horizontal spread of enemies all near the same height — expect real flicker, and it's
not something the scheduler can schedule its way out of; that's genuinely more objects
wanting the same 2-lane, X-constrained resource than it can serve. Worth checking your
actual object distribution against this before assuming 32 is free.

## Sources

- [An Arbitrary Sprite Positioning Routine For The Atari 2600 — Bumbershoot Software](https://bumbershootsoft.wordpress.com/2018/08/30/an-arbitrary-sprite-positioning-routine-for-the-atari-2600/)
- [Atari 2600 Programming for Newbies — Session 22 (Part 2): Horizontal Positioning / HMOVE](https://www.randomterrain.com/atari-2600-memories-tutorial-andrew-davie-22b.html)
- [Stella Programmer's Guide](https://alienbill.com/2600/101/docs/stella.html)
