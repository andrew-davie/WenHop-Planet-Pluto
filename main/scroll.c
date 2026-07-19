
#include "defines_dasm.h"

#include "decodeCaves.h"
#include "main.h"
#include "mellon.h"
#include "scroll.h"
#include "swipe.h"

int scrollX, scrollY;

static int scrollSpeedX, scrollSpeedY;

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

    if (!maskNeeded && playerDead && !waitRelease) {    // && *playerAnimation == FRAME_BLANK) {

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


    int bounds_l = (theCave->bounds_l) << 16;
    int bounds_r = (theCave->bounds_r) << 16;
    int bounds_t = (theCave->bounds_t) << 16;
    int bounds_b = (theCave->bounds_b) << 16;

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

    if (scrollY < bounds_t) {
        scrollY = bounds_t;
        scrollSpeedY = 0;
    }
}


void resetTracking() {

    // Clamp against theCave->bounds_l/r/t/b -- the SAME per-cave range
    // scroll()'s own hard clamp (above) enforces every frame -- not against
    // SCROLL_MIN/MAX_X/Y (the absolute board-wide range, used only as
    // drawScreen()'s last-ditch renderer safety clamp; see scroll.h). Those
    // two ranges normally coincide for an ordinary cave, but a cave that
    // locks an axis (e.g. a vertically-locked cave, where bounds_t ==
    // bounds_b pins the camera to one fixed scrollY regardless of player
    // position) has a per-cave range narrower than the board-wide one.
    // Using the wrong, wider range here meant resetTracking() could place
    // scrollX/scrollY somewhere scroll()'s own clamp would immediately
    // overrule on the very next frame it ran -- and since setSwipe() (see
    // schedule.c) can fire before or after that correction happens,
    // depending on exactly how many frames cave decode takes and where in
    // the object stream the player birth marker falls, the star's one-time
    // centring could end up capturing either the right or the wrong value.
    // Matching scroll()'s own clamp here removes that race entirely: this
    // is now the same value scroll() would settle on immediately anyway.
    scrollX = ((playerX * CHAR_TRIX_X - (SCREEN_TRIX_X >> 1)) << 16) + (CHAR_TRIX_X << 15);
    clamp(&scrollX, theCave->bounds_l << 16, theCave->bounds_r << 16);

    scrollY = ((playerY * CHAR_TRIX_Y - (SCREEN_TRIX_Y >> 1)) << 16);    // + (CHAR_TRIX_Y << 16);
    clamp(&scrollY, theCave->bounds_t << 16, theCave->bounds_b << 16);

    scrollSpeedX = scrollSpeedY = 0;
}


// EOF