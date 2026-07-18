#include "defines_dasm.h"

#include "cdfjplus.h"


#include "main.h"


#if ENABLE_SWIPE

#include "swipe.h"

// per-column bit within the byte a given x-coordinate (0-19, one screen half) maps to
static const unsigned char rebase[20] = {
    1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 7, 1 << 6, 1 << 5, 1 << 4, 1 << 3, 1 << 2,
    1 << 1, 1 << 0, 1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7,
};

static unsigned char swipeMask[6][SCREEN_TRIX_Y];

bool drawMaskBit(int x, int y);
void generateStar();
bool star();
void initStar(int x, int y);

static short starX[10];
static short starY[10];

static int swipeRadius;    // 24.8
static int swipeStep;      // 24.8

static int swipeCenterX;
static int swipeCenterY;
static int circleX, circleY, circleDelta;
static bool swipeComplete;
static bool swipeVisible;
static bool maskNeeded = true;    // false once fully open (grown-in) -- lets applySwipeMask skip the AND loop

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


void applySwipeMask(int buffer) {

    // Skip entirely once the mask is known fully-open (all 255 -- a no-op AND
    // either way, so there's nothing to gain by running it). Do NOT gate this
    // on swipeComplete generally though -- that was the previous bug: shrink
    // finishing also sets swipeComplete, but the mask is mostly-0 at that
    // point, not open, so skipping there let the raw buffer flash through.
    // maskNeeded only goes false when a grow genuinely finishes (see swipe()).
    if (!maskNeeded)
        return;

    // buffer is the left column (PF0_LEFT) of whichever kernel's six
    // PF0/PF1/PF2 LEFT/RIGHT planes we're masking; each plane is
    // _BUFFER_SIZE apart, so address each one from buffer rather than
    // walking a single pointer across all six (they aren't contiguous
    // with the SCREEN_TRIX_Y*3 content -- there's _BUFFER_SIZE - _SCANLINES
    // bytes of slack after each plane).
    for (int col = 0; col < 6; col++) {
        unsigned char *p = RAM + buffer + col * _BUFFER_SIZE;
        for (int y = 0; y < SCREEN_TRIX_Y; y++) {
            unsigned char mask = swipeMask[col][y];
            *p++ &= mask;
            *p++ &= mask;
            *p++ &= mask;
        }
    }
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


void setSwipe(int x, int y, int radius, int step, enum CIRCLEPHASE phase) {

    swipeCenterX = x;
    swipeCenterY = y;

    swipeRadius = radius + step;    // 24.8
    swipeStep = step;               // 24.8

    swipeComplete = false;
    swipeVisible = false;
    swipePhase = phase;

    switch (swipeType) {
    case SWIPE_CIRCLE:
        circleX = swipeRadius >> 8;
        circleY = 0;
        circleDelta = 1 - circleX;
        break;

    case SWIPE_STAR:
        generateStar();
        break;
    }
}

void startSwipeClose() {

    // Reuses whatever centre/radius the last setSwipe() left us at (i.e. wherever
    // the grow finished, already covering the screen) and reverses direction --
    // no need for the SEARCH/REGROW/RESHRINK dance since we already know we're
    // fully covering. Shrinks at the same rate the grow used, just backwards.
    maskNeeded = true;    // grow completing turned this off; shrinking needs it applied again
    setSwipe(swipeCenterX, swipeCenterY, swipeRadius, -swipeStep, SWIPE_SHRINK);
}

void clearMask(int v) {
    unsigned char *p = (unsigned char *)swipeMask;
    for (int i = 0; i < 6 * SCREEN_TRIX_Y; i++)
        p[i] = v;
}

void initStarSwipe() {
    swipeType = SWIPE_STAR;
    clearMask(0);
    maskNeeded = true;
}

bool drawMaskBit(int x, int y) {

    if (x < 0 || x >= _1ROW || y < 0 || y >= SCREEN_TRIX_Y)
        return false;

    if (swipePhase == SWIPE_SHRINK) {

        if (x >= 20) {
            x -= 20;
            y += 3 * SCREEN_TRIX_Y;
        }

        y += ((x + 4) >> 3) * SCREEN_TRIX_Y;

        ((unsigned char *)swipeMask)[y] &= ~rebase[x];
    }

    return true;
}

void swipe(int reserved) {

    // We do the circle processing at start of VB or OS

    while (!swipeComplete && T1TC < availableIdleTime - reserved) {

        switch (swipeType) {
        case SWIPE_CIRCLE: {

            int x2 = (circleX * 7) >> 4;
            int y2 = (circleY * 7) >> 4;

            swipeVisible |= drawMaskBit(swipeCenterX + x2, swipeCenterY + circleY) |
                            drawMaskBit(swipeCenterX + y2, swipeCenterY + circleX) |
                            drawMaskBit(swipeCenterX + y2, swipeCenterY - circleX) |
                            drawMaskBit(swipeCenterX + x2, swipeCenterY - circleY) |
                            drawMaskBit(swipeCenterX - x2, swipeCenterY - circleY) |
                            drawMaskBit(swipeCenterX - y2, swipeCenterY - circleX) |
                            drawMaskBit(swipeCenterX - y2, swipeCenterY + circleX) |
                            drawMaskBit(swipeCenterX - x2, swipeCenterY + circleY);

            circleY++;

            if (circleDelta < 0)
                circleDelta += 2 * circleY + 1;
            else
                circleDelta += 2 * (circleY - --circleX) + 1;
            swipeComplete = circleX < circleY;
            break;
        }

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
                if (!swipeVisible || swipeRadius <= -swipeStep)
                    return;
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

void generateStar() {

    int size = swipeRadius >> 8;
    int innerSize = (size * 7) >> 4;

    for (int i = 0; i < 10; i += 2) {
        starX[i] = swipeCenterX + ((((size * starCosine[i]) >> 16)));
        starY[i] = swipeCenterY + ((size * starSine[i]) >> 16);
        starX[i + 1] = swipeCenterX + ((((innerSize * starCosine[i + 1]) >> 16)));
        starY[i + 1] = swipeCenterY + ((innerSize * starSine[i + 1]) >> 16);
    }
}

int abs(int value) {
    return value >= 0 ? value : -value;
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
        // was: swipeStep += 20; -- accumulated every lap regardless of phase, so the
        // growth rate itself accelerated every frame and any starting step blew up
        // within a handful of frames. Keep growth flat/linear (set explicitly by
        // whoever calls setSwipe()) until there's a deliberate reason to accelerate.
    }

    bool visible = bresenhamLine(starX[starPoint], starY[starPoint], starX[starNextPoint], starY[starNextPoint]);

    starPoint = starNextPoint;
    return visible;
}

#endif
