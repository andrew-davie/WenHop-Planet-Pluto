#include "defines_dasm.h"

#include "cdfjplus.h"


#include "main.h"


#if ENABLE_SWIPE

#include "particle.h"    // sin_cos[32] -- used for the star's one-time random rotation
#include "random.h"      // rangeRandom() -- ditto
#include "swipe.h"

// per-column bit within the byte a given x-coordinate (0-19, one screen half) maps to
static const unsigned char rebase[20] = {
    1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 7, 1 << 6, 1 << 5, 1 << 4, 1 << 3, 1 << 2,
    1 << 1, 1 << 0, 1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7,
};

// aligned(4) on all four buffers below: clearBorderCur()/clearBorderPrev()/
// clearMask()/copyBuf() all bulk-access these 4 bytes at a time via an
// unsigned int* cast for speed. ARMv4T doesn't handle unaligned word access
// safely, and a plain "unsigned char[]" isn't guaranteed 4-byte aligned by
// the compiler on its own -- this attribute makes that guarantee explicit
// instead of relying on it happening to work.
//
// swipeMaskA/B: the revealed/hidden mask, double-buffered the same way the
// border is (see maskShow/swipeWriteMask below) -- was a single buffer,
// updated immediately in place, until it was observed on hardware that the
// mask could visibly run ahead of the (already double-buffered) border
// during a multi-frame lap: rows/segments already processed this lap had
// their mask pulled in to the new edge right away, while the border stayed
// frozen at last lap's position until the whole lap finished, briefly
// exposing revealed content beyond/outside where the border line actually
// sat. Now both buffers only ever become visible together, at the same
// lap-boundary swap.
static unsigned char swipeMaskA[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char swipeMaskB[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));

// Border ring: NOT cumulative like the mask -- a lap fully redraws its own
// connected trace from scratch rather than only touching a changed band,
// so its double buffer (below) doesn't need a copy-forward step the way
// the mask's does (see copyBuf()'s call site in swipe()).
static unsigned char borderMaskCur[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char borderMaskPrev[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));

// applySwipeMask() reads *borderShowA | *borderShowB per pixel rather than
// the two physical buffers by name, so BOTH swipe types can share the exact
// same real double-buffer scheme (they used to use two different, opposite
// ones -- see the git history on this comment if curious):
//
//   swipeWriteBorder (declared below) names whichever physical buffer THIS
//   lap's border-drawing calls (drawBorderBit() for circle,
//   bresenhamLine()'s border half for star) are actually writing into --
//   always the buffer that is NOT currently on screen. borderShowA and
//   borderShowB both point at the OTHER one (the last lap that finished,
//   fully connected, nothing partial) -- ORing a buffer with itself is just
//   itself, so this shows exactly one complete ring/outline, never a blend
//   of two, never a half-finished one. The swap happens once per lap, in
//   swipe()'s newLapPending handling, the moment the previous lap is
//   confirmed complete -- a multi-frame lap just holds last lap's finished
//   trace steady for however many extra frames it needs instead of showing
//   it mid-redraw, which costs nothing extra: the radius/size only ever
//   advances once per lap, not per frame, so neither shape was ever meant
//   to move faster than that anyway.
//
//   CIRCLE was the reason this exists: a hold-and-OR (see below) was tried
//   here first and caused a "ghost ring" bug -- for SHRINK, the held
//   previous lap's ring is always LARGER than the current one, so it sits
//   entirely outside the current disk, in territory the mask has already
//   hidden, and the border-forces-on rule overrides that, showing a stale
//   ring floating in blank space for an extra frame every lap. Circle also
//   can't get away with NO hold at all: a single lap's row loop can span
//   several video frames for a large disk, and the border used to be
//   cleared at lap-start and filled in row by row as budget allowed, so
//   applySwipeMask() -- which runs every frame regardless -- displayed
//   whatever partial trace existed at that point: the poles (the last rows
//   computed) were genuinely missing for however many frames the lap took,
//   read as the bottom of the circle flashing on/off. The double buffer
//   fixes both at once.
//
//   STAR used a DIFFERENT scheme up to this point: hold the previous lap's
//   complete trace and OR it with this lap's, so a pixel lit for only one
//   lap still shows for 2 consecutive frames -- needed because star's
//   border is a handful of long, coarsely-stepped segments that trace
//   different pixels lap to lap; without SOME persistence, a pixel lit for
//   a single frame reads as a flash. That worked, but blending two
//   different-sized copies of the same star (they're the same 10 angles,
//   just scaled) together is ALSO what made the outline look double-wide
//   for most of a sequence -- the whole point of switching star to the
//   same swap-not-blend scheme circle uses. Risk worth watching for on
//   hardware: the ORIGINAL reason the hold existed also mentioned that
//   fixed-point rounding can make several consecutive laps early in a GROW
//   (see STAR_GROW_RAMP_LAPS) trace almost-but-not-quite the same pixels,
//   which the hold's OR papered over "for free" -- swap has no such
//   fallback, so if THAT specific flicker reappears at the very start of a
//   grow, it needs its own fix rather than reintroducing the blend
//   permanently (which is what caused double-wide in the first place).
static unsigned char (*borderShowA)[SCREEN_TRIX_Y] = borderMaskCur;
static unsigned char (*borderShowB)[SCREEN_TRIX_Y] = borderMaskPrev;

// See borderShowA/B's comment above. Always the physical buffer currently
// NOT named by borderShowA/B. Shared by both swipe types (only one is ever
// active at a time) -- drawBorderBit() (circle) and bresenhamLine()'s
// border half (star) both target this unconditionally, no swipeType check
// needed either place.
static unsigned char (*swipeWriteBorder)[SCREEN_TRIX_Y] = borderMaskPrev;

// Mask's own display/write pair -- same idea as borderShowA/B, but only one
// display pointer is needed (never OR'd with anything; the mask was never a
// blend, just a single revealed/hidden bitmap). applySwipeMask() reads
// *maskShow instead of the literal swipeMaskA/B name. fillMaskSpan() (circle)
// and bresenhamLine()'s mask half (star) both write through swipeWriteMask.
//
// Unlike the border, a new lap's write target can't just start cleared --
// the mask is CUMULATIVE (each lap only touches the thin band between old
// and new radius/size; everything outside that band has to carry over
// unchanged from every earlier lap), so its write buffer has to start as a
// full copy of whatever's currently displayed, THEN this lap's span-fills
// apply on top of that copy. See copyBuf()'s call site in swipe() for where
// that copy happens -- same lap-boundary moment the border's clear already
// happens unconditionally (no time-budget check), which was never a
// problem for a buffer this size at that point in a lap, so doubling that
// one-time cost here follows the same reasoning.
static unsigned char (*maskShow)[SCREEN_TRIX_Y] = swipeMaskA;
static unsigned char (*swipeWriteMask)[SCREEN_TRIX_Y] = swipeMaskB;

static void fillMaskSpan(int x0, int x1, int y);
bool drawBorderBit(int x, int y);
bool bresenhamBorderLine(int x0, int y0, int x1, int y1);
void generateStar();
bool star();
bool circle();
void initStar(int x, int y);
static int isqrt(int n);
static int squashDy2(int dy);

static short starX[10];
static short starY[10];

static int swipeRadius;    // 24.8
static int swipeStep;      // 24.8

// Star's GROW used a flat per-lap step from the very first lap, which
// covered so much ground so fast that the iris-in's actual genesis (the
// tiny star appearing at the player) flashed past before it was visible --
// read as if it just appeared at a decent size already open. Ramps the
// EFFECTIVE per-lap step from 1/STAR_GROW_RAMP_LAPS up to the full target
// over the first STAR_GROW_RAMP_LAPS laps instead (see its use in
// setSwipe()), so the opening few frames are slow enough to actually see
// it start, while later laps (where the speed was already fine, and is now
// bumped a bit faster still -- see decodeCaves.c's GROW step) are
// unaffected. starGrowLapCount is reset once per fresh GROW sequence by
// randomizeStarAngle() (same "once per fresh sequence, not per lap" hook
// already used for the rotation) -- harmless that it also resets for a
// star SHRINK-close, since the ramp only ever applies when
// phase == SWIPE_GROW.
#define STAR_GROW_RAMP_LAPS 8
static int starGrowLapCount;

// One-time star rotation, chosen once per fresh star sequence (see
// randomizeStarAngle()) and held fixed for every lap of that sequence --
// generateStar() re-runs every lap (setSwipe() calls it each time to
// configure the next lap), so the rotation has to live outside that
// function or it'd get re-rolled mid-sequence, breaking the close overlap
// between consecutive laps that the fill relies on. These come straight out
// of sin_cos[32] (particle.h), which is Q8 (256 ~= 1.0), NOT the Q15 scale
// starSine/starCosine use below -- generateStar() shifts accordingly.
static int starRotSin = 0;
static int starRotCos = 256;    // identity rotation by default (Q8: 256 = 1.0)

static int swipeCenterX;
static int swipeCenterY;

// Replaces the old Bresenham-circle point tracker (circleX/circleY/
// circleDelta): circle() now computes the left/right edge of each
// horizontal row directly (see circle()'s comment for why), so there's no
// incremental octant-walking state to keep -- just the current radius and
// which row we're up to.
//
// circleRow now counts dy OUTWARD from the centre row (0 .. sweep), not
// from -sweep to sweep -- circle() processes the row at +dy and the row at
// -dy together off a single isqrt() call, since they're always the same
// half-width by construction. Halves the isqrt() count per lap.
static int circleRadius;      // current lap's disk radius, whole trix
static int circleRadiusSq;    // circleRadius*circleRadius, hoisted out of circle()'s per-row
                               // loop -- same value all lap, no need to re-multiply every row
