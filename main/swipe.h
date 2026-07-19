#ifndef __SWIPE_C
#define __SWIPE_C

enum CIRCLEPHASE {
    SWIPE_SEARCH,
    SWIPE_REGROW,
    SWIPE_RESHRINK,

    SWIPE_GROW,
    SWIPE_SHRINK,
};

enum SWIPE {
    SWIPE_CIRCLE,
    SWIPE_STAR,
};

extern bool maskNeeded;

void swipe(int reserved);
void setSwipePhase(enum CIRCLEPHASE newPhase);
void setSwipeType(enum SWIPE newSwipeType);
void setSwipe(int x, int y, int radius, int step, enum CIRCLEPHASE phase);
void startSwipeClose(int x, int y);
void applySwipeMask(int buffer);
void setSwipeMaskColour(bool white);    // true = white/COLUPF reveal, false = black/background reveal
// void initSwipe(enum SWIPE type, int mask);
void initStarSwipe();
void randomizeStarAngle();    // re-rolls the star's one-time rotation; call once per fresh star sequence (grow or shrink), not per lap
void clearMask(int v);
bool checkSwipeFinished();

int abs(int value);
#endif
