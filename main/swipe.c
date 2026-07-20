#include "defines_dasm.h"

#include "cdfjplus.h"


#include "main.h"

#if ENABLE_SWIPE

#include "particle.h"    // sin_cos[32]
#include "random.h"
#include "swipe.h"

// Bit within the byte a given x (0-19, one screen half) maps to.
static const unsigned char rebase[20] = {
    1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 7, 1 << 6, 1 << 5, 1 << 4, 1 << 3, 1 << 2,
    1 << 1, 1 << 0, 1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7,
};

// aligned(4) for bulk unsigned int* access (ARMv4T needs it explicit).
// Mask and border are each double-buffered, swapped together once per lap
// (see newLapPending in swipe()), so a pixel's mask/border only ever change
// in lockstep -- no mid-lap mask-outrunning-border or border-flash glitches.
static unsigned char swipeMaskA[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char swipeMaskB[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char borderMaskCur[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));
static unsigned char borderMaskPrev[6][SCREEN_TRIX_Y] __attribute__((aligned(4)));

// borderShowA/B point at last lap's finished trace (both types share one
// scheme); swipeWriteBorder is the other physical buffer, this lap's write
// target. maskShow/swipeWriteMask are the mask's equivalent, but the mask
// is cumulative so its write target starts as a copy, not a blank clear
// (see copyBuf()'s call site in swipe()).
static unsigned char (*borderShowA)[SCREEN_TRIX_Y] = borderMaskCur;
static unsigned char (*borderShowB)[SCREEN_TRIX_Y] = borderMaskPrev;
static unsigned char (*swipeWriteBorder)[SCREEN_TRIX_Y] = borderMaskPrev;
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

// Star GROW ramps its per-lap step up over the first STAR_GROW_RAMP_LAPS
// laps so the initial appearance is visible instead of flashing past.
#define STAR_GROW_RAMP_LAPS 8
static int starGrowLapCount;

// One-time star rotation, re-rolled per fresh sequence, held for every lap.
// Q8 (sin_cos[32]), not the Q15 starSine/starCosine scale.
static int starRotSin = 0;
static int starRotCos = 256;    // identity (Q8: 256 = 1.0)

static int swipeCenterX;
static int swipeCenterY;

// circleRow counts dy outward from centre (0 .. sweep); circle() handles
// +dy/-dy together off one isqrt() per call.
static int circleRadius;      // current lap's disk radius, whole trix
static int circleRadiusSq;
static int circleRow;

// Previous row's edge points, to connect consecutive rows with a line
// instead of isolated dots. Top/bottom are independent chains from dy==0.
static int topPrevLeft, topPrevRight, topPrevRow;
static bool topHasPrev;
static int botPrevLeft, botPrevRight, botPrevRow;
static bool botHasPrev;

// Lets circle() skip mask bands unchanged since last lap: radius changes by
// exactly `step` every lap, so last lap's radius is circleRadius +/- step.
// circleHasOldRadius is false only for a sequence's first lap.
static bool circleFreshSequence = true;
static bool circleHasOldRadius;
static int oldRadiusSq;

void markCircleFreshSequence() {
    circleFreshSequence = true;
}

static bool swipeComplete;
static bool swipeVisible;

// Force-closes a sequence that's still running after SWIPE_WATCHDOG_FRAMES
// (real sequences finish well under 100). Resets whenever genuinely idle.
// Can't help with a real ARM timing overrun, only a logic stall.
#define SWIPE_WATCHDOG_FRAMES 600
static int swipeWatchdogFrames;

// Sequence-end clear (SHRINK: clearMask(0)+clearBorderCur(); GROW:
// clearMask(255)) is real work, so it's done incrementally, one word per
// T1TC check, same discipline as circle()'s row loop.
enum {
    FINISH_MASK_A,
    FINISH_MASK_B,
    FINISH_BORDER_CUR,
    FINISH_BORDER_PREV,    // skipped if borderPrevIsZero
    FINISH_DONE
};
static unsigned char finishStage;
static int finishIndex;    // word cursor within finishStage's buffer
static bool finishPending;

bool maskNeeded = true;           // false once fully open -- lets applySwipeMask skip the AND loop
static bool maskWhite = false;    // hidden area forced ON (white) vs OFF (black)

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


// buffer is PF0_LEFT of the six PF0/PF1/PF2 LEFT/RIGHT planes, each
// _BUFFER_SIZE apart. 4 rows = 3 ints, bulk-processed as 3 read-modify-writes.
// Per pixel: border forces the bit ON; else mask (revealed) leaves it alone;
// else force per black/white. Skipped where rows are already fully revealed
// and border-free.
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


#define MASK_BUF_BYTES (6 * SCREEN_TRIX_Y)
#define MASK_BUF_WORDS (MASK_BUF_BYTES / 4)

static void clearBorderCur() {
    unsigned int *p = (unsigned int *)borderMaskCur;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)borderMaskCur)[i] = 0;
}

static bool borderPrevIsZero;