static int circleRow;         // |dy| cursor this lap, 0 .. sweep

// Tracks the previous row's edge points so circle() can connect consecutive
// rows' edges with an actual line instead of leaving them as isolated dots
// -- see circle()'s comment. Processing top and bottom rows outward from
// the centre means they're two independent chains (both starting from the
// shared centre-row point at dy==0), so each needs its own prev-point
// state. *HasPrev is false at the start of each lap and whenever a chain's
// trace has just stepped outside the disk (no edge to carry across the
// gap).
//
// Tried committing a row only if its value survived into the next row too
// (to drop single-trix-tall "teeth" near the poles) -- reverted: which
// rows count as a lone tooth depends on the exact radius, so the set of
// skipped rows near the poles differs from one lap to the next (radius
// shrinks by 1 trix/lap). That reshapes the whole pole region every lap
// instead of just trimming a stray pixel, which read as the border
// flickering/jittering -- worse the bigger the radius (more single-row
// candidates to disagree about lap to lap), confirmed by simulation.
// Committing every row unconditionally is geometrically less "clean" right
// at the poles but stable lap to lap, which matters far more.
static int topPrevLeft, topPrevRight, topPrevRow;
static bool topHasPrev;
static int botPrevLeft, botPrevRight, botPrevRow;
static bool botHasPrev;

// Lets circle() skip re-filling mask bands that didn't change since last
// lap (see circle()'s comment) WITHOUT a per-row history cache -- SHRINK's
// radius decreases by exactly `step` every lap (constant for a whole
// sequence), so last lap's radius is just circleRadius + step, recomputed
// on the fly with one extra isqrt() per row rather than ~540 bytes of
// static per-row storage (tried, reverted: not worth the RAM for the
// win). circleHasOldRadius is false only for a sequence's very first lap
// (nothing to derive an old radius from -- the mask is uniformly
// fully-revealed/hidden at that point, not shaped like any previous
// circle), set via circleFreshSequence by startSwipeClose() and
// decodeCaves.c's GROW-start call, same reasoning/pattern as
// randomizeStarAngle().
static bool circleFreshSequence = true;
static bool circleHasOldRadius;
static int oldRadiusSq;    // oldRadius*oldRadius, hoisted the same way as circleRadiusSq --
                           // oldRadius is constant for the whole lap (derived from circleRadius
                           // and swipeStep, neither of which change mid-lap), so this only needs
                           // computing once per lap instead of once per row.

void markCircleFreshSequence() {
    circleFreshSequence = true;
}

static bool swipeComplete;
static bool swipeVisible;

// Safety valve for a LOGIC stall -- something not advancing when it should
// (e.g. a future bug leaves swipeComplete permanently false). Every real
// sequence we've measured finishes in well under 100 frames (worst-case
// simulated circle shrink was ~85 laps), so anything still running after
// SWIPE_WATCHDOG_FRAMES is not "just slow", it's stuck -- force it closed
// rather than leave the player staring at a frozen swipe forever.
// Self-resetting, no separate "start of sequence" hook needed: swipe()
// zeroes it every frame the sequence is genuinely idle (swipeComplete
// true), which is the state for every ordinary gameplay frame between
// sequences -- so it's always back at 0 well before the next setSwipe()
// call flips swipeComplete false again and starts it counting.
//
// NOTE: this only catches a stall in swipe()'s own logic -- it can't do
// anything about a genuine ARM timing overrun (busted the frame's cycle
// budget badly enough to desync the 6502/TIA side), since that kind of
// crash can take the whole system down before this code ever gets to run
// again. The real defence against THAT is keeping every path in this file
// inside its time budget in the first place, not a software fallback.
#define SWIPE_WATCHDOG_FRAMES 600
static int swipeWatchdogFrames;

// Finishing a sequence (SHRINK's clearMask(0)+clearBorderCur(), GROW's
// clearMask(255)) is real, unavoidable work -- up to ~200 words. It used
// to run completely unconditionally the instant the last lap's
// termination check tripped, with no time-budget check at all, landing
// exactly when the lap that just finished may have already spent the
// frame's whole budget getting there -- reported as a CPU jam right as
// the circle was about to finish.
//
// First attempt deferred the WHOLE clear until T1TC showed some fixed
// margin of slack -- wrong fix: with reserved already tuned right up
// against the edge, that slack might not exist on ANY frame, which just
// traded an occasional overrun crash for a deterministic hang at the same
// spot (still reported as jammed, now every time instead of sometimes).
//
// Correct fix: clear it the same way circle()'s own row loop already
// handles its budget -- one small unit of work at a time, re-checking
// T1TC before each one, so even a razor-thin frame still makes SOME
// forward progress (worst case a single word) instead of requiring a
// specific amount of slack to exist all at once. finishStage/finishIndex
// track exactly where this incremental clear is up to, across as many
// frames as it actually takes -- no arbitrary margin constant involved,
// no sweep-speed change involved either.
enum {
    FINISH_MASK_A,        // clearing swipeMaskA (255 for GROW, 0 for SHRINK)
    FINISH_MASK_B,        // clearing swipeMaskB (255 for GROW, 0 for SHRINK) -- both physical mask
                          // buffers need setting now that the mask is double-buffered too
    FINISH_BORDER_CUR,    // clearing borderMaskCur (always -- real data every time)
    FINISH_BORDER_PREV,   // clearing borderMaskPrev (skipped entirely if already known zero)
    FINISH_DONE
};
static unsigned char finishStage;
static int finishIndex;    // word cursor within whichever buffer finishStage currently names
static bool finishPending;

bool maskNeeded = true;           // false once fully open (grown-in) -- lets applySwipeMask skip the AND loop
static bool maskWhite = false;    // true: hidden area forced ON (COLUPF/white); false: forced OFF (background/black)

static enum CIRCLEPHASE swipePhase;
static enum SWIPE swipeType;


enum PHASE {
    PHASE_NONE = 0,
#if ENABLE_PERFECT
    PHASE_PERFECT = 1,
#endif
    PHASE_TIME = 2,
    PHASE_GEMS = 3,
    PHASE_SWIPE = 4,
    PHASE_END = 5
};

static enum PHASE exitPhase;


// buffer is the left column (PF0_LEFT) of whichever kernel's six
// PF0/PF1/PF2 LEFT/RIGHT planes we're masking; each plane is
// _BUFFER_SIZE apart, so address each one from buffer rather than
// walking a single pointer across all six (they aren't contiguous
// with the SCREEN_TRIX_Y*3 content -- there's _BUFFER_SIZE - _SCANLINES
// bytes of slack after each plane).
// Each row writes the same byte 3 times (one per real scanline), so a row
// doesn't align to a 4-byte int on its own -- but every 4 rows (12 bytes)
// is exactly 3 ints, so we bulk-process 4 rows at a time as 3 int32 writes
// (one read-modify-write instead of four).
// Endianness assumption: little-endian (standard for this target), byte0 =
// low byte of the int.
// Split into two colour-specific variants (rather than branching on
// maskWhite inside the loop) so the hot path never re-tests it per int --
// applySwipeMask() below picks one once, outside any loop.
//
// Per pixel: if the border (borderShowA | borderShowB -- see their
// declaration for what these actually name per swipe type) is set, force
// the bit fully ON (that's the "all
// pixels set" ring at the current edge of the swipe, same in both colour
// modes). Otherwise, if maskShow (revealed) is set, leave the real content
// alone. Otherwise, force per the black/white hidden-area rule as before.
// Per-int this collapses to:
//   black: final = (orig & R) | B
//   white: final = orig | (~R | B)
// Skip-if-no-op check is the same for both: nothing to do if the relevant
// revealed rows are all 0xFF AND the relevant border rows are all 0.

static void applySwipeMaskBlack(int buffer) {

    for (int col = 0; col < 6; col++) {
        unsigned char *p = RAM + buffer + col * _BUFFER_SIZE;
        int y = 0;

        for (; y + 4 <= SCREEN_TRIX_Y; y += 4) {
            unsigned int r0 = maskShow[col][y];
            unsigned int r1 = maskShow[col][y + 1];
            unsigned int r2 = maskShow[col][y + 2];
            unsigned int r3 = maskShow[col][y + 3];
            unsigned int b0 = borderShowA[col][y] | borderShowB[col][y];
            unsigned int b1 = borderShowA[col][y + 1] | borderShowB[col][y + 1];
            unsigned int b2 = borderShowA[col][y + 2] | borderShowB[col][y + 2];
            unsigned int b3 = borderShowA[col][y + 3] | borderShowB[col][y + 3];
            unsigned int *ip = (unsigned int *)p;

            // int0 = [r0,r0,r0,r1] / [b0,b0,b0,b1], int1 = [r1,r1,r2,r2] / [b1,b1,b2,b2], int2 = [r2,r3,r3,r3] /
            // [b2,b3,b3,b3]
            if (!(r0 == 0xFF && r1 == 0xFF && b0 == 0 && b1 == 0)) {
                unsigned int R = r0 | (r0 << 8) | (r0 << 16) | (r1 << 24);
                unsigned int B = b0 | (b0 << 8) | (b0 << 16) | (b1 << 24);
                ip[0] = (ip[0] & R) | B;
            }
            if (!(r1 == 0xFF && r2 == 0xFF && b1 == 0 && b2 == 0)) {
                unsigned int R = r1 | (r1 << 8) | (r2 << 16) | (r2 << 24);
                unsigned int B = b1 | (b1 << 8) | (b2 << 16) | (b2 << 24);
                ip[1] = (ip[1] & R) | B;
            }
            if (!(r2 == 0xFF && r3 == 0xFF && b2 == 0 && b3 == 0)) {
                unsigned int R = r2 | (r3 << 8) | (r3 << 16) | (r3 << 24);
                unsigned int B = b2 | (b3 << 8) | (b3 << 16) | (b3 << 24);
                ip[2] = (ip[2] & R) | B;
            }

            p += 12;
        }

        // Tail: same 2-leftover-rows-plus-padding handling as before, now also
        // folding in the border rows the same way.
        if (y < SCREEN_TRIX_Y) {
            unsigned int r0 = maskShow[col][y];
            unsigned int r1 = maskShow[col][y + 1];
            unsigned int b0 = borderShowA[col][y] | borderShowB[col][y];
            unsigned int b1 = borderShowA[col][y + 1] | borderShowB[col][y + 1];
            unsigned int *ip = (unsigned int *)p;

            if (!(r0 == 0xFF && r1 == 0xFF && b0 == 0 && b1 == 0)) {
                unsigned int R = r0 | (r0 << 8) | (r0 << 16) | (r1 << 24);
                unsigned int B = b0 | (b0 << 8) | (b0 << 16) | (b1 << 24);
                ip[0] = (ip[0] & R) | B;
            }
            if (!(r1 == 0xFF && b1 == 0)) {
                unsigned int R = r1 | (r1 << 8) | (r1 << 16) | (r1 << 24);
                unsigned int B = b1 | (b1 << 8) | (b1 << 16) | (b1 << 24);
                ip[1] = (ip[1] & R) | B;
            }
        }
    }
}

