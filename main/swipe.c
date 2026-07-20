#include "defines_dasm.h"

#include "cdfjplus.h"


#include "main.h"

#if ENABLE_SWIPE

#include "particle.h"    // sin_cos[32] -- star's one-time random rotation
#include "random.h"      // rangeRandom()
#include "swipe.h"

// Per-column bit within the byte a given x-coordinate (0-19, one screen half) maps to.
static const unsigned char rebase[20] = {
    1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 7, 1 << 6, 1 << 5, 1 << 4, 1 << 3, 1 << 2,
    1 << 1, 1 << 0, 1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7,
};

// aligned(4): clear/copy helpers below bulk-access these 4 bytes at a time via
// an unsigned int* cast -- ARMv4T needs that alignment guaranteed explicitly.
//
// swipeMaskA/B and borderMaskCur/Prev are each double-buffered so a pixel's
// mask and border only ever change together, at one lap-boundary swap (see
// newLapPending's handling in swipe()) -- avoids the mask visibly outrunning
// the border, or the border flashing, mid-lap.
static unsigned char swipeMaskA[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char swipeMaskB[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));

// Border ring isn't cumulative like the mask -- each lap fully redraws its
// own connected trace, so its double buffer needs no copy-forward step.
static unsigned char borderMaskCur[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char borderMaskPrev[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));

// applySwipeMask() reads *borderShowA | *borderShowB per pixel rather than a
// named buffer, so both swipe types share one double-buffer scheme:
// swipeWriteBorder names whichever physical buffer THIS lap is drawing into
// (always the one not currently shown); borderShowA/B both point at the
// other one (last lap's finished, fully-connected trace). ORing a buffer
// with itself is just itself, so this always shows exactly one complete
// ring, never a blend, never a half-finished one.
static unsigned char (*borderShowA)[SCREEN_TRIX_Y] = borderMaskCur;
static unsigned char (*borderShowB)[SCREEN_TRIX_Y] = borderMaskPrev;

// Always the physical buffer NOT currently named by borderShowA/B. Shared by
// both swipe types (only one active at a time).
static unsigned char (*swipeWriteBorder)[SCREEN_TRIX_Y] = borderMaskPrev;

// Mask's display/write pair -- same idea as borderShowA/B, but only one
// display pointer (the mask is never a blend, just one revealed/hidden
// bitmap). Unlike the border, a new lap's write target can't start cleared:
// the mask is cumulative (a lap only touches the band between old and new
// radius/size), so it has to start as a full copy of what's shown, with this
// lap's span-fills applied on top (see copyBuf()'s call site in swipe()).
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

// Star's GROW ramps its effective per-lap step from 1/STAR_GROW_RAMP_LAPS up
// to the full target over the first STAR_GROW_RAMP_LAPS laps, so the tiny
// star's actual appearance is visible instead of flashing past at full
// speed immediately. swipeStep itself still holds the true, unramped rate.
// Reset once per fresh GROW sequence by randomizeStarAngle().
#define STAR_GROW_RAMP_LAPS 8
static int starGrowLapCount;

// One-time star rotation, chosen once per fresh sequence (randomizeStarAngle())
// and held fixed for every lap -- generateStar() re-runs every lap, so the
// rotation has to live outside it. Q8 scale (sin_cos[32], particle.h), NOT
// the Q15 scale starSine/starCosine use -- generateStar() shifts accordingly.
static int starRotSin = 0;
static int starRotCos = 256;    // identity rotation by default (Q8: 256 = 1.0)

static int swipeCenterX;
static int swipeCenterY;

// circleRow counts dy OUTWARD from the centre row (0 .. sweep): circle()
// handles the +dy and -dy row together off one isqrt() per call.
static int circleRadius;      // current lap's disk radius, whole trix
static int circleRadiusSq;    // circleRadius*circleRadius, hoisted out of circle()'s per-row loop
static int circleRow;         // |dy| cursor this lap, 0 .. sweep

// Previous row's edge points, so circle() can connect consecutive rows with
// an actual line instead of isolated dots. Top and bottom are two
// independent chains (both start from the shared dy==0 point), so each
// needs its own prev-point state. *HasPrev is false at lap start and
// whenever a chain has just stepped outside the disk (no edge to connect to).
static int topPrevLeft, topPrevRight, topPrevRow;
static bool topHasPrev;
static int botPrevLeft, botPrevRight, botPrevRow;
static bool botHasPrev;