static void clearBorderPrev() {
    unsigned int *p = (unsigned int *)borderMaskPrev;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)borderMaskPrev)[i] = 0;
    borderPrevIsZero = true;
}

static void clearBorderBuf(unsigned char (*buf)[SCREEN_TRIX_Y]) {
    unsigned int *p = (unsigned int *)buf;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        p[i] = 0;
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)buf)[i] = 0;
}

static void copyBuf(unsigned char (*dst)[SCREEN_TRIX_Y], unsigned char (*src)[SCREEN_TRIX_Y]) {
    unsigned int *pd = (unsigned int *)dst;
    unsigned int *ps = (unsigned int *)src;
    for (int i = 0; i < MASK_BUF_WORDS; i++)
        pd[i] = ps[i];
    for (int i = MASK_BUF_WORDS * 4; i < MASK_BUF_BYTES; i++)
        ((unsigned char *)dst)[i] = ((unsigned char *)src)[i];
}

static bool newLapPending;

void setSwipe(int x, int y, int radius, int step, enum CIRCLEPHASE phase) {

    swipeCenterX = x;
    swipeCenterY = y;

    int effectiveStep = step;
    if (swipeType == SWIPE_STAR && phase == SWIPE_GROW && starGrowLapCount < STAR_GROW_RAMP_LAPS) {
        effectiveStep = (step * (starGrowLapCount + 1)) >> 3;
        starGrowLapCount++;
    }

    swipeRadius = radius + effectiveStep;    // 24.8
    swipeStep = step;                        // 24.8, unramped

    // Clamp SHRINK to land on exactly 0 rather than undershooting -- lets
    // the termination check below just ask "radius <= 0?" at any step size.
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
        circleRow = 0;
        topHasPrev = false;
        botHasPrev = false;
        circleHasOldRadius = !circleFreshSequence;
        circleFreshSequence = false;
        if (circleHasOldRadius) {
            int oldRadius = circleRadius - (swipeStep >> 8);
            oldRadiusSq = oldRadius * oldRadius;
        }
        break;
    }

    case SWIPE_STAR:
        generateStar();
        break;
    }
}

// Radius (whole trix) a circle at (cx, cy) needs to fully cover the screen
// -- true corner distance (not max of the two axes independently), solved
// from circle()'s own per-row equation. rForX is aspect-scaled (7/16), same
// as circle()'s halfWidth.
static int circleCoverRadius(int cx, int cy) {
    int dx = cx > (_1ROW - 1 - cx) ? cx : (_1ROW - 1 - cx);
    int dy = cy > (SCREEN_TRIX_Y - 1 - cy) ? cy : (SCREEN_TRIX_Y - 1 - cy);
    int rForX = ((dx * 16 + 6) * (0x10000 / 7)) >> 16;    // ceil(dx*16/7)
    int dy2 = squashDy2(dy);
    int r = isqrt(dy2 + rForX * rForX);
    return r + 2;    // safety margin
}

void startSwipeClose(int x, int y) {

    // Circle uses circleCoverRadius(), not swipeRadius -- a star has to
    // overshoot screen coverage before its pointed shape clears every
    // corner, so its swipeRadius is bigger than a circle needs.
    int radius = (swipeType == SWIPE_CIRCLE) ? (circleCoverRadius(x, y) << 8) : swipeRadius;
    int step = (swipeType == SWIPE_CIRCLE) ? ((1 << 8) + (1 << 7)) : swipeStep;    // 1.25 trix/lap for circle

    if (swipeType == SWIPE_STAR)
        randomizeStarAngle();
    else
        markCircleFreshSequence();

    maskNeeded = true;
    setSwipe(x, y, radius, -step, SWIPE_SHRINK);
}

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
    // Shape-agnostic (doesn't touch swipeType). Holds idle, fully hidden,
    // until setSwipe() is called for real once the player position is known.
    clearMask(0);
    clearBorderCur();
    clearBorderPrev();
    maskNeeded = true;
    swipeComplete = true;
}

// Sets/clears mask bits for x in [x0,x1] on row y. One screen half maps to
// 3 groups (see rebase[]): xx 0-3 upper nibble only (PF0, 4 real bits), xx
// 4-11 reversed byte (PF1), xx 12-19 forward byte (PF2) -- a span fully
// covering a group is one write instead of a per-column loop.
static void fillHalfSpan(int xa, int xb, int rowBase, bool setBits) {

    if (xa > xb)
        return;

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

    int la = x0 > 0 ? x0 : 0;
    int lb = x1 < 19 ? x1 : 19;
    fillHalfSpan(la, lb, y, setBits);

    int ra = x0 >= 20 ? x0 - 20 : 0;
    int rb = x1 < 39 ? x1 - 20 : 19;
    fillHalfSpan(ra, rb, y + 3 * SCREEN_TRIX_Y, setBits);
}