static void applySwipeMaskWhite(int buffer) {

    for (int col = 0; col < 6; col++) {
        unsigned char *p = RAM + buffer + col * _BUFFER_SIZE;
        int y = 0;

        for (; y + 4 <= SCREEN_TRIX_Y; y += 4) {
            unsigned int r0 = maskShow[col][y];
            unsigned int r1 = maskShow[col][y + 1];
            unsigned int r2 = maskShow[col][y + 2];
            unsigned int r3 = maskShow[col][y + 3];
            unsigned int b0 = borderShowA[col][y] | borderShowB[col][y];
            unsigned int b1 = borderShowA[col][y + 1] | borderShowB[col][y + 1];
            unsigned int b2 = borderShowA[col][y + 2] | borderShowB[col][y + 2];
            unsigned int b3 = borderShowA[col][y + 3] | borderShowB[col][y + 3];
            unsigned int *ip = (unsigned int *)p;

            if (!(r0 == 0xFF && r1 == 0xFF && b0 == 0 && b1 == 0)) {
                unsigned int R = r0 | (r0 << 8) | (r0 << 16) | (r1 << 24);
                unsigned int B = b0 | (b0 << 8) | (b0 << 16) | (b1 << 24);
                ip[0] |= ~R | B;
            }
            if (!(r1 == 0xFF && r2 == 0xFF && b1 == 0 && b2 == 0)) {
                unsigned int R = r1 | (r1 << 8) | (r2 << 16) | (r2 << 24);
                unsigned int B = b1 | (b1 << 8) | (b2 << 16) | (b2 << 24);
                ip[1] |= ~R | B;
            }
            if (!(r2 == 0xFF && r3 == 0xFF && b2 == 0 && b3 == 0)) {
                unsigned int R = r2 | (r3 << 8) | (r3 << 16) | (r3 << 24);
                unsigned int B = b2 | (b3 << 8) | (b3 << 16) | (b3 << 24);
                ip[2] |= ~R | B;
            }

            p += 12;
        }

        if (y < SCREEN_TRIX_Y) {
            unsigned int r0 = maskShow[col][y];
            unsigned int r1 = maskShow[col][y + 1];
            unsigned int b0 = borderShowA[col][y] | borderShowB[col][y];
            unsigned int b1 = borderShowA[col][y + 1] | borderShowB[col][y + 1];
            unsigned int *ip = (unsigned int *)p;

            if (!(r0 == 0xFF && r1 == 0xFF && b0 == 0 && b1 == 0)) {
                unsigned int R = r0 | (r0 << 8) | (r0 << 16) | (r1 << 24);
                unsigned int B = b0 | (b0 << 8) | (b0 << 16) | (b1 << 24);
                ip[0] |= ~R | B;
            }
            if (!(r1 == 0xFF && b1 == 0)) {
                unsigned int R = r1 | (r1 << 8) | (r1 << 16) | (r1 << 24);
                unsigned int B = b1 | (b1 << 8) | (b1 << 16) | (b1 << 24);
                ip[1] |= ~R | B;
            }
        }
    }
}

void applySwipeMask(int buffer) {

    // Skip entirely once the mask is known fully-open (all 255 -- a no-op AND
    // either way, so there's nothing to gain by running it). Do NOT gate this
    // on swipeComplete generally though -- that was the previous bug: shrink
    // finishing also sets swipeComplete, but the mask is mostly-0 at that
    // point, not open, so skipping there let the raw buffer flash through.
    // maskNeeded only goes false when a grow genuinely finishes (see swipe()).
    if (!maskNeeded)
        return;

    if (maskWhite)
        applySwipeMaskWhite(buffer);
    else
        applySwipeMaskBlack(buffer);
}

void setSwipeMaskColour(bool white) {
    maskWhite = white;
}

bool checkSwipeFinished() {
    return swipeComplete;
}

void setSwipeType(enum SWIPE newSwipeType) {
    swipeType = newSwipeType;
}

void setSwipePhase(enum CIRCLEPHASE newPhase) {
    swipePhase = newPhase;
}


// Byte count of one of these buffers, as a word count (+ tail bytes if it
// doesn't divide evenly -- it currently does, 6*SCREEN_TRIX_Y=396=99*4, but
// this stays correct if that ever changes instead of silently dropping the
// remainder).
#define MASK_BUF_BYTES (6 * SCREEN_TRIX_Y)
#define MASK_BUF_WORDS (MASK_BUF_BYTES / 4)

static void clearBorderCur() {
    unsigned int *p = (unsigned int *)borderMaskCur;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)borderMaskCur)[i] = 0;
}

// Tracks whether borderMaskPrev is already known to be all-zero, so a
// redundant clear can be skipped -- see clearBorderPrev()'s call site in
// swipe() for why this is safe.
static bool borderPrevIsZero;

static void clearBorderPrev() {
    unsigned int *p = (unsigned int *)borderMaskPrev;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)borderMaskPrev)[i] = 0;
    borderPrevIsZero = true;
}

// Same clear as clearBorderCur()/clearBorderPrev() above, but by pointer --
// needed because the write target (swipeWriteBorder) alternates between
// the two physical buffers lap to lap (see borderShowA/B's declaration).
static void clearBorderBuf(unsigned char (*buf)[SCREEN_TRIX_Y]) {
    unsigned int *p = (unsigned int *)buf;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)buf)[i] = 0;
}

// Copies one [6][SCREEN_TRIX_Y] buffer over another -- used to seed the
// mask's write target with everything accumulated so far before this lap's
// span-fills apply on top of it (see swipeWriteMask's declaration). Run
// unconditionally, no time-budget check, same as clearBorderBuf() above --
// this happens at the same lap-boundary moment the border's own
// unconditional clear already does, where there's normally a whole lap's
// worth of budget still ahead, not the razor-thin, budget-already-spent
// moment finishPending's incremental clear exists for.
static void copyBuf(unsigned char (*dst)[SCREEN_TRIX_Y], unsigned char (*src)[SCREEN_TRIX_Y]) {
    unsigned int *pd = (unsigned int *)dst;
    unsigned int *ps = (unsigned int *)src;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        pd[i] = ps[i];
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)dst)[i] = ((unsigned char *)src)[i];
}

// Gate the lap-transition handling on "a new lap started", not "a new frame
// happened", so this stays correct even if a lap ever does span multiple
// frames (we don't want to disturb swipeWriteBorder mid-trace).
static bool newLapPending;

void setSwipe(int x, int y, int radius, int step, enum CIRCLEPHASE phase) {

    swipeCenterX = x;
    swipeCenterY = y;

    int effectiveStep = step;
    if (swipeType == SWIPE_STAR && phase == SWIPE_GROW && starGrowLapCount < STAR_GROW_RAMP_LAPS) {
        // Ease-in -- see starGrowLapCount's declaration. Ramps the radius
        // increment actually applied THIS lap from 1/STAR_GROW_RAMP_LAPS up
        // to the full step over the first STAR_GROW_RAMP_LAPS laps --
        // swipeStep itself still gets the real, unramped target value below,
        // so nothing else that reads it (there's no such reader for STAR
        // today, but keep it honest regardless) sees anything other than the
        // true rate. STAR_GROW_RAMP_LAPS is a power of 2 so this stays a
        // shift, not a real divide.
        effectiveStep = (step * (starGrowLapCount + 1)) >> 3;
        starGrowLapCount++;
    }

    swipeRadius = radius + effectiveStep;    // 24.8
    swipeStep = step;                        // 24.8 -- true target rate, not the ramped one

    swipeComplete = false;
    swipeVisible = false;
    swipePhase = phase;
    newLapPending = true;

    switch (swipeType) {
    case SWIPE_CIRCLE: {
        circleRadius = swipeRadius >> 8;
        circleRadiusSq = circleRadius * circleRadius;
        // For SHRINK, sweep circleRadius PLUS one step's worth of margin --
        // not the current radius alone, and NOT the whole original starting
        // extent either (that was tried and swept ~150-190 rows every
        // single lap regardless of how far the disk had shrunk, which was
        // too much work to fit in one frame's time budget most laps -- it
        // visibly spilled across several frames, reading as a slow top-to-
        // bottom sweep instead of the disk shrinking).
        //
        // The margin only needs to cover the band of rows that JUST fell
        // outside the disk since last lap (radius shrank by exactly one
        // step), so they can be explicitly, fully hidden the same lap they
        // exit the disk -- rather than frozen at their last partial reveal
        // until the final blanket clear. That's a band `step` trix wide,
        // not the whole original radius, so the sweep shrinks along with
        // the disk instead of staying fixed at its largest size the entire
        // sequence. GROW never needs to retract anything already revealed,
        // so it just sweeps its own current (growing) extent, no margin.
        // circleRow now runs 0 .. sweep (see its declaration) -- each call
        // handles the +dy and -dy row together off one isqrt().
        circleRow = 0;
        topHasPrev = false;    // fresh lap -- nothing to connect the first edge point to yet
        botHasPrev = false;
        circleHasOldRadius = !circleFreshSequence;    // false only for a sequence's very first lap
        circleFreshSequence = false;                  // consumed -- every later lap this sequence has an old radius
        if (circleHasOldRadius) {
            int oldRadius = circleRadius - (swipeStep >> 8);
            oldRadiusSq = oldRadius * oldRadius;    // hoisted -- see oldRadiusSq's declaration
        }
        break;
    }

    case SWIPE_STAR:
        generateStar();
        break;
    }
}