// Lets circle() skip re-filling mask bands unchanged since last lap, without
// a per-row history cache: SHRINK/GROW's radius changes by exactly `step`
// every lap, so last lap's radius is just circleRadius +/- step, recomputed
// with one extra isqrt() per row. circleHasOldRadius is false only for a
// sequence's first lap (nothing to derive an old radius from); set via
// circleFreshSequence by startSwipeClose() and decodeCaves.c's GROW start.
static bool circleFreshSequence = true;
static bool circleHasOldRadius;
static int oldRadiusSq;    // oldRadius*oldRadius, hoisted the same way as circleRadiusSq

void markCircleFreshSequence() {
    circleFreshSequence = true;
}

static bool swipeComplete;
static bool swipeVisible;

// Safety valve for a logic stall (e.g. a bug leaves swipeComplete
// permanently false) -- worst-case real sequences finish in well under 100
// frames, so anything still running after SWIPE_WATCHDOG_FRAMES is stuck,
// not slow. Resets to 0 every frame the sequence is genuinely idle.
//
// Only catches a stall in swipe()'s own logic -- can't do anything about a
// genuine ARM timing overrun, since that can take the whole system down
// before this code runs again. The real defence there is staying inside the
// time budget everywhere in this file, not a software fallback.
#define SWIPE_WATCHDOG_FRAMES 600
static int swipeWatchdogFrames;

// Finishing a sequence (SHRINK's clearMask(0)+clearBorderCur(), GROW's
// clearMask(255)) is real work -- up to ~200 words -- so it's done
// incrementally, one small unit at a time with a T1TC check before each,
// the same budget discipline circle()'s row loop uses. Guarantees forward
// progress every frame, however little time is left, rather than requiring
// a fixed amount of slack to exist all at once. finishStage/finishIndex
// track exactly where this is up to, across as many frames as it takes.
enum {
    FINISH_MASK_A,         // clearing swipeMaskA (255 for GROW, 0 for SHRINK)
    FINISH_MASK_B,         // clearing swipeMaskB -- both physical mask buffers need setting
    FINISH_BORDER_CUR,     // clearing borderMaskCur (always -- real data every time)
    FINISH_BORDER_PREV,    // clearing borderMaskPrev (skipped if already known zero)
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


// buffer is the left column (PF0_LEFT) of whichever kernel's six PF0/PF1/PF2
// LEFT/RIGHT planes we're masking; each plane is _BUFFER_SIZE apart.
// Every 4 rows (12 bytes) is exactly 3 ints, so 4 rows get bulk-processed as
// 3 int32 read-modify-writes. Little-endian assumed. Split into black/white
// variants so the hot path never re-tests maskWhite per int.
//
// Per pixel: if the border is set, force the bit fully ON. Otherwise, if
// maskShow (revealed) is set, leave the real content alone. Otherwise force
// per the black/white hidden-area rule. Per-int:
//   black: final = (orig & R) | B
//   white: final = orig | (~R | B)
// Skipped entirely if the relevant rows are already all-revealed and border-free.
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

        // Tail: leftover 2 rows plus padding.
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

    // maskNeeded only goes false when a GROW genuinely finishes -- not
    // gated on swipeComplete generally, since a SHRINK finishing also sets
    // that, but the mask is mostly-0 (not open) at that point.
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
// doesn't divide evenly -- currently does, 6*SCREEN_TRIX_Y=396=99*4).
#define MASK_BUF_BYTES (6 * SCREEN_TRIX_Y)
#define MASK_BUF_WORDS (MASK_BUF_BYTES / 4)

static void clearBorderCur() {
    unsigned int *p = (unsigned int *)borderMaskCur;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)borderMaskCur)[i] = 0;
}

// Tracks whether borderMaskPrev is already known all-zero, so a redundant
// clear can be skipped -- see its use in swipe()'s finishPending machine.
static bool borderPrevIsZero;

static void clearBorderPrev() {
    unsigned int *p = (unsigned int *)borderMaskPrev;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)borderMaskPrev)[i] = 0;
    borderPrevIsZero = true;
}

// Same clear, but by pointer -- swipeWriteBorder alternates between the two
// physical buffers lap to lap.
static void clearBorderBuf(unsigned char (*buf)[SCREEN_TRIX_Y]) {
    unsigned int *p = (unsigned int *)buf;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)buf)[i] = 0;
}