// Marks the border ring (not the mask). 2 trix-rows per point for uniform
// thickness (trix rows are shorter than columns are wide, 7/16 aspect).
bool drawBorderBit(int x, int y) {

    if (x < 0 || x >= _1ROW || y < 0 || y >= SCREEN_TRIX_Y - 1)
        return false;

    if (x >= 20) {
        x -= 20;
        y += 3 * SCREEN_TRIX_Y;
    }

    y += ((x + 4) >> 3) * SCREEN_TRIX_Y;

    ((unsigned char *)swipeWriteBorder)[y] |= rebase[x];
    ((unsigned char *)swipeWriteBorder)[y + 1] |= rebase[x];

    return true;
}

void swipe(int reserved) {

    if (swipeComplete && !finishPending) {
        swipeWatchdogFrames = 0;
    } else if (!finishPending && ++swipeWatchdogFrames > SWIPE_WATCHDOG_FRAMES) {
        finishStage = FINISH_MASK_A;
        finishIndex = 0;
        finishPending = true;
        swipeComplete = false;
    }

    if (finishPending) {
        while (finishStage != FINISH_DONE && T1TC < availableIdleTime - reserved) {

            int tailBytes = MASK_BUF_BYTES - MASK_BUF_WORDS * 4;    // 0 today
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

    // New lap: promote swipeWriteBorder to display, hand the other physical
    // buffer over as the next write target (cleared first), and swap the
    // mask the same moment (copied forward, since it's cumulative).
    if (newLapPending) {

        unsigned char (*justFinishedBorder)[SCREEN_TRIX_Y] = swipeWriteBorder;
        unsigned char (*nextWriteBorder)[SCREEN_TRIX_Y] =
            (justFinishedBorder == borderMaskCur) ? borderMaskPrev : borderMaskCur;

        borderShowA = justFinishedBorder;
        borderShowB = justFinishedBorder;
        swipeWriteBorder = nextWriteBorder;

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
                if (swipeRadius <= 0) {
                    finishStage = FINISH_MASK_A;
                    finishIndex = 0;
                    finishPending = true;
                    swipeComplete = false;
                    return;
                }
                break;

            case SWIPE_GROW:
                if (!swipeVisible) {
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

// Unscaled cosine, no aspect correction baked in -- rotated in true space
// first, squeezed afterwards (rotating starCosine[] directly would distort
// a circle into an ellipse).
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

void randomizeStarAngle() {
    int r = rangeRandom(32) & 0x1F;
    starRotSin = sin_cos[r];
    starRotCos = sin_cos[(r + 8) & 0x1F];
    starGrowLapCount = 0;
}

// Plain >> floor-rounds negative ints; this truncates toward zero instead,
// keeping the star's centroid on swipeCenterX/Y regardless of rotation.
static int symShr(int v, int n) {
    return (v >= 0) ? (v >> n) : -((-v) >> n);
}

void generateStar() {

    int size = swipeRadius >> 8;
    int innerSize = (size * 7) >> 4;

    for (int i = 0; i < 10; i += 2) {

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

// floor(sqrt(n)), digit-by-digit -- no divide, no float.
static int isqrt(int n) {
    if (n <= 0)
        return 0;
    int res = 0;
    int bit = 1 << 14;
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

// ~5% vertical-reach trim so the circle doesn't render taller than wide --
// scales dy^2 by 71/64 before it's subtracted from radius^2 in circle().
static int squashDy2(int dy) {
    int dy2 = dy * dy;
    return dy2 + (dy2 >> 3) - (dy2 >> 6);
}

// Connects one row's edge point to the next's, EXCLUSIVE of (x0,y0) --
// every call site chains from the previous endpoint, already drawn.
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

// Draws the +dy/-dy row pair at the current lap's radius off one isqrt().
// Sweep bounded to circleRadius (+step margin for SHRINK). Mask fill only
// touches the band changed since last lap (circleHasOldRadius).
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
        // Outside the disk -- SHRINK hides whatever was still revealed.
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

        int w = isqrt(rem);
        int halfWidth = (w * 7) >> 4;

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

        // Fill the gap at the disk's tip, where there's no next row to
        // connect the two edges vertically.
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

            if (swipePhase == SWIPE_SHRINK) {
                ((unsigned char *)swipeWriteMask)[y] &= ~rebase[x];
                ((unsigned char *)swipeWriteMask)[y + 1] &= ~rebase[x];
            } else {
                ((unsigned char *)swipeWriteMask)[y] |= rebase[x];
                ((unsigned char *)swipeWriteMask)[y + 1] |= rebase[x];
            }

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

    // Walk inner->outer vertex -- the inner one's more likely on-screen,
    // and bresenhamLine() only short-circuits after going visible.
    bool visible;
    if (starPoint % 2 == 0)
        visible = bresenhamLine(starX[starNextPoint], starY[starNextPoint], starX[starPoint], starY[starPoint]);
    else
        visible = bresenhamLine(starX[starPoint], starY[starPoint], starX[starNextPoint], starY[starNextPoint]);

    starPoint = starNextPoint;
    return visible;
}

#endif