// Radius (in whole trix, not 24.8) a circle centred at (cx, cy) needs to
// fully cover the screen. Takes the centre as explicit PARAMETERS -- NOT
// swipeCenterX/Y (a prior version read those globals directly instead, which
// was wrong: startSwipeClose() calls this BEFORE setSwipe() updates
// swipeCenterX/Y to the new death position, so it was computing the covering
// radius for wherever the PREVIOUS sequence had been centred -- e.g. the
// star's level-start position -- not the death position it was about to draw
// around. Confirmed by simulation: for ~1 in 6 randomised (stale-centre,
// real-centre) pairs, the mismatch was bad enough that the radius handed to
// the very first lap didn't fit the ACTUAL draw centre at all -- the ring
// missed the screen in both x and y for every row of that lap, tripping
// circle()'s (otherwise correct) "not visible this lap" safety check
// immediately: PF snaps to black with no ring ever drawn, then straight to
// the post-death transition once shakeTime runs out. Matches "instant black,
// no border, next level ~10-20 frames later" exactly, and explains why it
// only happened "a couple of instances" -- it depends on how far the player
// had wandered from the level's start position before dying.
//
// Combines dx and dy into a true corner distance -- NOT max(rForX, rForY)
// (a prior version took the max of "radius needed to cover dx alone" and
// "radius needed to cover dy alone" independently, which is wrong: that only
// guarantees each AXIS is covered on its own, not the actual corner where
// both are farthest simultaneously. A corner needs radius >= the combined
// (aspect/squash-corrected) distance to it, which is bigger than either
// axis's own figure whenever both dx and dy are nonzero -- i.e. almost
// always, unless the player happens to be exactly on the centre row or
// column. Confirmed by simulation this was under-covering the corners at
// EVERY single one of the 2640 possible on-screen positions -- not an edge
// case, the formula was simply wrong throughout. Fixed by solving circle()'s
// own per-row equation (halfWidth = isqrt(R^2 - squashDy2(dy)) * 7/16) for R
// at the exact corner's (dx, dy) offset: R = isqrt(squashDy2(dy) + rForX^2).
// Re-verified the same way: 0/2640 positions fail to fully cover the screen
// with this formula (same +2 margin as before).
//
// rForX itself is still aspect-scaled (NOT a plain Euclidean dx): circle()
// scales its x-extent by 7/16 relative to the true (isotropic) half-width
// (halfWidth = (w*7)>>4) as a display aspect-ratio correction -- trix
// columns are physically wider than trix rows are tall, so the x-reach
// needs compressing for the shape to actually look round. That means the
// radius needed to cover the x-extent is dx*16/7, not just dx.
static int circleCoverRadius(int cx, int cy) {
    int dx = cx > (_1ROW - 1 - cx) ? cx : (_1ROW - 1 - cx);
    int dy = cy > (SCREEN_TRIX_Y - 1 - cy) ? cy : (SCREEN_TRIX_Y - 1 - cy);
    int rForX = ((dx * 16 + 6) * (0x10000 / 7)) >> 16;    // ceil(dx*16/7)
    int dy2 = squashDy2(dy);                              // same squash circle() applies per-row
    int r = isqrt(dy2 + rForX * rForX);
    return r + 2;    // small safety margin
}

void startSwipeClose(int x, int y) {

    // Centre on the caller's supplied position (the player's position at the
    // moment this is called), NOT swipeCenterX/Y left over from whatever the
    // last setSwipe() was -- that used to be the spawn/level-start centre
    // from GROW, which is wrong for a death happening anywhere else on the
    // board.
    //
    // Radius: NOT swipeRadius unless we're still the same shape: a star has
    // to grow well past simple screen coverage before its pointed shape (and
    // the narrower "waist" between points) actually clears every corner, so
    // swipeRadius at the point GROW completes is bigger than a circle needs.
    // Handing that straight to a circle shrink meant almost the whole
    // countdown ran through radius values that were still fully covering (no
    // visible change), and the actual visible shrink -- from "just barely
    // covering" down to 0 -- was crammed into the last couple of laps,
    // reading as "disappears almost instantly". Computing circle's own
    // covering radius here instead fixes that; star keeps reusing
    // swipeRadius as before, since that IS the correct value when grow and
    // shrink are both star.
    //
    // Step: NOT star's own per-lap step (3 trix) reused directly. Circle no
    // longer holds border data across laps at all (see swipe()) -- each lap
    // draws a fully connected, tall-pixel outline of its own. But a coarse
    // step still means consecutive laps' outlines sit several trix apart,
    // which reads as visible gaps/jumps between frames. A fine 1-trix step
    // keeps consecutive laps close enough to look like continuous motion.
    int radius = (swipeType == SWIPE_CIRCLE) ? (circleCoverRadius(x, y) << 8) : swipeRadius;
    int step = (swipeType == SWIPE_CIRCLE) ? (1 << 8) : swipeStep;    // 1 trix/lap for circle

    if (swipeType == SWIPE_STAR)
        randomizeStarAngle();    // fresh look each time star is used to close, same reasoning as the grow
    else
        markCircleFreshSequence();    // fresh sequence -- last shrink/grow's radius doesn't apply here

    maskNeeded = true;    // grow completing turned this off; shrinking needs it applied again
    setSwipe(x, y, radius, -step, SWIPE_SHRINK);
}

// Sets BOTH physical mask buffers, not just whichever one is currently
// maskShow/swipeWriteMask -- called at sequence start/finish, when the
// intent is "both agree on this value from here on", not a per-lap update,
// so there's no single "the buffer" to name.
void clearMask(int v) {
    unsigned char b = (unsigned char)v;
    unsigned int word = b | (b << 8) | (b << 16) | (b << 24);
    unsigned int *pA = (unsigned int *)swipeMaskA;
    unsigned int *pB = (unsigned int *)swipeMaskB;
    for (int i = 0; i < MASK_BUF_WORDS; i++) {
        pA[i] = word;
        pB[i] = word;
    }
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++) {
        ((unsigned char *)swipeMaskA)[i] = b;
        ((unsigned char *)swipeMaskB)[i] = b;
    }
}

void initStarSwipe() {
    // Despite the name, this is shape-agnostic setup (mask clear + flag) -- it used
    // to hardcode swipeType = SWIPE_STAR here too, which silently overrode whatever
    // setSwipeType() the caller had just set immediately before calling this.
    // Mask clears to 0 (fully hidden/black) and stays that way -- maskNeeded
    // true means applySwipeMask() keeps enforcing that -- until setSwipe() is
    // called for real with the actual grow parameters. swipeComplete = true
    // holds swipe() idle (its while loop won't run) in the meantime, so
    // nothing traces with default/leftover geometry before we're ready to
    // start: the caller no longer starts the grow here immediately, since
    // playerX/Y (and therefore the correct centre) aren't known yet at this
    // point -- see decodeCaves.c, which calls setSwipe() once the cave
    // decode has actually placed the player.
    clearMask(0);
    clearBorderCur();
    clearBorderPrev();
    maskNeeded = true;
    swipeComplete = true;
}

