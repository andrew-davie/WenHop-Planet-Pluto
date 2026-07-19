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

// aligned(4) on all three buffers below: clearBorderCur()/clearBorderPrev()/
// clearMask()/the star hold-copy in swipe() all bulk-access these 4 bytes
// at a time via an unsigned int* cast for speed. ARMv4T doesn't handle
// unaligned word access safely, and a plain "unsigned char[]" isn't
// guaranteed 4-byte aligned by the compiler on its own -- this attribute
// makes that guarantee explicit instead of relying on it happening to work.
static unsigned char swipeMask[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));

// Border ring: NOT permanent like swipeMask. borderMaskCur is this lap's
// trace (cleared at the start of each new lap); borderMaskPrev is a copy of
// whatever borderMaskCur held at the end of the PREVIOUS lap. applySwipeMask
// renders the OR of both, so the ring is always the last 2 laps' worth,
// continuously sliding -- rather than periodically clearing to nothing and
// redrawing, which reads as on/off blinking (especially early in a grow,
// where generateStar()'s fixed-point rounding means several consecutive
// laps can trace almost the exact same pixels -- a pure blink with no
// visible motion to mask it, vs. later laps where the shape has moved
// enough that any gap reads as motion rather than a flash).
static unsigned char borderMaskCur[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char borderMaskPrev[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));

static void fillMaskSpan(int x0, int x1, int y);
bool drawBorderBit(int x, int y);
bool bresenhamBorderLine(int x0, int y0, int x1, int y1);
void generateStar();
bool star();
bool circle();
void initStar(int x, int y);

static short starX[10];
static short starY[10];

static int swipeRadius;    // 24.8
static int swipeStep;      // 24.8

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
// Per pixel: if the border (borderMaskCur | borderMaskPrev) is set, force
// the bit fully ON (that's the "all
// pixels set" ring at the current edge of the swipe, same in both colour
// modes). Otherwise, if swipeMask (revealed) is set, leave the real content
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
            unsigned int r0 = swipeMask[col][y];
            unsigned int r1 = swipeMask[col][y + 1];
            unsigned int r2 = swipeMask[col][y + 2];
            unsigned int r3 = swipeMask[col][y + 3];
            unsigned int b0 = borderMaskCur[col][y] | borderMaskPrev[col][y];
            unsigned int b1 = borderMaskCur[col][y + 1] | borderMaskPrev[col][y + 1];
            unsigned int b2 = borderMaskCur[col][y + 2] | borderMaskPrev[col][y + 2];
            unsigned int b3 = borderMaskCur[col][y + 3] | borderMaskPrev[col][y + 3];
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
            unsigned int r0 = swipeMask[col][y];
            unsigned int r1 = swipeMask[col][y + 1];
            unsigned int b0 = borderMaskCur[col][y] | borderMaskPrev[col][y];
            unsigned int b1 = borderMaskCur[col][y + 1] | borderMaskPrev[col][y + 1];
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
            unsigned int r0 = swipeMask[col][y];
            unsigned int r1 = swipeMask[col][y + 1];
            unsigned int r2 = swipeMask[col][y + 2];
            unsigned int r3 = swipeMask[col][y + 3];
            unsigned int b0 = borderMaskCur[col][y] | borderMaskPrev[col][y];
            unsigned int b1 = borderMaskCur[col][y + 1] | borderMaskPrev[col][y + 1];
            unsigned int b2 = borderMaskCur[col][y + 2] | borderMaskPrev[col][y + 2];
            unsigned int b3 = borderMaskCur[col][y + 3] | borderMaskPrev[col][y + 3];
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
            unsigned int r0 = swipeMask[col][y];
            unsigned int r1 = swipeMask[col][y + 1];
            unsigned int b0 = borderMaskCur[col][y] | borderMaskPrev[col][y];
            unsigned int b1 = borderMaskCur[col][y + 1] | borderMaskPrev[col][y + 1];
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

// Gate the lap-transition handling on "a new lap started", not "a new frame
// happened", so this stays correct even if a lap ever does span multiple
// frames (we don't want to disturb borderMaskCur mid-trace).
static bool newLapPending;

