
#include "defines_dasm.h"

#include "cavedata.h"
#include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "main.h"
#include "mellon.h"
#include "playerAnimation.h"
#include "scroll.h"

int scrollX, scrollY;

static int scrollSpeedX, scrollSpeedY;
// static int targetScrollSpeed, targetYScrollSpeed;

// #define accel 4096
// #define accelY (accel * 16)
// #define decel (accel * 8 / 8)
// #define decelY (decel * 8)

// void calculateVisibleBorders();

bool isScrolling() {
    return (scrollSpeedX || scrollSpeedY);
}


void clamp(int *v, int min, int max) {
    if (*v < min)
        *v = min;
    if (*v > max)
        *v = max;
}


int approach(int current, int target, int speed) {


    int diff = target - current;
    if (diff > speed)
        diff = speed;
    if (diff < -speed)
        diff = -speed;
    return current + diff;
}


void scroll() {

    // if (true) {
    if (playerDead && !waitRelease) {    // && *playerAnimation == FRAME_BLANK) {

        // Manual look-around

        int dir = (swcha ^ 0xFF) >> 4;
        scrollSpeedX = xInc[dir] << 16;
        scrollSpeedY = yInc[dir] << 17;
    }

    else {


        // Auto-tracking

#define MAX_SPEED_X 0x7000
#define SCROLL_EDGE_X ((SCREEN_TRIX_X / 5) << 16)
#define ACCEL_X (1 << 12)
#define DECEL_X (2 << 9)

        int tX = ((playerX * CHAR_TRIX_X) << 16) + (CHAR_TRIX_X << 15);
        int hX = scrollX + (SCREEN_TRIX_X << 15);

        if (tX < (hX - SCROLL_EDGE_X)) {
            scrollSpeedX = approach(scrollSpeedX, -MAX_SPEED_X, ACCEL_X);
        } else if (tX > (hX + SCROLL_EDGE_X))
            scrollSpeedX = approach(scrollSpeedX, MAX_SPEED_X, ACCEL_X);
        else
            scrollSpeedX = approach(scrollSpeedX, 0, DECEL_X);

#define MAX_SPEED_Y (1 << 16)
#define SCROLL_EDGE_Y (4 << 16)
#define ACCEL_Y (1 << 13)
#define DECEL_Y (1 << 12)

        int tY = ((playerY * CHAR_TRIX_Y) << 16) + (CHAR_TRIX_Y << 15);
        int hY = scrollY + (SCREEN_TRIX_Y << 15) + (CHAR_TRIX_Y << 15);    // @navel :)

        if (tY < (hY - SCROLL_EDGE_Y))
            scrollSpeedY = approach(scrollSpeedY, -MAX_SPEED_Y, ACCEL_Y);
        else if (tY > (hY + SCROLL_EDGE_Y))
            scrollSpeedY = approach(scrollSpeedY, MAX_SPEED_Y, ACCEL_Y);
        else
            scrollSpeedY = approach(scrollSpeedY, 0, DECEL_Y);
    }

    scrollX += scrollSpeedX;
    scrollY += scrollSpeedY;


    int bounds_l = ((theCave->bounds_l) * CHAR_TRIX_X) << 16;
    int bounds_r = ((theCave->bounds_r - 8) * CHAR_TRIX_X) << 16;
    int bounds_t = ((theCave->bounds_t) * CHAR_TRIX_Y) << 16;
    int bounds_b = ((theCave->bounds_b) * CHAR_TRIX_Y) << 16;


    if (scrollX > bounds_r) {
        scrollX = bounds_r;
        scrollSpeedX = 0;
    }

    else if (scrollX < bounds_l) {
        scrollX = bounds_l;
        scrollSpeedX = 0;
    }

    if (scrollY > bounds_b) {
        scrollY = bounds_b;
        scrollSpeedY = 0;
    }

    else if (scrollY < bounds_t) {
        scrollY = bounds_t;
        scrollSpeedY = 0;
    }


    if (theCave->flags & CAVEDEF_LOCK_X) {
        scrollX = 0x70000;
        scrollSpeedX = 0;
    }

    if (theCave->flags & CAVEDEF_LOCK_Y) {
        scrollY = 0x110000;
        scrollSpeedY = 0;
    }
}


void resetTracking() {

    scrollX = ((playerX * CHAR_TRIX_X - (SCREEN_TRIX_X >> 1)) << 16) + (CHAR_TRIX_X << 15);
    clamp(&scrollX, SCROLL_MIN_X, SCROLL_MAX_X);

    scrollY = ((playerY * CHAR_TRIX_Y - (SCREEN_TRIX_Y >> 1)) << 16);    // + (CHAR_TRIX_Y << 16);
    clamp(&scrollY, SCROLL_MIN_Y, SCROLL_MAX_Y);

    scrollSpeedX = scrollSpeedY = 0;
}


// EOF