// Sets (GROW/etc) or clears (SHRINK) every mask bit for x in [x0,x1] on row
// y (clipped to the screen). A single lap's Bresenham circle only touches
// pixels at exactly the current radius -- a 1-pixel-wide ring -- so no
// matter how small the per-lap radius step is, there's always an annulus
// between one lap's radius and the next that a thin ring never reaches,
// leaving the mask mostly untouched. (Confirmed empirically: even at a
// 1px/lap step, most mask bytes never fully cleared across a full shrink.)
// Star's fill doesn't have this problem since its long diagonal line
// segments happen to sweep enough area per lap on their own; circle's does,
// so it needs an actual span fill instead of a point trace.
//
// One screen half (xx 0-19) always maps to exactly 3 groups (see rebase[]
// and the (xx+4)>>3 split): group 0 is xx 0-3, ONE bit each in the byte's
// upper nibble (PF0 -- real TIA hardware only implements 4 bits there, the
// lower nibble is never read by anything so it doesn't need preserving
// carefully, just left alone); group 1 is xx 4-11, all 8 bits, one each,
// reversed order (PF1); group 2 is xx 12-19, all 8 bits, one each, forward
// order (PF2). None of the bits within a group overlap between columns, so
// whenever a span fully covers one of these groups, the whole group can be
// set in ONE byte write instead of iterating each of its columns through
// rebase[] individually -- only a span's PARTIAL group (its two ragged
// ends) still needs the per-column loop. This matters here specifically
// because circle()'s SHRINK fills are two spans -- the parts OUTSIDE the
// current disk edge -- and near the poles the disk is narrow, so those
// outside spans are close to the FULL row width every one of those rows:
// up to 40 individual rebase-lookup writes each, for potentially ~15-20
// rows per lap, right when circleRadius (and so the per-lap row count) is
// largest -- exactly the situation most likely to blow a frame's idle-time
// budget and be the actual cause of the remaining flicker. Verified
// bit-for-bit identical to the original per-column loop across 20000
// randomised (x0, x1, phase, pre-existing byte contents) trials.
static void fillHalfSpan(int xa, int xb, int rowBase, bool setBits) {

    if (xa > xb)
        return;

    // Writes through swipeWriteMask, NOT the literal swipeMaskA/B -- see
    // swipeWriteMask's declaration.
    // Group 0: xx 0-3, upper nibble only.
    if (xa <= 3 && xb >= 0) {
        int lo = xa > 0 ? xa : 0;
        int hi = xb < 3 ? xb : 3;
        if (lo == 0 && hi == 3) {
            unsigned char b = ((unsigned char *)swipeWriteMask)[rowBase];
            ((unsigned char *)swipeWriteMask)[rowBase] = (b & 0x0F) | (setBits ? 0xF0 : 0x00);
        } else {

            if (setBits)
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeWriteMask)[rowBase] |= rebase[x];
            else
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeWriteMask)[rowBase] &= ~rebase[x];
        }
    }

    // Group 1: xx 4-11, full byte, reversed bit order.
    if (xa <= 11 && xb >= 4) {
        int lo = xa > 4 ? xa : 4;
        int hi = xb < 11 ? xb : 11;
        if (lo == 4 && hi == 11) {
            ((unsigned char *)swipeWriteMask)[rowBase + SCREEN_TRIX_Y] = setBits ? 0xFF : 0x00;
        } else {

            if (setBits)
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeWriteMask)[rowBase + SCREEN_TRIX_Y] |= rebase[x];
            else
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeWriteMask)[rowBase + SCREEN_TRIX_Y] &= ~rebase[x];
        }
    }

    // Group 2: xx 12-19, full byte, forward bit order.
    if (xa <= 19 && xb >= 12) {
        int lo = xa > 12 ? xa : 12;
        int hi = xb < 19 ? xb : 19;
        if (lo == 12 && hi == 19) {
            ((unsigned char *)swipeWriteMask)[rowBase + 2 * SCREEN_TRIX_Y] = setBits ? 0xFF : 0x00;
        } else {

            if (setBits)
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeWriteMask)[rowBase + 2 * SCREEN_TRIX_Y] |= rebase[x];
            else
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeWriteMask)[rowBase + 2 * SCREEN_TRIX_Y] &= ~rebase[x];
        }
    }
}

static void fillMaskSpan(int x0, int x1, int y) {

    if (y < 0 || y >= SCREEN_TRIX_Y)
        return;

    if (x0 < 0)
        x0 = 0;
    if (x1 > _1ROW - 1)
        x1 = _1ROW - 1;

    bool setBits = (swipePhase != SWIPE_SHRINK);

    // Left half: x 0-19, group rows start at y.
    int la = x0 > 0 ? x0 : 0;
    int lb = x1 < 19 ? x1 : 19;
    fillHalfSpan(la, lb, y, setBits);

    // Right half: x 20-39 (xx = x-20), group rows start at y + 3*SCREEN_TRIX_Y.
    int ra = x0 >= 20 ? x0 - 20 : 0;
    int rb = x1 < 39 ? x1 - 20 : 19;
    fillHalfSpan(ra, rb, y + 3 * SCREEN_TRIX_Y, setBits);
}

// Border-only: marks the visible ring outline, without touching the mask
// (that's fillMaskSpan()'s job for circle now -- see above).
//
// Marks 2 consecutive trix-rows per point (y and y+1), not 1 -- same thing
// bresenhamLine() does below for every pixel it plots for star, which is
// exactly why star's border reads as uniform thickness regardless of a
// segment's slope: trix rows are physically shorter than trix columns are
// wide (same 7/16 aspect ratio used everywhere else in this file), so a
// "tall pixel" is needed to make 1 unit of real vertical extent visually
// match 1 trix-column's real width. Doing this here, unconditionally, for
// every point circle() draws replaces an earlier attempt that only
// thickened rows near the poles where a connecting line happened to run
// shallow -- that band turned out too short/inconsistent; this fixes the
// root cause everywhere at once, the same way star already does it.
//
// Bounds check is tightened to SCREEN_TRIX_Y-1 (not just SCREEN_TRIX_Y) so
// the y+1 mark always stays within the same column's contiguous block --
// same reason bresenhamLine() uses the same tightened check.
bool drawBorderBit(int x, int y) {

    if (x < 0 || x >= _1ROW || y < 0 || y >= SCREEN_TRIX_Y - 1)
        return false;

    if (x >= 20) {
        x -= 20;
        y += 3 * SCREEN_TRIX_Y;
    }

    y += ((x + 4) >> 3) * SCREEN_TRIX_Y;

    // Writes into whichever physical buffer swipeWriteBorder currently
    // names, NOT a literal buffer name -- see its declaration. bresenhamLine()
    // (star's own border+mask writer) targets the same pointer independently.
    ((unsigned char *)swipeWriteBorder)[y] |= rebase[x];
    ((unsigned char *)swipeWriteBorder)[y + 1] |= rebase[x];

    return true;
}