void setSwipe(int x, int y, int radius, int step, enum CIRCLEPHASE phase) {

    swipeCenterX = x;
    swipeCenterY = y;

    swipeRadius = radius + step;    // 24.8
    swipeStep = step;               // 24.8

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

// Radius (in whole trix, not 24.8) a circle centred at swipeCenterX/Y needs
// to fully cover the screen. NOT a plain Euclidean corner distance: circle()
// scales its x-extent by 7/16 relative to the true (isotropic) half-width
// (halfWidth = (w*7)>>4) as a display aspect-ratio correction -- trix
// columns are physically wider than trix rows are tall,
// so the x-reach needs compressing for the shape to actually look round.
// That means the radius needed to cover the x-extent is dx*16/7, not just
// dx -- confirmed empirically (simulation): using the plain Euclidean
// distance here left the shape's x-reach several trix short at the sides,
// so a whole PF plane near the centre column never got touched by the
// shrink no matter how many laps ran.
static int circleCoverRadius() {
    int dx = swipeCenterX > (_1ROW - 1 - swipeCenterX) ? swipeCenterX : (_1ROW - 1 - swipeCenterX);
    int dy = swipeCenterY > (SCREEN_TRIX_Y - 1 - swipeCenterY) ? swipeCenterY : (SCREEN_TRIX_Y - 1 - swipeCenterY);
    int rForX = ((dx * 16 + 6) * (0x10000 / 7)) >> 16;    // ceil(dx*16/7)
    // Compensate for squashDy2()'s ~5% vertical trim in circle(): the disk's
    // actual dy-reach for a given radius is only radius*sqrt(64/71) now, so
    // the radius needed to cover a given dy is dy/sqrt(64/71) =~ dy*1.053.
    // dy + dy>>4 (dy*17/16 = 1.0625x) rounds that up a little further, which
    // is fine -- this only runs once per sequence (not the per-row path
    // squashDy2() itself is the cheap-on-purpose one for), and erring
    // towards slightly MORE coverage is the safe direction; the existing
    // "+2" margin below absorbs the rest of any rounding regardless.
    int rForY = dy + (dy >> 4);
    int r = (rForY > rForX) ? rForY : rForX;
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
    int radius = (swipeType == SWIPE_CIRCLE) ? (circleCoverRadius() << 8) : swipeRadius;
    int step = (swipeType == SWIPE_CIRCLE) ? (1 << 8) : swipeStep;    // 1 trix/lap for circle

    if (swipeType == SWIPE_STAR)
        randomizeStarAngle();    // fresh look each time star is used to close, same reasoning as the grow
    else
        markCircleFreshSequence();    // fresh sequence -- last shrink/grow's radius doesn't apply here

    maskNeeded = true;    // grow completing turned this off; shrinking needs it applied again
    setSwipe(x, y, radius, -step, SWIPE_SHRINK);
}

void clearMask(int v) {
    unsigned char b = (unsigned char)v;
    unsigned int word = b | (b << 8) | (b << 16) | (b << 24);
    unsigned int *p = (unsigned int *)swipeMask;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = word;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)swipeMask)[i] = b;
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

    // Group 0: xx 0-3, upper nibble only.
    if (xa <= 3 && xb >= 0) {
        int lo = xa > 0 ? xa : 0;
        int hi = xb < 3 ? xb : 3;
        if (lo == 0 && hi == 3) {
            unsigned char b = ((unsigned char *)swipeMask)[rowBase];
            ((unsigned char *)swipeMask)[rowBase] = (b & 0x0F) | (setBits ? 0xF0 : 0x00);
        } else {

            if (setBits)
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeMask)[rowBase] |= rebase[x];
            else
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeMask)[rowBase] &= ~rebase[x];
        }
    }

    // Group 1: xx 4-11, full byte, reversed bit order.
    if (xa <= 11 && xb >= 4) {
        int lo = xa > 4 ? xa : 4;
        int hi = xb < 11 ? xb : 11;
        if (lo == 4 && hi == 11) {
            ((unsigned char *)swipeMask)[rowBase + SCREEN_TRIX_Y] = setBits ? 0xFF : 0x00;
        } else {

            if (setBits)
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeMask)[rowBase + SCREEN_TRIX_Y] |= rebase[x];
            else
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeMask)[rowBase + SCREEN_TRIX_Y] &= ~rebase[x];
        }
    }

    // Group 2: xx 12-19, full byte, forward bit order.
    if (xa <= 19 && xb >= 12) {
        int lo = xa > 12 ? xa : 12;
        int hi = xb < 19 ? xb : 19;
        if (lo == 12 && hi == 19) {
            ((unsigned char *)swipeMask)[rowBase + 2 * SCREEN_TRIX_Y] = setBits ? 0xFF : 0x00;
        } else {

            if (setBits)
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeMask)[rowBase + 2 * SCREEN_TRIX_Y] |= rebase[x];
            else
                for (int x = lo; x <= hi; x++)
                    ((unsigned char *)swipeMask)[rowBase + 2 * SCREEN_TRIX_Y] &= ~rebase[x];
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

// Border-only: marks the visible ring outline, without touching swipeMask
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

    ((unsigned char *)borderMaskCur)[y] |= rebase[x];
    ((unsigned char *)borderMaskCur)[y + 1] |= rebase[x];

    return true;
}

