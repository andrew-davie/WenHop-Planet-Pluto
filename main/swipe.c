#include "defines_dasm.h"

#include "cdfjplus.h"


#include "main.h"


#if ENABLE_SWIPE

#include "swipe.h"

unsigned char swipeMask[6][SCREEN_TRIX_Y];

bool drawMaskBit(int x, int y);
void generateStar();
bool star();
void initStar(int x, int y);

short starX[10];
short starY[10];

int swipeRadius;    // 24.8
int swipeStep;      // 24.8

int swipeCenterX;
int swipeCenterY;
int circleX, circleY, circleDelta;
bool swipeComplete;
bool swipeVisible;

enum CIRCLEPHASE swipePhase;
enum SWIPE swipeType;


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

enum PHASE exitPhase;


void applySwipeMask() {

    if (!swipeComplete || exitPhase == PHASE_END) {    //! swipeComplete || triggerNextLife) {

        //        FLASH(0xD2, 10);

        unsigned char *p = RAM + _BUF_PF0_LEFT;
        for (int col = 0; col < 6; col++)
            for (int y = 0; y < _ICC_SCANLINES; y++) {
                unsigned char mask = swipeMask[col][y];
                *p++ &= mask;
                *p++ &= mask;
                *p++ &= mask;
            }
    }
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

void clearMask(int v) {
    unsigned char *p = (unsigned char *)swipeMask;
    for (int i = 0; i < 6 * _ICC_SCANLINES; i++)
        p[i] = v;
}

void initStarSwipe() {
    swipeType = SWIPE_STAR;
    clearMask(0);
}

bool drawMaskBit(int x, int y) {

    if (x < 0 || x >= _1ROW || y < 0 || y >= _ICC_SCANLINES)
        return false;

    if (swipePhase == SWIPE_SHRINK) {

        if (x >= 20) {
            x -= 20;
            y += 3 * _ICC_SCANLINES;
        }

        y += ((x + 4) >> 3) * _ICC_SCANLINES;

        ((unsigned char *)swipeMask)[y] &= ~EXTERNAL(__rebase)[x];
    }

    return true;
}

void swipe(int reserved) {

    // if (gameSchedule == SCHEDULE_UNPACK_CAVE)
    //     return;

    // We do the circle processing at start of VB or OS

    // int start = T1TC;
    while (!swipeComplete && T1TC < availableIdleTime - reserved) {    //< availableIdleTime - 70000) {

        switch (swipeType) {
        case SWIPE_CIRCLE: {

            //            FLASH(0xD2, 2);

            int x2 = (circleX * 7) >> 4;
            int y2 = (circleY * 7) >> 4;

            //            int start = T1TC;

            swipeVisible |= drawMaskBit(swipeCenterX + x2, swipeCenterY + circleY) |
                            drawMaskBit(swipeCenterX + y2, swipeCenterY + circleX) |
                            drawMaskBit(swipeCenterX + y2, swipeCenterY - circleX) |
                            drawMaskBit(swipeCenterX + x2, swipeCenterY - circleY) |
                            drawMaskBit(swipeCenterX - x2, swipeCenterY - circleY) |
                            drawMaskBit(swipeCenterX - y2, swipeCenterY - circleX) |
                            drawMaskBit(swipeCenterX - y2, swipeCenterY + circleX) |
                            drawMaskBit(swipeCenterX - x2, swipeCenterY + circleY);

            //          actualScore = T1TC - start;

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
                    return;
                }
                break;
            }

            setSwipe(swipeCenterX, swipeCenterY, swipeRadius, swipeStep, swipePhase);
            return;
        }
    }

    // int t = availableIdleTime - T1TC;
    // if (!actualScore || t < actualScore)
    //     actualScore = t;
    // if ((T1TC - start) > actualScore)
    //     actualScore = T1TC - start;
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
        if (y0 >= _ICC_SCANLINES || y1 < 0)
            return false;
    } else {
        if (y1 >= _ICC_SCANLINES || y0 < 0)
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

        if (x0 >= 0 && x0 < _1ROW && y0 >= 0 && y0 < _ICC_SCANLINES - 1) {

            int x = x0;
            int y = y0;

            if (x >= 20) {
                x -= 20;
                y += 3 * _ICC_SCANLINES;
            }

            y += ((x + 4) >> 3) * _ICC_SCANLINES;

            ((unsigned char *)swipeMask)[y] |= EXTERNAL(__rebase)[x];
            ((unsigned char *)swipeMask)[y + 1] |= EXTERNAL(__rebase)[x];

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
        swipeStep += 20;
    }

    bool visible = bresenhamLine(starX[starPoint], starY[starPoint], starX[starNextPoint], starY[starNextPoint]);

    starPoint = starNextPoint;
    return visible;
}

#endif