void swipe(int reserved) {

    // Watchdog -- see swipeWatchdogFrames' declaration. Idle (swipeComplete
    // true, the normal state whenever nothing is actively growing/shrinking)
    // keeps it reset to 0 every frame; only an ACTIVE sequence that hasn't
    // finished after SWIPE_WATCHDOG_FRAMES gets force-closed. Doesn't do the
    // clearing itself -- just hands off to the SAME incremental finishPending
    // path a normal termination uses (see below), so a watchdog-triggered
    // finish can't overrun a frame any more than a normal one can.
    if (swipeComplete && !finishPending) {
        swipeWatchdogFrames = 0;
    } else if (!finishPending && ++swipeWatchdogFrames > SWIPE_WATCHDOG_FRAMES) {
        finishStage = FINISH_MASK_A;
        finishIndex = 0;
        finishPending = true;
        swipeComplete = false;    // not really done until the incremental finish below actually runs
    }

    // Incremental finish -- see finishStage's declaration. One word at a
    // time, re-checking T1TC before each one -- exactly the same budget
    // discipline circle()'s own row loop below uses, just applied to
    // clearing instead of row processing. Guarantees forward progress every
    // single frame, however little time is left, rather than needing some
    // fixed amount of slack to exist all at once.
    if (finishPending) {
        while (finishStage != FINISH_DONE && T1TC < availableIdleTime - reserved) {
            int tailBytes = MASK_BUF_BYTES - MASK_BUF_WORDS * 4;    // 0 today (396 divides evenly by
                                                                     // 4) -- kept generic, same as
                                                                     // clearBorderCur()/clearMask()
            switch (finishStage) {
            case FINISH_MASK_A: {
                unsigned char v = (swipePhase == SWIPE_GROW) ? 0xFF : 0;
                if (finishIndex < MASK_BUF_WORDS) {
                    unsigned int word = v | (v << 8) | (v << 16) | (v << 24);
                    ((unsigned int *)swipeMaskA)[finishIndex] = word;
                    finishIndex++;
                } else if (finishIndex - MASK_BUF_WORDS < tailBytes) {
                    ((unsigned char *)swipeMaskA)[MASK_BUF_WORDS * 4 + (finishIndex - MASK_BUF_WORDS)] = v;
                    finishIndex++;
                } else {
                    finishStage = FINISH_MASK_B;
                    finishIndex = 0;
                }
                break;
            }
            case FINISH_MASK_B: {
                unsigned char v = (swipePhase == SWIPE_GROW) ? 0xFF : 0;
                if (finishIndex < MASK_BUF_WORDS) {
                    unsigned int word = v | (v << 8) | (v << 16) | (v << 24);
                    ((unsigned int *)swipeMaskB)[finishIndex] = word;
                    finishIndex++;
                } else if (finishIndex - MASK_BUF_WORDS < tailBytes) {
                    ((unsigned char *)swipeMaskB)[MASK_BUF_WORDS * 4 + (finishIndex - MASK_BUF_WORDS)] = v;
                    finishIndex++;
                } else {
                    finishStage = FINISH_BORDER_CUR;
                    finishIndex = 0;
                }
                break;
            }
            case FINISH_BORDER_CUR: {
                if (finishIndex < MASK_BUF_WORDS) {
                    ((unsigned int *)borderMaskCur)[finishIndex] = 0;
                    finishIndex++;
                } else if (finishIndex - MASK_BUF_WORDS < tailBytes) {
                    ((unsigned char *)borderMaskCur)[MASK_BUF_WORDS * 4 + (finishIndex - MASK_BUF_WORDS)] = 0;
                    finishIndex++;
                } else {
                    // borderMaskPrev holds real (non-zero) data any time
                    // STAR's hold-copy or CIRCLE's write-target swap have
                    // touched it (see newLapPending's handling further down,
                    // and borderPrevIsZero's declaration) -- skip this whole
                    // stage instead of clearing an already-all-zero buffer
                    // only when neither of those has happened since the last
                    // clear.
                    finishStage = borderPrevIsZero ? FINISH_DONE : FINISH_BORDER_PREV;
                    finishIndex = 0;
                }
                break;
            }
            case FINISH_BORDER_PREV: {
                if (finishIndex < MASK_BUF_WORDS) {
                    ((unsigned int *)borderMaskPrev)[finishIndex] = 0;
                    finishIndex++;
                } else if (finishIndex - MASK_BUF_WORDS < tailBytes) {
                    ((unsigned char *)borderMaskPrev)[MASK_BUF_WORDS * 4 + (finishIndex - MASK_BUF_WORDS)] = 0;
                    finishIndex++;
                } else {
                    borderPrevIsZero = true;
                    finishStage = FINISH_DONE;
                }
                break;
            }

            default:
                break;
            }
        }

        if (finishStage == FINISH_DONE) {
            if (swipePhase == SWIPE_GROW)
                maskNeeded = false;
            finishPending = false;
            swipeComplete = true;
        }
        return;
    }

    // We do the circle processing at start of VB or OS

    // Start of a new lap -- see borderShowA/B's declaration for the full
    // reasoning (including STAR's now-abandoned hold-and-OR, and why swap
    // works for both types instead). swipeWriteBorder names whichever
    // physical buffer THIS lap's just-finished, fully-connected trace lives
    // in -- promote it to display (borderShowA = borderShowB = that buffer,
    // so the OR in applySwipeMask collapses to just it, never blended with
    // anything older) and hand the OTHER physical buffer (the one that WAS
    // on screen, no longer needed) to swipeWriteBorder as the upcoming
    // lap's write target, clearing it first since both drawBorderBit()
    // (circle) and bresenhamLine() (star) only ever OR bits in. Works
    // identically for a sequence's very first lap too, and for the first
    // lap after switching from one swipe type to the other: both physical
    // buffers are already all-zero at that point (guaranteed by the
    // previous sequence's finishPending clear, or by initStarSwipe() at
    // boot), so "promoting" swipeWriteBorder's leftover value just displays
    // blank until lap 1 actually finishes -- no separate first-lap or
    // type-switch case needed.
    //
    // The mask swaps at the exact same moment, for the exact same reason:
    // it was observed on hardware that the (previously single-buffered)
    // mask could visibly outrun the border during a multi-frame lap,
    // revealing pixels beyond where the border line had actually reached
    // yet. Doing both swaps in this one block guarantees mask and border
    // only ever change together -- there's no way to see one updated
    // without the other any more. Unlike the border, the mask can't just
    // start its write target blank: it's cumulative (a lap only touches the
    // thin band between old and new radius/size; everything else has to
    // carry over from every earlier lap), so its write target has to start
    // as a full copy of whatever's currently displayed, via copyBuf(), with
    // THIS lap's span-fills then applying on top of that copy.
    if (newLapPending) {
        unsigned char(*justFinishedBorder)[SCREEN_TRIX_Y] = swipeWriteBorder;
        unsigned char(*nextWriteBorder)[SCREEN_TRIX_Y] =
            (justFinishedBorder == borderMaskCur) ? borderMaskPrev : borderMaskCur;

        borderShowA = justFinishedBorder;
        borderShowB = justFinishedBorder;
        swipeWriteBorder = nextWriteBorder;

        // Pessimistically mark not-zero the instant we hand this buffer
        // over as a write target, rather than after the lap fills it in --
        // if this turns out to be the sequence's LAST lap, the
        // finishPending machine's borderPrevIsZero check (see its
        // declaration) runs right after, and it needs to already see "not
        // zero" for whatever real data this lap is about to trace, not the
        // split-second-stale "just cleared" state.
        if (nextWriteBorder == borderMaskPrev)
            borderPrevIsZero = false;

        clearBorderBuf(nextWriteBorder);

        unsigned char(*justFinishedMask)[SCREEN_TRIX_Y] = swipeWriteMask;
        unsigned char(*nextWriteMask)[SCREEN_TRIX_Y] = (justFinishedMask == swipeMaskA) ? swipeMaskB : swipeMaskA;

        maskShow = justFinishedMask;
        swipeWriteMask = nextWriteMask;
        copyBuf(nextWriteMask, justFinishedMask);

        newLapPending = false;
    }

    while (!swipeComplete && T1TC < availableIdleTime - reserved) {

        switch (swipeType) {
        case SWIPE_CIRCLE:
            swipeVisible |= circle();
            break;

        case SWIPE_STAR:
            swipeVisible |= star();
            break;
        }

        if (swipeComplete) {

            switch (swipePhase) {
            case SWIPE_SEARCH:
                if (swipeVisible) {
                    swipeStep = 4096;
                    swipePhase = SWIPE_REGROW;
                }
                break;

            case SWIPE_REGROW:
                if (!swipeVisible) {
                    swipeStep = -256;
                    swipePhase = SWIPE_RESHRINK;
                }
                break;

            case SWIPE_RESHRINK:
                if (swipeVisible)
                    swipePhase = SWIPE_SHRINK;
                break;

            case SWIPE_SHRINK:
                // NOT "!swipeVisible ||" any more. GROW legitimately needs a
                // visibility-based end condition -- it starts small and has
                // no natural "big enough" radius to check against, so it has
                // to keep growing until the border provably can't be seen any
                // more (see SWIPE_GROW below). SHRINK is different: it starts
                // at circleCoverRadius(), which is now sized to just barely
                // enclose the farthest screen corner -- meaning the RING
                // ITSELF is expected, correctly, to sit outside the visible
                // screen for however many of the earliest laps it takes to
                // shrink back down to where it actually crosses the screen
                // edge. Nothing should be visibly changing yet at that point
                // (the disk still fully covers everything), so an invisible
                // ring there isn't "finished", it's "hasn't started
                // mattering yet". !swipeVisible treated that as completion
                // and ended the sequence on lap 1, near-instantly, for any
                // centre where covering the farthest corner overshoots the
                // nearer edges by enough that the first lap's ring misses
                // the screen entirely -- confirmed by simulation against the
                // exact per-row circle() math: 108 of 2640 possible on-screen
                // centres hit this on the very first lap. SHRINK already has
                // an exact, unambiguous numeric end condition (radius
                // reaching ~0, checked below) that doesn't depend on
                // incidental on-screen visibility at all -- re-verified by
                // simulation that using ONLY that check, the same 2640
                // centres all complete in the expected number of laps, zero
                // early terminations, zero runaways.
                if (swipeRadius <= -swipeStep) {
                    // Needs both clearMask(0) (force fully hidden -- circle's
                    // thin per-lap outline is gap-prone, same reasoning as
                    // GROW's clearMask(255)) AND clearing borderMaskCur
                    // (applySwipeMask() forces a pixel fully ON wherever
                    // border is set, completely regardless of the mask, so
                    // the final lap's last-drawn ring would otherwise sit on
                    // screen forever). That's real, unavoidable work -- up to
                    // ~200 words -- so don't do it here unconditionally: hand
                    // off to the budget-checked finishPending path instead
                    // (see its declaration and use just above). Doing it here
                    // directly, with no time-check at all, right as the
                    // lap that just finished may have already spent the
                    // frame's whole budget getting here, is exactly what
                    // caused a CPU jam right as the shrink was about to end.
                    finishStage = FINISH_MASK_A;
                    finishIndex = 0;
                    finishPending = true;
                    swipeComplete = false;    // not really done until finishPending's clear actually runs
                    return;
                }
                break;

            case SWIPE_GROW:
                if (!swipeVisible) {
                    // Same reasoning as SWIPE_SHRINK above -- clearMask(255)
                    // is real work, deferred to finishPending instead of run
                    // unconditionally right here.
                    finishStage = FINISH_MASK_A;
                    finishIndex = 0;
                    finishPending = true;
                    swipeComplete = false;
                    return;
                }
                break;
            }

            setSwipe(swipeCenterX, swipeCenterY, swipeRadius, swipeStep, swipePhase);
            return;
        }
    }
}

// clang-format off

const short starSine[] = {
    0,
    19260,
    31163,
    31163,
    19260,
    0,
    -19260,
    -31163,
    -31163,
    -19260
};

// Unscaled cosine -- same angles/scale as starCosine[], but WITHOUT its
// baked-in 7/16 aspect correction, so it can be rotated in true (isotropic)
// space first and have the aspect squeeze applied afterwards, only to the
// final x. Rotating the already-squeezed starCosine[] directly would rotate
// an ellipse instead of a circle, distorting the star's shape at any angle
// that isn't a multiple of 90 degrees.
const short starTrueCosine[] = {
    32767,
    26509,
    10126,
    -10126,
    -26509,
    -32767,
    -26509,
    -10126,
    10126,
    26509
};

const short starCosine[] = {
    (32767*7)>>4,
    (26509*7)>>4,
    (10126*7)>>4,
    -(10126*7)>>4,
    -(26509*7)>>4,
    -(32767*7)>>4,
    -(26509*7)>>4,
    -(10126*7)>>4,
    (10126*7)>>4,
    (26509*7)>>4
};

// clang-format on

// Re-rolls the star's one-time rotation. Call this ONCE per fresh star
// sequence (grow start, or shrink start if the shrink is a star) -- NOT per
// lap and NOT from inside setSwipe()'s own per-lap re-arm (see swipe()'s
// tail call), since that would rotate mid-sequence and break the close
// overlap between consecutive laps that the fill depends on. A 5-pointed
// star has 5-fold (72 degree) rotational symmetry, so a plain index-shift
// through starCosine/starSine would only ever reproduce the same look;
// picking an arbitrary angle from sin_cos[32] (11.25 degree steps) and
// rotating properly (see generateStar()) actually varies the look.
void randomizeStarAngle() {
    int r = rangeRandom(32) & 0x1F;
    starRotSin = sin_cos[r];
    starRotCos = sin_cos[(r + 8) & 0x1F];
    starGrowLapCount = 0;    // fresh sequence -- see starGrowLapCount's declaration
}