void swipe(int reserved) {

    // We do the circle processing at start of VB or OS

    // Start of a new lap. For STAR: the ring that was just active
    // (borderMaskCur, as left by the lap that just finished) becomes
    // borderMaskPrev, and applySwipeMask renders the OR of both, so every
    // lit border pixel persists for at least 2 consecutive frames instead
    // of exactly 1 -- needed there because star's border is a handful of
    // long, coarsely-stepped (3 trix/lap) segments; without holding, a
    // pixel lit for only a single frame reads as a flash.
    //
    // Circle doesn't hold at all any more (it used to, and that's what
    // caused the "ghost ring" bug: for SHRINK, the held previous lap's
    // ring was drawn at a LARGER radius than the current one, so it sat
    // entirely OUTSIDE the current disk -- in territory the mask had
    // already hidden -- and the border-forces-on rule in applySwipeMask()
    // overrode that, showing a stale ring floating in what should've been
    // blank space for one extra frame every lap). Turns out circle doesn't
    // need the workaround in the first place: each lap already traces a
    // fully CONNECTED outline (bresenhamBorderLine() row-to-row, not
    // isolated points), each point is already 2 trix-rows tall
    // (drawBorderBit()), and the per-lap step is a fine 1 trix -- so
    // consecutive frames' independently-drawn outlines are already close
    // enough, and dense enough, to put plenty of the same pixels on screen
    // frame after frame without needing to explicitly carry data over.
    // Star's border (coarse, isolated segment endpoints) doesn't have that
    // property, so it still needs the hold.
    if (newLapPending) {
        if (swipeType == SWIPE_STAR) {
            unsigned int *pPrev = (unsigned int *)borderMaskPrev;
            unsigned int *pCur = (unsigned int *)borderMaskCur;
            for (int i = 0; i < MASK_BUF_WORDS; i++)
                pPrev[i] = pCur[i];
            for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
                ((unsigned char *)borderMaskPrev)[i] = ((unsigned char *)borderMaskCur)[i];
            borderPrevIsZero = false;    // now holds real border data (or did last we touched it)
        } else if (!borderPrevIsZero) {
            // Circle never writes to borderMaskPrev at all (see above), so
            // once it's zero for a circle sequence it STAYS zero every lap
            // after the first -- re-clearing an already-all-zero buffer
            // every single lap was pure waste. Only the first circle lap
            // after switching away from star (when prev still holds star's
            // last-held ring) actually needs this.
            clearBorderPrev();
        }
        clearBorderCur();
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
                if (!swipeVisible || swipeRadius <= -swipeStep) {
                    // Mirror of GROW's clearMask(255) below: the accumulated
                    // AND-clearing from each lap's thin outline is gap-prone
                    // (same "successive outlines must overlap closely"
                    // constraint as the fill side), and unlike GROW this had
                    // no blanket fallback -- so whatever the outlines missed
                    // stayed visible forever, which is what made circle (a
                    // much thinner per-lap sweep than star's ten long
                    // diagonal segments, so proportionally far gappier) look
                    // like it wasn't masking at all. Force fully hidden
                    // regardless of how completely the traces covered it.
                    //
                    // Also clear the border buffers here, not just the mask:
                    // applySwipeMask() forces a pixel fully ON whenever
                    // borderMaskCur|borderMaskPrev has that bit set, completely
                    // regardless of swipeMask -- so whatever the final lap's
                    // ring last drew (a small dot near the pole, since the
                    // shrink was almost closed) would otherwise sit on screen
                    // forever: swipeComplete never gets reset after this
                    // early return, so swipe() does nothing on every later
                    // frame and nothing else ever clears it. That's the "shrinks
                    // to ~2 trix and stops" bug -- the mask genuinely finished,
                    // but a stray border pixel was stuck on top of it.
                    clearMask(0);
                    clearBorderCur();
                    clearBorderPrev();
                    return;
                }
                break;

            case SWIPE_GROW:
                if (!swipeVisible) {
                    clearMask(255);
                    maskNeeded = false;
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
        // back to Q15, matching the un-rotated values these replace.
        int rc0 = (starTrueCosine[i] * starRotCos - starSine[i] * starRotSin) >> 8;
        int rs0 = (starTrueCosine[i] * starRotSin + starSine[i] * starRotCos) >> 8;
        int rc1 = (starTrueCosine[i + 1] * starRotCos - starSine[i + 1] * starRotSin) >> 8;
        int rs1 = (starTrueCosine[i + 1] * starRotSin + starSine[i + 1] * starRotCos) >> 8;

        starX[i] = swipeCenterX + (((size * ((rc0 * 7) >> 4)) >> 16));
        starY[i] = swipeCenterY + ((size * rs0) >> 16);
        starX[i + 1] = swipeCenterX + (((innerSize * ((rc1 * 7) >> 4)) >> 16));
        starY[i + 1] = swipeCenterY + ((innerSize * rs1) >> 16);
    }
}

int abs(int value) {
    return value >= 0 ? value : -value;
}

// Integer square root, floor(sqrt(n)) for n>=0 -- the standard "digit by
// digit" method: shifts, adds, subtracts and compares only. No division
// (this coprocessor has none, confirmed) and no float. Verified against
// Python's exact math.isqrt() for every n in 0..95*95 (95 comfortably above
// any radius circleCoverRadius() can produce) -- exact match throughout.
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

// Border-only Bresenham line: marks borderMaskCur (via drawBorderBit()) for
// every pixel from (x0,y0) EXCLUSIVE to (x1,y1) inclusive, without touching
// swipeMask (that's fillMaskSpan()'s job elsewhere). Used to connect one
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
        // is most of the screen and clearly "inside" the shape. But near a
        // pole -- wherever this lap's disk is narrowest, which is always
        // somewhere near the top/bottom of whatever's currently visible, every
        // single lap -- left and right end up only 1-2 trix apart, and the
        // handful of columns between them never get a border mark: two
        // isolated dots with a visible gap between, right where the shape
        // should look rounded off. Once the gap's small, just fill it in too --
        // this only ever fires within a trix or two of a pole, so it's a
        // handful of extra points at most, not a per-row cost.
        if (right - left <= 4) {
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

            if (swipePhase == SWIPE_SHRINK) {
                ((unsigned char *)swipeMask)[y] &= ~rebase[x];
                ((unsigned char *)swipeMask)[y + 1] &= ~rebase[x];
            } else {
                ((unsigned char *)swipeMask)[y] |= rebase[x];
                ((unsigned char *)swipeMask)[y + 1] |= rebase[x];
            }

            // Border ring -- see drawBorderBit() for why this isn't phase-gated.
            ((unsigned char *)borderMaskCur)[y] |= rebase[x];
            ((unsigned char *)borderMaskCur)[y + 1] |= rebase[x];

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