// Copies one [6][SCREEN_TRIX_Y] buffer over another -- seeds the mask's
// write target with everything accumulated so far, before this lap's
// span-fills apply on top (see swipeWriteMask's declaration).
static void copyBuf(unsigned char (*dst)[SCREEN_TRIX_Y], unsigned char (*src)[SCREEN_TRIX_Y]) {
    unsigned int *pd = (unsigned int *)dst;
    unsigned int *ps = (unsigned int *)src;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        pd[i] = ps[i];
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)dst)[i] = ((unsigned char *)src)[i];
}

// Gated on "a new lap started", not "a new frame happened", so this stays
// correct even if a lap spans multiple frames.
static bool newLapPending;

void setSwipe(int x, int y, int radius, int step, enum CIRCLEPHASE phase) {

    swipeCenterX = x;
    swipeCenterY = y;

    int effectiveStep = step;
    if (swipeType == SWIPE_STAR && phase == SWIPE_GROW && starGrowLapCount < STAR_GROW_RAMP_LAPS) {
        // Ease-in -- see starGrowLapCount's declaration. swipeStep itself
        // still gets the real, unramped value below. STAR_GROW_RAMP_LAPS is
        // a power of 2 so this stays a shift, not a real divide.
        effectiveStep = (step * (starGrowLapCount + 1)) >> 3;
        starGrowLapCount++;
    }

    swipeRadius = radius + effectiveStep;    // 24.8
    swipeStep = step;                        // 24.8 -- true target rate, not the ramped one

    // Clamp SHRINK to never go negative -- a large step can overshoot past 0
    // in one jump. Without this, the last lap would land on some negative
    // remainder rather than exactly 0, so the termination check (see
    // swipe()'s SWIPE_SHRINK case) had to stop one whole step early to
    // avoid ever handing circle() a negative radius. Clamping here means
    // the worst case is landing exactly on 0 (a single point), so that
    // check can just ask "have we reached 0 yet?" at any step size.
    if (phase == SWIPE_SHRINK && swipeRadius < 0)
        swipeRadius = 0;

    swipeComplete = false;
    swipeVisible = false;
    swipePhase = phase;
    newLapPending = true;

    switch (swipeType) {
    case SWIPE_CIRCLE: {
        circleRadius = swipeRadius >> 8;
        circleRadiusSq = circleRadius * circleRadius;
        // SHRINK sweeps circleRadius plus one step's margin, not the whole
        // starting extent -- just enough to explicitly hide the band of
        // rows that fell outside the disk since last lap, without sweeping
        // (and re-checking) rows that left the disk many laps ago. GROW
        // never needs to retract anything already revealed, so it just
        // sweeps its own current extent, no margin.
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

// Radius (whole trix, not 24.8) a circle centred at (cx, cy) needs to fully
// cover the screen. Takes the centre as explicit parameters, not
// swipeCenterX/Y -- startSwipeClose() calls this before setSwipe() updates
// those to the new death position.
//
// Combines dx/dy into a true corner distance (not max(rForX, rForY), which
// only guarantees each axis independently, not the actual farthest corner):
// solves circle()'s own per-row equation (halfWidth = isqrt(R^2 -
// squashDy2(dy)) * 7/16) for R at the corner's (dx, dy) offset.
//
// rForX is aspect-scaled (not plain Euclidean dx): circle() compresses its
// x-extent by 7/16 relative to the true half-width as a display aspect
// correction, so the radius needed to cover a given dx is dx*16/7.
static int circleCoverRadius(int cx, int cy) {
    int dx = cx > (_1ROW - 1 - cx) ? cx : (_1ROW - 1 - cx);
    int dy = cy > (SCREEN_TRIX_Y - 1 - cy) ? cy : (SCREEN_TRIX_Y - 1 - cy);
    int rForX = ((dx * 16 + 6) * (0x10000 / 7)) >> 16;    // ceil(dx*16/7)
    int dy2 = squashDy2(dy);                              // same squash circle() applies per-row
    int r = isqrt(dy2 + rForX * rForX);
    return r + 2;    // small safety margin
}

void startSwipeClose(int x, int y) {

    // Centre on the caller's supplied position (the player's current
    // position), not swipeCenterX/Y left over from the last setSwipe().
    //
    // Radius: circleCoverRadius(), not swipeRadius, for a circle -- a star
    // has to grow well past simple screen coverage before its pointed shape
    // clears every corner, so swipeRadius at GROW's end is bigger than a
    // circle needs. Star keeps reusing swipeRadius, since that IS correct
    // when grow and shrink are both star.
    //
    // Step: a fine per-lap step (not star's coarser one) so consecutive
    // laps' outlines stay close enough to read as continuous motion, since
    // circle no longer holds border data across laps (each lap draws a
    // fully connected outline of its own).
    int radius = (swipeType == SWIPE_CIRCLE) ? (circleCoverRadius(x, y) << 8) : swipeRadius;
    int step = (swipeType == SWIPE_CIRCLE) ? ((1 << 8) + (1 << 7)) : swipeStep;    // 1.25 trix/lap for circle

    if (swipeType == SWIPE_STAR)
        randomizeStarAngle();    // fresh look each time star is used to close, same reasoning as the grow
    else
        markCircleFreshSequence();    // fresh sequence -- last shrink/grow's radius doesn't apply here

    maskNeeded = true;    // grow completing turned this off; shrinking needs it applied again
    setSwipe(x, y, radius, -step, SWIPE_SHRINK);
}

// Sets BOTH physical mask buffers, not just whichever is current -- called
// at sequence start/finish, when both need to agree, not a per-lap update.
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
    // Shape-agnostic setup (mask clear + flag) despite the name -- doesn't
    // touch swipeType, so it doesn't override whatever setSwipeType() the
    // caller just set. Mask clears to 0 (hidden) and stays that way until
    // setSwipe() is called for real; swipeComplete = true holds swipe()
    // idle in the meantime (playerX/Y, and so the correct centre, aren't
    // known yet -- see decodeCaves.c).
    clearMask(0);
    clearBorderCur();
    clearBorderPrev();
    maskNeeded = true;
    swipeComplete = true;
}

// Sets (GROW/etc) or clears (SHRINK) every mask bit for x in [x0,x1] on row
// y (clipped to the screen). A lap's Bresenham circle only touches pixels at
// exactly the current radius -- a 1-pixel ring -- so the annulus between one
// lap's radius and the next needs an actual span fill, not just the ring.
//
// One screen half (xx 0-19) maps to 3 groups (see rebase[] and the
// (xx+4)>>3 split): group 0 is xx 0-3, upper nibble only (PF0, hardware only
// implements 4 bits there); group 1 is xx 4-11, full byte reversed (PF1);
// group 2 is xx 12-19, full byte forward (PF2). A span fully covering a
// group can be set in one byte write instead of a per-column rebase[] loop
// -- only matters for the ragged ends.
static void fillHalfSpan(int xa, int xb, int rowBase, bool setBits) {

    if (xa > xb)
        return;

    // Writes through swipeWriteMask, not the literal swipeMaskA/B.
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
// (fillMaskSpan()'s job). Marks 2 consecutive trix-rows per point (y and
// y+1), same as bresenhamLine() does for star -- trix rows are physically
// shorter than trix columns are wide (7/16 aspect ratio), so a "tall pixel"
// is needed for uniform border thickness regardless of a segment's slope.
//
// Bounds check tightened to SCREEN_TRIX_Y-1 so the y+1 mark always stays
// within the same column's contiguous block.
bool drawBorderBit(int x, int y) {

    if (x < 0 || x >= _1ROW || y < 0 || y >= SCREEN_TRIX_Y - 1)
        return false;

    if (x >= 20) {
        x -= 20;
        y += 3 * SCREEN_TRIX_Y;
    }

    y += ((x + 4) >> 3) * SCREEN_TRIX_Y;

    // Writes into whichever physical buffer swipeWriteBorder currently
    // names -- see its declaration.
    ((unsigned char *)swipeWriteBorder)[y] |= rebase[x];
    ((unsigned char *)swipeWriteBorder)[y + 1] |= rebase[x];

    return true;
}

void swipe(int reserved) {

    // Watchdog -- see swipeWatchdogFrames' declaration. Idle keeps it reset
    // to 0; an active sequence that hasn't finished after
    // SWIPE_WATCHDOG_FRAMES gets force-closed via the same incremental
    // finishPending path a normal termination uses.
    if (swipeComplete && !finishPending) {
        swipeWatchdogFrames = 0;
    } else if (!finishPending && ++swipeWatchdogFrames > SWIPE_WATCHDOG_FRAMES) {
        finishStage = FINISH_MASK_A;
        finishIndex = 0;
        finishPending = true;
        swipeComplete = false;    // not really done until the incremental finish below actually runs
    }

    // Incremental finish -- see finishStage's declaration. One word at a
    // time, re-checking T1TC before each one.
    if (finishPending) {
        while (finishStage != FINISH_DONE && T1TC < availableIdleTime - reserved) {

            int tailBytes = MASK_BUF_BYTES - MASK_BUF_WORDS * 4;    // 0 today -- kept generic
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
                    // borderMaskPrev holds real data whenever the write-target
                    // swap has touched it -- skip clearing an already-zero buffer.
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

    // Start of a new lap -- promote swipeWriteBorder (this lap's just-
    // finished, fully-connected trace) to display, and hand the other
    // physical buffer to swipeWriteBorder as the next lap's write target,
    // clearing it first since drawBorderBit()/bresenhamLine() only ever OR
    // bits in. Works for a sequence's first lap and type-switches too, since
    // both physical buffers are already all-zero then.
    //
    // The mask swaps at the same moment, for the same reason (mask and
    // border must only ever change together). Its write target starts as a
    // full copy of what's currently displayed via copyBuf(), since the mask
    // is cumulative, not redrawn from scratch each lap.
    if (newLapPending) {

        unsigned char (*justFinishedBorder)[SCREEN_TRIX_Y] = swipeWriteBorder;
        unsigned char (*nextWriteBorder)[SCREEN_TRIX_Y] =
            (justFinishedBorder == borderMaskCur) ? borderMaskPrev : borderMaskCur;

        borderShowA = justFinishedBorder;
        borderShowB = justFinishedBorder;
        swipeWriteBorder = nextWriteBorder;

        // Pessimistically mark not-zero the instant this buffer becomes a
        // write target -- if this is the sequence's last lap,
        // finishPending's borderPrevIsZero check runs right after and needs
        // to see "not zero" for the real data this lap is about to trace.
        if (nextWriteBorder == borderMaskPrev)
            borderPrevIsZero = false;

        clearBorderBuf(nextWriteBorder);

        unsigned char (*justFinishedMask)[SCREEN_TRIX_Y] = swipeWriteMask;
        unsigned char (*nextWriteMask)[SCREEN_TRIX_Y] = (justFinishedMask == swipeMaskA) ? swipeMaskB : swipeMaskA;

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
                // Not gated on !swipeVisible -- SHRINK starts at
                // circleCoverRadius(), which can legitimately sit outside
                // the visible screen for its earliest laps (nothing wrong,
                // just hasn't started mattering yet). GROW does need a
                // visibility-based end condition, since it starts small
                // with no natural "big enough" radius to check against.
                //
                // Checks swipeRadius <= 0 directly -- setSwipe() clamps
                // SHRINK to land on exactly 0 rather than undershooting, so
                // this plain check is correct at any step size.
                if (swipeRadius <= 0) {
                    // clearMask(0) + clearing borderMaskCur is real work --
                    // handed off to the budget-checked finishPending path
                    // rather than run unconditionally here.
                    finishStage = FINISH_MASK_A;
                    finishIndex = 0;
                    finishPending = true;
                    swipeComplete = false;    // not really done until finishPending's clear actually runs
                    return;
                }
                break;

            case SWIPE_GROW:
                if (!swipeVisible) {
                    // Same reasoning as SWIPE_SHRINK above.
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

// Unscaled cosine -- same angles/scale as starCosine[], but without its
// baked-in 7/16 aspect correction, so it can be rotated in true (isotropic)
// space first, with the aspect squeeze applied afterwards only to the final
// x. Rotating the already-squeezed starCosine[] would rotate an ellipse
// instead of a circle.
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

// Re-rolls the star's one-time rotation. Call once per fresh star sequence
// (grow start, or shrink start if the shrink is a star) -- not per lap. A
// 5-pointed star has 5-fold rotational symmetry, so a plain index-shift
// through starCosine/starSine would only reproduce the same look; picking
// an arbitrary angle from sin_cos[32] and rotating properly actually varies it.
void randomizeStarAngle() {
    int r = rangeRandom(32) & 0x1F;
    starRotSin = sin_cos[r];
    starRotCos = sin_cos[(r + 8) & 0x1F];
    starGrowLapCount = 0;    // fresh sequence -- see starGrowLapCount's declaration
}

// Plain C >> on a negative int floor-rounds (toward -infinity), biasing
// every value derived from a negative intermediate outward. Fine for
// magnitude-only values (isqrt() results), wrong for generateStar()'s
// signed rc0/rs0/rc1/rs1 and the vertex coordinates built from them --
// truncates toward zero instead (same negate/shift/negate trick as abs()),
// keeping the star's centroid on swipeCenterX/Y regardless of rotation.
static int symShr(int v, int n) {
    return (v >= 0) ? (v >> n) : -((-v) >> n);
}

void generateStar() {

    int size = swipeRadius >> 8;
    int innerSize = (size * 7) >> 4;

    for (int i = 0; i < 10; i += 2) {

        // Rotate the true (unsquashed) unit vector for each vertex by the
        // fixed one-time angle before applying the 7/16 x-squeeze and the
        // radius scale. starTrueCosine/starSine are Q15, starRotSin/Cos are
        // Q8, so the product is Q23 -- >>8 brings it back to Q15.
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

// Integer square root, floor(sqrt(n)) for n>=0 -- digit-by-digit method:
// shifts, adds, subtracts and compares only. No division (this coprocessor
// has none) and no float.
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

// The circle rendered very slightly taller than wide -- trims the vertical
// reach by ~5% to compensate. Scales dy^2 by 71/64 before it's subtracted
// from radius^2 in circle()'s rem calculation, via shifts+add/subtract on
// the already-computed dy*dy rather than an extra multiply.
// circleCoverRadius()'s own dy reach is scaled up to compensate.
static int squashDy2(int dy) {
    int dy2 = dy * dy;
    return dy2 + (dy2 >> 3) - (dy2 >> 6);
}

// Border-only Bresenham line: marks swipeWriteBorder for every pixel from
// (x0,y0) EXCLUSIVE to (x1,y1) inclusive, without touching the mask. Used to
// connect one row's edge point to the next's.
//
// Deliberately doesn't draw the (x0,y0) start point -- every call site
// chains from the previous call's endpoint (or an initial seed point drawn
// directly by drawBorderBit()), so (x0,y0) here was already drawn.
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

// Draws the +dy and -dy row pair at the current lap's radius, off one
// isqrt() (half-width = sqrt(radius^2 - dy^2), aspect-scaled by 7/16).
// Border edges connect to the previous row via bresenhamBorderLine(); top
// and bottom trace outward from dy==0 as two independent chains. Sweep is
// bounded to circleRadius (+step margin for SHRINK, see setSwipe()). Mask
// fill only touches the band changed since last lap (see circleHasOldRadius).
bool circle() {

    bool visible = false;

    int dy = circleRow;
    int topRowY = swipeCenterY - dy;
    int botRowY = swipeCenterY + dy;

    int dy2 = squashDy2(dy);
    int rem = circleRadiusSq - dy2;

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
        // hide it now (the sweep margin in setSwipe() guarantees this row
        // is visited exactly once, the lap it transitions out; GROW never
        // sweeps beyond its own current radius, so can't hit this branch).
        // Only re-hides whatever was still revealed last lap.
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
            // Shared starting point for both chains -- nothing to connect to yet.
            visible |= drawBorderBit(left, topRowY);
            visible |= drawBorderBit(right, topRowY);
            topPrevLeft = botPrevLeft = left;
            topPrevRight = botPrevRight = right;
            topPrevRow = botPrevRow = topRowY;
            topHasPrev = botHasPrev = true;
        } else {
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

        // Left/right are never connected horizontally, only the vertical
        // row-to-row chains above. At the disk's true tip (the last row
        // with any width, no next row to connect to), that leaves two
        // isolated dots with a gap -- fill it in on just that one row,
        // detected via the next row's rem going negative.
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

            // Writes through swipeWriteMask, not the literal swipeMaskA/B.
            if (swipePhase == SWIPE_SHRINK) {
                ((unsigned char *)swipeWriteMask)[y] &= ~rebase[x];
                ((unsigned char *)swipeWriteMask)[y + 1] &= ~rebase[x];
            } else {
                ((unsigned char *)swipeWriteMask)[y] |= rebase[x];
                ((unsigned char *)swipeWriteMask)[y + 1] |= rebase[x];
            }

            // Border ring -- not phase-gated, see drawBorderBit().
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

    // Segments connect an outer vertex (even index) to an inner one (odd
    // index); the inner vertex is always closer to swipeCenterX/Y, so it's
    // more likely still on-screen once the star's grown past screen-
    // covering size. bresenhamLine() only short-circuits after a point's
    // been visible, so walking inner->outer puts any off-screen slack at
    // the tail where that catches it.
    bool visible;
    if (starPoint % 2 == 0)    // starPoint is outer, starNextPoint is inner -- walk inner -> outer
        visible = bresenhamLine(starX[starNextPoint], starY[starNextPoint], starX[starPoint], starY[starPoint]);
    else    // starPoint is inner already
        visible = bresenhamLine(starX[starPoint], starY[starPoint], starX[starNextPoint], starY[starNextPoint]);

    starPoint = starNextPoint;
    return visible;
}

#endif