// Plain C >> on a negative int is an arithmetic (floor) shift -- rounds
// toward NEGATIVE INFINITY, not toward zero. Fine for magnitude-only values
// (isqrt()'s results, e.g.), but wrong for a genuinely signed quantity:
// floor-rounding means every value computed from a negative intermediate
// ends up pushed slightly FURTHER from zero than the same magnitude would
// from a positive one. generateStar()'s rc0/rs0/rc1/rs1 (and the vertex
// coordinates built from them) are exactly that -- signed, sign varying
// with rotation -- so plain >> was biasing every vertex derived from a
// negative term outward, consistently toward negative x/y regardless of
// the star's rotation or size. Confirmed by simulation: the star's true
// centroid (which should sit exactly on swipeCenterX/Y -- 5-fold
// rotational symmetry cancels any real offset) averaged roughly 0.3-0.7
// trix off toward negative x/y with plain >>; switching to this
// truncate-toward-zero version (same negate/shift/negate trick as abs()
// below -- still no division) brought the average centroid to exactly
// (0,0) in the same simulation.
static int symShr(int v, int n) {
    return (v >= 0) ? (v >> n) : -((-v) >> n);
}

void generateStar() {

    int size = swipeRadius >> 8;
    int innerSize = (size * 7) >> 4;

    for (int i = 0; i < 10; i += 2) {

        // Rotate the true (unsquashed) unit vector for each vertex by the
        // fixed one-time angle before applying the 7/16 x-squeeze and the
        // radius scale -- see starTrueCosine's comment for why the order
        // matters. starTrueCosine/starSine are Q15, starRotSin/Cos are Q8
        // (straight from sin_cos[]), so the product is Q23 -- >>8 brings it
        // back to Q15, matching the un-rotated values these replace. Uses
        // symShr(), not plain >>, since these are signed -- see its comment.
        int rc0 = symShr(starTrueCosine[i] * starRotCos - starSine[i] * starRotSin, 8);
        int rs0 = symShr(starTrueCosine[i] * starRotSin + starSine[i] * starRotCos, 8);
        int rc1 = symShr(starTrueCosine[i + 1] * starRotCos - starSine[i + 1] * starRotSin, 8);
        int rs1 = symShr(starTrueCosine[i + 1] * starRotSin + starSine[i + 1] * starRotCos, 8);

        starX[i] = swipeCenterX + symShr(size * symShr(rc0 * 7, 4), 16);
        starY[i] = swipeCenterY + symShr(size * rs0, 16);
        starX[i + 1] = swipeCenterX + symShr(innerSize * symShr(rc1 * 7, 4), 16);
        starY[i + 1] = swipeCenterY + symShr(innerSize * rs1, 16);
    }
}

int abs(int value) {
    return value >= 0 ? value : -value;
}

// Integer square root, floor(sqrt(n)) for n>=0 -- the standard "digit by
// digit" method: shifts, adds, subtracts and compares only. No division
// (this coprocessor has none, confirmed) and no float. Verified against
// Python's exact math.isqrt() for every n in 0..13107 (circleCoverRadius()'s
// combined dy2+rForX^2 term tops out at 12608 across every on-screen centre
// -- comfortably inside the 1<<14 initial bit guess below) -- exact match
// throughout.
static int isqrt(int n) {
    if (n <= 0)
        return 0;
    int res = 0;
    int bit = 1 << 14;    // largest power of 4 comfortably above any r^2 we'll see
    while (bit > n)
        bit >>= 2;
    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

// The circle rendered very slightly taller than wide -- this trims the
// vertical reach by ~5% to compensate, leaving the horizontal reach (which
// only ever depends on dy==0, unaffected by this term) untouched. Scales
// dy^2 by 71/64 (~1.109) before it's subtracted from radius^2 in circle()'s
// rem calculation: a bigger dy^2 term makes rem hit zero at a smaller |dy|,
// i.e. the disk's pole is reached sooner going up/down -- shrinking the
// vertical reach to radius*sqrt(64/71) =~ 0.949x, about 5% shorter. Done
// via shifts+add/subtract on the ALREADY-computed dy*dy (dy2 + dy2>>3 -
// dy2>>6 = dy2*71/64) rather than an extra multiply, so this costs nothing
// beyond what dy*dy already cost. circleCoverRadius()'s own dy reach is
// scaled up to compensate (see there) so the disk still fully covers the
// screen at the start of a shrink.
static int squashDy2(int dy) {
    int dy2 = dy * dy;
    return dy2 + (dy2 >> 3) - (dy2 >> 6);
}

// Border-only Bresenham line: marks swipeWriteBorder (via drawBorderBit())
// for every pixel from (x0,y0) EXCLUSIVE to (x1,y1) inclusive, without touching
// the mask (that's fillMaskSpan()'s job elsewhere). Used to connect one
// row's edge point to the next's -- see circle()'s comment for why an
// isolated dot per row can't form a connected curve on its own.
//
// Deliberately does NOT draw the (x0,y0) start point: every call site
// chains from the previous call's endpoint (or the chain's initial seed
// point, drawn once directly via drawBorderBit() -- see circle()), so
// (x0,y0) here was always already drawn by whatever came before. Drawing
// it again is a pure no-op (drawBorderBit() just re-ORs the same bits) but
// still costs 2 read-modify-write byte ops for nothing -- over ~100 rows a
// lap, both edges, that's a real amount of wasted work for a stray pixel
// that was never going anywhere. NOT the same as skipping the LAST point
// instead: that would drop the true final endpoint of a chain (a pole
// tip, or wherever the trace ends) since there's no later call to redraw
// it -- skipping the *first* point is safe unconditionally because
// there's always an earlier draw that already covered it.
//
// No early-exit optimisation like bresenhamLine() has, since these are
// always short, adjacent-row lines, not the far-off-screen star vertices
// that needed one.
bool bresenhamBorderLine(int x0, int y0, int x1, int y1) {

    bool visible = false;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (x0 != x1 || y0 != y1) {
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
        visible |= drawBorderBit(x0, y0);
    }

    return visible;
}

// One PAIR of horizontal rows of the circle at the current lap's radius
// (circleRadius) -- the row at +dy and the row at -dy -- computed directly
// off a single isqrt() rather than accumulated from Bresenham point-
// stepping. Per |dy|, the disk's half-width is sqrt(radius^2 - dy^2)
// (aspect-scaled by 7/16 same as everywhere else in this file); +dy and
// -dy always share that same half-width by construction, so there's no
// need to compute it twice -- circleRow now runs 0 .. sweep (not -sweep ..
// sweep) and each call handles both rows.
//
// Border: each row's left/right edge is connected to the PREVIOUS row's
// (in its own half's outward direction) via an actual Bresenham line
// (bresenhamBorderLine()), not left as an isolated dot -- a single pixel
// per row can't form a connected curve where the disk's edge is nearly
// flat (near the top/bottom), since consecutive rows' edges can be several
// trix apart there; a line between them closes that gap the same way
// star's segments connect its vertices. Since top and bottom now trace
// outward from the centre row instead of one continuous top-to-bottom
// sweep, they're two independent chains that share their starting point at
// dy==0 -- top/botPrevLeft/Right/Row and top/botHasPrev track each
// separately. (Bresenham's exact intermediate pixel choice isn't fully
// symmetric under reversal, so a handful of corner pixels land 1 trix-row
// off from the old single-sweep version -- checked by simulation: differs
// by ~2.5% of border points on average, never changes lap-to-lap stability
// characteristics, and the tall-pixel drawBorderBit() thickness already
// swallows a 1-row difference. Left and right edges independently converge
// to the same point at the very top/bottom row (half-width -> 0 there), so
// tracing both edges this way closes the loop at the poles on its own --
// no separate cap case needed.
//
// Every row commits unconditionally (no "skip a lone single-row step"
// smoothing) -- that was tried and reverted: which rows count as a lone
// single-row step depends on the exact radius, so the set of skipped rows
// near the poles differs lap to lap as the radius shrinks by 1 -- reshaping
// the whole pole region every lap instead of just trimming a stray pixel,
// which read as flickering (confirmed by simulation, worse at large radius
// where there are more single-row candidates to disagree about). Committing
// every row is stable lap to lap even though the poles are a touch more
// jagged, which matters far more than the fine detail there.
//
// Sweep range: bounded to circleRadius (+one step's margin for SHRINK, see
// setSwipe()), NOT the whole original starting extent -- sweeping the full
// extent every lap regardless of how far the disk had shrunk cost too much
// to fit in one frame's time budget most laps, which visibly spilled the
// redraw across several frames (a slow top-to-bottom sweep instead of the
// disk shrinking). The margin still guarantees any row that just fell
// outside the shrinking disk gets explicitly, fully hidden the same lap
// that happens, rather than freezing at its last partial reveal until the
// final blanket clearMask(0) -- same correctness, bounded cost.
//
// Mask fill: only the band that changed since LAST lap gets touched --
// same idea as the per-row edge cache that was tried and reverted for its
// RAM cost, but derived arithmetically instead of stored: SHRINK/GROW's
// radius changes by exactly `step` every lap (constant for a whole
// sequence), so last lap's radius is just circleRadius +/- step,
// recomputed here with one extra isqrt() per row rather than any per-row
// storage. circleHasOldRadius is false only for a sequence's first lap
// (see its declaration), when there's nothing to derive an old edge from
// -- that one lap still does the full-span fill. Verified against the
// always-full-refill behaviour by simulation across many multi-lap
// sequences (both directions) before trusting it here.
bool circle() {

    bool visible = false;

    int dy = circleRow;
    int topRowY = swipeCenterY - dy;
    int botRowY = swipeCenterY + dy;

    int dy2 = squashDy2(dy);
    int rem = circleRadiusSq - dy2;

    // Last lap's radius-squared, if any -- both circleRadiusSq and
    // oldRadiusSq are computed once per lap in setSwipe() now (they're
    // constant for the whole lap), not re-multiplied here on every single
    // row -- see their declarations.
    bool haveOld = false;
    int oldLeft = 0, oldRight = 0;
    if (circleHasOldRadius) {
        int oldRem = oldRadiusSq - dy2;
        if (oldRem >= 0) {
            int oldW = isqrt(oldRem);
            int oldHalfWidth = (oldW * 7) >> 4;
            oldLeft = swipeCenterX - oldHalfWidth;
            oldRight = swipeCenterX + oldHalfWidth;
            haveOld = true;
        }
    }

    if (rem < 0) {
        // Outside the current (shrunk) disk entirely -- SHRINK must fully
        // hide it now (see the sweep margin in setSwipe(), which
        // guarantees this row is visited exactly once, the lap it
        // transitions out -- GROW never sweeps beyond its own current
        // radius, so this branch can't be hit for GROW). No edge here, so
        // there's nothing to connect the next valid row's point to either.
        // Only re-hides whatever was still revealed last lap (oldLeft..
        // oldRight), not the whole row.
        if (swipePhase == SWIPE_SHRINK) {
            if (haveOld) {
                fillMaskSpan(oldLeft, oldRight, topRowY);
                if (dy != 0)
                    fillMaskSpan(oldLeft, oldRight, botRowY);
            } else {
                fillMaskSpan(0, _1ROW - 1, topRowY);
                if (dy != 0)
                    fillMaskSpan(0, _1ROW - 1, botRowY);
            }
        }
        topHasPrev = false;
        botHasPrev = false;
    } else {

        int w = isqrt(rem);              // circleX-equivalent half-width, pre-aspect-scale
        int halfWidth = (w * 7) >> 4;    // aspect-scaled to trix, same 7/16 rule used everywhere else

        int left = swipeCenterX - halfWidth;
        int right = swipeCenterX + halfWidth;

        if (swipePhase == SWIPE_SHRINK) {
            if (haveOld) {
                fillMaskSpan(oldLeft, left - 1, topRowY);
                fillMaskSpan(right + 1, oldRight, topRowY);
                if (dy != 0) {
                    fillMaskSpan(oldLeft, left - 1, botRowY);
                    fillMaskSpan(right + 1, oldRight, botRowY);
                }
            } else {
                fillMaskSpan(0, left - 1, topRowY);
                fillMaskSpan(right + 1, _1ROW - 1, topRowY);
                if (dy != 0) {
                    fillMaskSpan(0, left - 1, botRowY);
                    fillMaskSpan(right + 1, _1ROW - 1, botRowY);
                }
            }
        } else {
            if (haveOld) {
                fillMaskSpan(left, oldLeft - 1, topRowY);
                fillMaskSpan(oldRight + 1, right, topRowY);
                if (dy != 0) {
                    fillMaskSpan(left, oldLeft - 1, botRowY);
                    fillMaskSpan(oldRight + 1, right, botRowY);
                }
            } else {
                fillMaskSpan(left, right, topRowY);
                if (dy != 0)
                    fillMaskSpan(left, right, botRowY);
            }
        }

        if (dy == 0) {
            // Shared starting point for both the top-going and bottom-going
            // chains -- nothing to connect to yet either way.
            visible |= drawBorderBit(left, topRowY);
            visible |= drawBorderBit(right, topRowY);
            topPrevLeft = botPrevLeft = left;
            topPrevRight = botPrevRight = right;
            topPrevRow = botPrevRow = topRowY;
            topHasPrev = botHasPrev = true;
        } else {
            // Uniform thickness comes from drawBorderBit() itself now
            // (marks 2 trix-rows per point, same as star) -- no per-segment
            // slope detection or touch-up needed here any more.
            if (topHasPrev) {
                visible |= bresenhamBorderLine(topPrevLeft, topPrevRow, left, topRowY);
                visible |= bresenhamBorderLine(topPrevRight, topPrevRow, right, topRowY);
            } else {
                visible |= drawBorderBit(left, topRowY);
                visible |= drawBorderBit(right, topRowY);
            }
            topPrevLeft = left;
            topPrevRight = right;
            topPrevRow = topRowY;
            topHasPrev = true;

            if (botHasPrev) {
                visible |= bresenhamBorderLine(botPrevLeft, botPrevRow, left, botRowY);
                visible |= bresenhamBorderLine(botPrevRight, botPrevRow, right, botRowY);
            } else {
                visible |= drawBorderBit(left, botRowY);
                visible |= drawBorderBit(right, botRowY);
            }
            botPrevLeft = left;
            botPrevRight = right;
            botPrevRow = botRowY;
            botHasPrev = true;
        }

        // Nothing ever connects left and right HORIZONTALLY on the same row --
        // only the vertical row-to-row chains (bresenhamBorderLine above) do.
        // Doesn't matter at wide (equatorial) rows, where the gap between them
        // is most of the screen and clearly "inside" the shape. At the true
        // TIP of the disk though -- the last row this lap has any width at
        // all, where there's no next row for the vertical chains to connect
        // to -- left and right stop only 1-2 trix apart with nothing marking
        // the columns between: two isolated dots with a real gap, right where
        // the shape should round off.
        //
        // First attempt gated this on "right - left is small" instead of
        // "this is actually the last row" -- wrong condition: for a small
        // enough disk, EVERY row in the lap satisfies "small gap", not just
        // the tip, so it filled in every narrow row's span, each one already
        // 2 trix-rows tall via drawBorderBit(), all just 1 dy apart and so
        // overlapping vertically -- the whole tiny disk turned into one solid
        // chunky blob (visibly 4+ trix-rows tall, not the correct 2) instead
        // of a thin ring, and the intended one-row cap. Checking the NEXT
        // row's rem instead (one extra cheap squashDy2() call, no isqrt
        // needed since only its sign matters) correctly finds just the
        // single tip row per chain, so only that one row gets the extra
        // horizontal fill -- every other narrow-but-not-last row is left
        // exactly as the vertical chains alone draw it, same as before this
        // whole gap-fix existed.
        if (circleRadiusSq - squashDy2(dy + 1) < 0) {
            for (int x = left + 1; x < right; x++)
                visible |= drawBorderBit(x, topRowY);
            if (dy != 0)
                for (int x = left + 1; x < right; x++)
                    visible |= drawBorderBit(x, botRowY);
        }
    }

    circleRow++;

    int sweep = circleRadius;
    if (swipePhase == SWIPE_SHRINK)
        sweep += abs(swipeStep >> 8);
    if (circleRow > sweep)
        swipeComplete = true;

    return visible;
}

bool bresenhamLine(int x0, int y0, int x1, int y1) {

    if (x0 <= x1) {
        if (x0 >= _1ROW || x1 < 0)
            return false;
    } else {
        if (x1 >= _1ROW || x0 < 0)
            return false;
    }

    if (y0 <= y1) {
        if (y0 >= SCREEN_TRIX_Y || y1 < 0)
            return false;
    } else {
        if (y1 >= SCREEN_TRIX_Y || y0 < 0)
            return false;
    }

    bool visible = false;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int e2;

    while (true) {

        if (x0 >= 0 && x0 < _1ROW && y0 >= 0 && y0 < SCREEN_TRIX_Y - 1) {

            int x = x0;
            int y = y0;

            if (x >= 20) {
                x -= 20;
                y += 3 * SCREEN_TRIX_Y;
            }

            y += ((x + 4) >> 3) * SCREEN_TRIX_Y;

            // Writes through swipeWriteMask, NOT the literal swipeMaskA/B --
            // see swipeWriteMask's declaration.
            if (swipePhase == SWIPE_SHRINK) {
                ((unsigned char *)swipeWriteMask)[y] &= ~rebase[x];
                ((unsigned char *)swipeWriteMask)[y + 1] &= ~rebase[x];
            } else {
                ((unsigned char *)swipeWriteMask)[y] |= rebase[x];
                ((unsigned char *)swipeWriteMask)[y + 1] |= rebase[x];
            }

            // Border ring -- see drawBorderBit() for why this isn't
            // phase-gated, and swipeWriteBorder's declaration for why this
            // isn't the literal borderMaskCur any more.
            ((unsigned char *)swipeWriteBorder)[y] |= rebase[x];
            ((unsigned char *)swipeWriteBorder)[y + 1] |= rebase[x];

            visible = true;
        }

        else if (visible)
            return true;

        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }

    return visible;
}

bool star() {

    static int starPoint = 0;

    int starNextPoint = starPoint + 1;
    if (starNextPoint > 9) {
        starNextPoint = 0;
        swipeComplete = true;
    }

    // Segments always connect an outer vertex (even index) to an inner one
    // (odd index), and the inner vertex is provably closer to swipeCenterX/Y
    // than the outer one (innerSize < size always) -- so it's more likely to
    // still be on/near screen once the star's grown past screen-covering
    // size. bresenhamLine() only short-circuits AFTER a point's been visible
    // (see its "else if (visible) return true"), so walking inner->outer
    // puts any off-screen slack at the tail end where that catches it,
    // rather than at the head where nothing does. Bresenham draws the same
    // pixels either direction, so this doesn't change the shape -- unlike
    // clamping vertex coordinates, which was tried and distorted the star
    // into looking like a pentagon once points got clamped to a box edge.
    bool visible;
    if (starPoint % 2 == 0)    // starPoint is outer, starNextPoint is inner -- walk inner -> outer
        visible = bresenhamLine(starX[starNextPoint], starY[starNextPoint], starX[starPoint], starY[starPoint]);
    else    // starPoint is inner already
        visible = bresenhamLine(starX[starPoint], starY[starPoint], starX[starNextPoint], starY[starNextPoint]);

    starPoint = starNextPoint;
    return visible;
}

#endif
