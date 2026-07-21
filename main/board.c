#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "animations.h"
#include "attribute.h"
#include "board.h"
// #include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "gameState.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "random.h"
#include "schedule.h"
#include "scroll.h"
#include "sound.h"
#include "swipe.h"
#include "wyrm.h"

#define PARTICLE_GRAVITY_FLAG 0x80


// must be init'd at startup
static int selectorCounter;
static int waterDir;
static int explodeCount;
static int explodeRadius;

// init'd locally
static int conveyorDirection;
static int activeStar;
static int lastActiveStar;
static bool single;


bool processTypes(BoardCursor *cur, enum ObjectType type, unsigned char creature);
void processCreatures(BoardCursor *cur, unsigned char creature);
void restartBoardScan();

void processDoge(unsigned char *me, int row, int col);
void processPebble(unsigned char *me, int row, int col);
void processWater(unsigned char *me, int row);
void processWaterFlow(unsigned char *me, int row, int col);
void processCharBeltAndGrinder(unsigned char *me, unsigned char creature);
void processFallingThings(unsigned char *me, int row, int col, unsigned char creature);
void processCharGeoDogeAndRock(unsigned char *me);

void genericPush(unsigned char *me, int row, int col, int offsetX, int offsetY);
void genericPushReverse(unsigned char *me, int offsetX, int offsetY);
void chainReact_GeoDogeToDoge(unsigned char *me);
void chainReact_Pipe(unsigned char *me);
void doRoll(unsigned char *me, int row, int col);
void setInsulator(unsigned char *p, int row, int col);

//------------------------------------------------------------------------------

#define isVisible(x, y) (onScreenX[x] && onScreenY[y])

#define W4 ((_BOARD_COLS + 3) & ~3)
#define H4 ((_BOARD_ROWS + 3) & ~3)

bool onScreenX[W4] __attribute__((aligned(4)));
bool onScreenY[H4] __attribute__((aligned(4)));

void calculateVisibleMasks() {

    myMemsetInt((unsigned int *)onScreenX, 0, sizeof(onScreenX) / 4);
    myMemsetInt((unsigned int *)onScreenY, 0, sizeof(onScreenY) / 4);

    int sX = scrollX >> 16;
    int eX = sX + 40 + CHAR_TRIX_X;
    int x = (sX * (0x10000 / CHAR_TRIX_X)) >> 16;

    // Bounded by the board's actual trixel width (BOARD_TRIX_X) rather than
    // SCREEN_TRIX_X/_BOARD_COLS (a different unit that happens to share the
    // value 40) -- otherwise this stops marking columns visible as soon as
    // scrollX passes ~1/4 of the way across the level. Also cap the array
    // index directly so a scroll near the right edge can't overrun onScreenX.
    for (int i = sX; i <= eX && i < BOARD_TRIX_X && x < _BOARD_COLS; i += CHAR_TRIX_X)
        onScreenX[x++] = true;

    int sY = scrollY >> 16;
    int eY = sY + _SCANLINES / 3;
    int y = (sY * (0x10000 / CHAR_TRIX_Y)) >> 16;

    for (int i = sY; i < eY && y < _BOARD_ROWS; i += CHAR_TRIX_Y)
        onScreenY[y++] = true;
}

//------------------------------------------------------------------------------

static const int isActive[] = {

    ATT_PHASE1 | ATT_PHASE2,    // 0
    ATT_PHASE1 | ATT_PHASE4,    // 1
    ATT_PHASE1 | ATT_PHASE2,    // 2
    ATT_PHASE1,                 // 3
};

void initBoard() {

    selectorCounter = 0;
    waterDir = 0;
    explodeCount = 0;
    explodeRadius = 0;
    conveyorDirection = -1;    // ?
    activeStar = lastActiveStar = 0;
    single = false;
}


void setupBoardScanner() {

    // After board scan complete, throttles until we're at correct FPS
    if (gameFrame >= gameSpeed) {    // && !autoMoveFrameCount) {

        waterDir++;

        restartBoardScan();

        if (gameState == nextGameState)
            processWyrms();

        gameFrame = 0;

        lastActiveStar = activeStar;
        activeStar = 0;

        if (!single && !totalDogePossible) {
            single = true;
            FLASH(0xC8, 10);
            startCharAnimation(TYPE_STAR, AnimateStar);
        }

        ++selectorCounter;

        if (showLava) {

            static int ss = 0;
            if (--ss < 0)
                ss = 20;

            if (gravity > 0 && lavaSurfaceTrixel && !ss)
                lavaSurfaceTrixel -= gravity;

            // Surface lava "bubbles"
            int posX = ((scrollX * 5) >> 16) + rangeRandom(_BOARD_COLS);
            nDotsAtTrixel(1, posX, lavaSurfaceTrixel - 2, PT_SPIRAL, 120, 0x10, 7);    // untested speed
        }

        if (showWater) {
            static const unsigned char sinus[] = {0, 1, 1, 2, 2, 2, 3, 3, 4, 3, 3, 2, 2, 2, 1, 1};
            static int waves = 0;

            if (lavaSurfaceTrixel) {

                lavaSurfaceTrixel -= sinus[(waves >> 0) & 15];

                if (((sinus[(waves >> 0) & 15] & 3) != 0) || (gameFrame & 31) == 0)
                    ++waves;

                lavaSurfaceTrixel += sinus[(waves >> 0) & 15];
            }

            for (int i = 0; i < 4; i++) {
                int posX = ((scrollX * 5) >> 16) + rangeRandom(_BOARD_COLS);
                int deep = lavaSurfaceTrixel + rangeRandom(21 * CHAR_Y - lavaSurfaceTrixel);
                if (deep > 0 && deep < _SCANLINES)
                    bubbles(1, posX, deep, 2240, 0x8000);
            }
        }

        // gameFrame++;
        gravity = nextGravity;

        usableSWCHA = bufferedSWCHA;
        bufferedSWCHA = 0xFF;

        // if (boardRow < 0) {
        //     boardRow = _BOARD_ROWS - 1;
        //     boardCol = _BOARD_COLS - 1;
        // } else {
        boardCol = 0;
        boardRow = 0;
        // }

        calculateVisibleMasks();

        setSchedule(SCHEDULE_PROCESS_BOARD);

        processBoardSquares();
    }
}


void processBoardSquares() {

    int chIndex = -1;
    unsigned int chStart = 0;

    while (gameState == nextGameState && T1TC < availableIdleTime - 12500) {

        // boardRow/boardCol are the only pieces of this that genuinely need
        // to survive across frames (this loop yields when its time budget
        // runs out and resumes here next frame). Everything else is
        // per-cell context, so from here down it's threaded explicitly via
        // 'cur' instead of leaning on implicit globals.
        BoardCursor cur;
        cur.row = boardRow;
        cur.col = boardCol;
        cur.me = RAM + _BOARD + cur.row * _BOARD_COLS + cur.col;

#ifdef DEBUG_TIMES
        // Close the previous cell's timing window right where the next one
        // opens -- one T1TC read serves as both, so this doesn't cost an
        // extra volatile register read on top of the one it's timing.
        unsigned int now = T1TC;
        if (chIndex >= 0) {
            unsigned int elapsed = now - chStart;
            if ((int)elapsed > debug[chIndex])
                debug[chIndex] = elapsed;
        }
#endif

        unsigned char creature = *cur.me;

        chStart = now;
        chIndex = GET(creature);

        if (creature < FLAG_THISFRAME) {

            enum ObjectType type = CharToType[creature];

            // Any blanks/dissolves underwater/lava gets transformed to water/lava chars
            if (isVisible(cur.col, cur.row))
                if (cur.row * CHAR_TRIX_Y >= lavaSurfaceTrixel && Attribute[type] & ATT_CONVERT)
                    *cur.me = showLava ? CH_LAVA_BLANK : CH_WATER;

            if (Attribute[type] & isActive[selectorCounter & 3]) {
                if (!processTypes(&cur, type, creature))
                    processCreatures(&cur, creature);
            }
        }


        // Clear any "scanned this frame" objects on the previous line
        // note: we need to also do the last row ... or do we? if it's steel wall, no

        // if (gravity > 0) {
        if (cur.row > 1) {
            unsigned char *prev = cur.me - _BOARD_COLS;
            *prev &= ~FLAG_THISFRAME;
        }
        // }

        // else {
        //     if (cur.row < _BOARD_ROWS - 1) {
        //         unsigned char *prev = cur.me + _BOARD_COLS;
        //         *prev &= ~FLAG_THISFRAME;
        //     }
        // }

        cur.me += gravity;
        cur.col += gravity;

        // if (gravity < 0) {
        //     if (cur.col < 0) {
        //         cur.col = _BOARD_COLS - 1;

        //         if (!--cur.row) {
        //             // Leave boardRow negative so setupBoardScanner's
        //             // "if (boardRow < 0)" restart branch actually triggers;
        //             // otherwise the next scan wrongly restarts at (0,0)
        //             // instead of the bottom row whenever gravity is negative.
        //             boardRow = -1;
        //             boardCol = cur.col;
        //             setSchedule(SCHEDULE_START_SCAN);
        //             return;
        //         }
        //     }
        // }

        // else {
        if (cur.col > (_BOARD_COLS - 1)) {
            cur.col = 0;
            if (++cur.row > _BOARD_ROWS - 1) {
                boardRow = cur.row;
                boardCol = cur.col;
                setSchedule(SCHEDULE_START_SCAN);


#ifdef DEBUG_TIMES

                // Early exit -- no "next fetch" is coming to close this
                // cell's window, so close it here instead.
                unsigned int elapsed = T1TC - chStart;
                if ((int)elapsed > debug[chIndex])
                    debug[chIndex] = elapsed;

#endif
                return;
            }
            // }
        }

        boardRow = cur.row;
        boardCol = cur.col;
    }

#ifdef DEBUG_TIMES
    // Loop exited via its own condition (gameState changed, or this frame's
    // time budget ran out) rather than the early return above -- close
    // whatever cell was mid-flight when that happened. Guarded because
    // chIndex is still -1 if the loop body never ran at all this call.
    if (chIndex >= 0) {
        unsigned int elapsed = T1TC - chStart;
        if ((int)elapsed > debug[chIndex])
            debug[chIndex] = elapsed;
    }
#endif
}


bool processTypes(BoardCursor *cur, enum ObjectType type, unsigned char creature) {

    switch (type) {


    case TYPE_STAR_EXPLODE: {

        activeStar++;
        if (*Animate[TYPE_STAR_EXPLODE] == CH_DUST_0) {
            ADDAUDIO(SFX_EXPLODE);
            nDots(8, cur->col, cur->row, PT_TWO, 25, CHAR_CENTER_X, CHAR_CENTER_Y, 80, 7);
            *cur->me = FLAG(CH_DUST_0);
        }
        break;
    }

    case TYPE_STAR: {

        static int activeDelay = 0;
        static int delay = 30;

        if (!lastActiveStar) {
            if (--activeDelay < 0) {
                activeDelay = (delay >> 1) + 1;
                ADDAUDIO(SFX_SPACE);
            }
        }

        // fall

        unsigned char *next = cur->me + _BOARD_COLS;
        if (Attribute[CharToType[GET(*next)]] & ATT_BLANK) {
            *next = FLAG(CH_STAR_FALLING_BOTTOM);
            *cur->me = FLAG(CH_STAR_FALLING_TOP);
            nDots(5, cur->col, cur->row, PT_TWO, 40, CHAR_CENTER_X, CHAR_CENTER_Y, 40, 2);
        }


        break;
    }


    case TYPE_OUTBOX:
        FLASH(0x28, 10);
        nDots(10, cur->col, cur->row, PT_SPIRAL, 40, 2, 5, 0x40, 7);    // untested speed
        break;

    case TYPE_DOGE:
        processDoge(cur->me, cur->row, cur->col);
        break;

    case TYPE_PEBBLE1:
        processPebble(cur->me, cur->row, cur->col);
        break;

    case TYPE_WATER:
        processWater(cur->me, cur->row);
        break;

    case TYPE_MELLON_HUSK:
        if (!exitMode)
            movePlayer(cur);

        else {

            exitMode--;
            if (exitMode < 20) {
                lumTarget = -15;
                if (lumTarget == luminance && gameState == nextGameState) {
                    setGameState(GS_MENU);
                    // return;
                }
            }
        }

        break;

    case TYPE_WATERFLOW_0:
    case TYPE_WATERFLOW_1:
    case TYPE_WATERFLOW_2:
    case TYPE_WATERFLOW_3:
    case TYPE_WATERFLOW_4:
        processWaterFlow(cur->me, cur->row, cur->col);
        break;

    case TYPE_GRINDER:
    case TYPE_GRINDER_1:
        if (!(getRandom32() & 7))
            nDots(1, cur->col, cur->row, PT_TWO, 10, 3, 7, 100, 7);

        __attribute__((fallthrough));

    case TYPE_BELT:
    case TYPE_BELT_1:
        processCharBeltAndGrinder(cur->me, creature);
        break;

    case TYPE_GEODOGE:
        processCharGeoDogeAndRock(cur->me);
        break;

    default:
        return false;
        break;
    }

    return true;
}


void setInsulator(unsigned char *p, int row, int col) {

    int visibleLeft = scrollX >> 16;
    int colTrix = col * CHAR_TRIX_X;

    if (colTrix <= visibleLeft - CHAR_TRIX_X || colTrix >= visibleLeft + SCREEN_TRIX_X) {
        return;
    }

    p += _BOARD_COLS;

    if (!onOff[col]) {
        if (lastOnOff[col]) {
            while (++row < _BOARD_ROWS && CharToType[GET(*p)] != TYPE_INSULATOR) {
                if (CharToType[GET(*p)] == TYPE_ELECTRIC)
                    *p = FLAG(CH_BLANK);
                p += _BOARD_COLS;
            }
        }
    }

    else {

        while (++row < _BOARD_ROWS && CharToType[GET(*p)] != TYPE_INSULATOR) {

            int ch = GET(*p);
            int type2 = CharToType[GET(*p)];
            unsigned int att = Attribute[type2];


            if (att & (ATT_EXPLODABLE | ATT_GEODOGE | ATT_DISSOLVES)) {
                *p = CH_ELECTRIC_0;

                if (type2 != TYPE_BLANK)
                    nDots(10, col, row, PT_SPIRAL, 10 + rangeRandom(10), CHAR_CENTER_X, CHAR_CENTER_Y, 40, 7);
                return;
            }

            if (att & ATT_BLANK) {

                if (ch >= CH_ELECTRIC_H0 && ch <= CH_ELECTRIC_H3) {
                    *p = CH_CROSSED_STREAMS;
                }

                else {

                    if (type2 != TYPE_ELECTRIC)
                        *p = CH_ELECTRIC_0;    // + (!rangeRandom(230) ? 1 : 0);
                }
            }


            else if (type2 == TYPE_ROCK_BONUS) {

                *p = FLAG(CH_STAR);
                // nDots(1, col, row, PT_SPIRAL, 10 + rangeRandom(10), CHAR_CENTER_X, 0, 40, 7);
                return;
            }


            // if (*p == CH_CROSSED_STREAMS)
            //     nDots(3, col, row, PT_SPIRAL, 10 + rangeRandom(10), CHAR_CENTER_X, CHAR_CENTER_Y, 40, 2);

            p += _BOARD_COLS;
        }
    }
}


void setInsulatorHoriz(unsigned char *p, int row, int col) {

    p++;


    if (!onOffHoriz[row]) {
        if (lastOnOffHoriz[row])
            while (++col < _BOARD_COLS && CharToType[GET(*p)] != TYPE_INSULATOR) {
                if (CharToType[GET(*p)] == TYPE_ELECTRIC)
                    *p = FLAG(CH_BLANK);
                p++;
            }
    }

    else {

        while (++col < _BOARD_COLS && CharToType[GET(*p)] != TYPE_INSULATOR) {

            int ch = GET(*p);
            int type2 = CharToType[GET(*p)];
            unsigned int att = Attribute[type2];

            if (att & (ATT_EXPLODABLE | ATT_GEODOGE | ATT_DISSOLVES)) {


                *p = CH_ELECTRIC_H0;
                if (type2 != TYPE_BLANK)
                    nDots(10, col, row, PT_SPIRAL, 10 + rangeRandom(10), CHAR_CENTER_X, CHAR_CENTER_Y, 40, 7);
                return;
            }

            if (att & ATT_BLANK) {

                if (ch >= CH_ELECTRIC_0 && ch <= CH_ELECTRIC_3) {
                    *p = CH_CROSSED_STREAMS;
                }

                else if (type2 != TYPE_ELECTRIC)
                    *p = CH_ELECTRIC_H0;    // + (!rangeRandom(230) ? 1 : 0);
            }


            else if (type2 == TYPE_ROCK_BONUS) {

                *p = FLAG(CH_STAR);
                // nDots(1, col, row, PT_SPIRAL, 10 + rangeRandom(10), CHAR_CENTER_X, 0, 40, 7);
                return;
            }


            if (*p == CH_CROSSED_STREAMS) {
                nDots(5, col, row, PT_SPIRAL2, 20, CHAR_CENTER_X + 1, CHAR_CENTER_Y, 10 + rangeRandom(20), 7);
            }
            p++;
        }
    }
}


bool onOff[_BOARD_COLS] = {false};
bool lastOnOff[_BOARD_COLS] = {false};
bool onOffHoriz[_BOARD_ROWS] = {false};
bool lastOnOffHoriz[_BOARD_ROWS] = {false};

void setInsulatorPattern() {

    static int s = 0;
    s++;

    for (int i = 0; i < _BOARD_COLS; i++) {
        lastOnOff[i] = onOff[i];
        onOff[i] = sin_cos[(s + i * 20) & 31] < 128;
    }

    for (int i = 0; i < _BOARD_ROWS; i++) {
        lastOnOffHoriz[i] = onOffHoriz[i];
        onOffHoriz[i] = sin_cos[(s + i * 20) & 31] < 128;
    }
}

int pickDifferent(int current) {

    if (!rangeRandom(10))
        return 0;

    if (current || !rangeRandom(10)) {
        if (++current > 3)
            current = 1;
    }

    return current;
}

void processCreatures(BoardCursor *cur, unsigned char creature) {

    switch (creature) {

    case CH_ELECTRIC_0:
    case CH_ELECTRIC_1:
    case CH_ELECTRIC_2:
    case CH_ELECTRIC_3: {
        *cur->me = pickDifferent(*cur->me - CH_ELECTRIC_0) + CH_ELECTRIC_0;
        break;
    }

    case CH_ELECTRIC_H0:
    case CH_ELECTRIC_H1:
    case CH_ELECTRIC_H2:
    case CH_ELECTRIC_H3: {
        *cur->me = pickDifferent(*cur->me - CH_ELECTRIC_H0) + CH_ELECTRIC_H0;
        break;
    }


    case CH_INSULATOR_TOP:
        setInsulator(cur->me, cur->row, cur->col);
        break;

    case CH_INSULATOR_L:
        setInsulatorHoriz(cur->me, cur->row, cur->col);
        break;

    case CH_ROCK_PEBBLE_1:
        *cur->me = FLAG(CH_ROCK_PEBBLE);
        break;

    case CH_ROCK_PEBBLE:
        *cur->me = FLAG(CH_BLANK);
        break;

    case CH_PEBBLE_ROCK:
        *cur->me = FLAG(CH_GEODOGE);
        break;

    case CH_PUSH_LEFT:
        genericPush(cur->me, cur->row, cur->col, -1, 0);
        break;

    case CH_PUSH_LEFT_REVERSE:
        genericPushReverse(cur->me, 1, 0);
        break;

    case CH_PUSH_RIGHT:
        genericPush(cur->me, cur->row, cur->col, 1, 0);
        break;

    case CH_PUSH_RIGHT_REVERSE:
        genericPushReverse(cur->me, -1, 0);
        break;

    case CH_PUSH_UP:
        genericPush(cur->me, cur->row, cur->col, 0, -1);
        break;

    case CH_PUSH_UP_REVERSE:
        genericPushReverse(cur->me, 0, 1);
        break;

    case CH_PUSH_DOWN:
        genericPush(cur->me, cur->row, cur->col, 0, 1);
        break;

    case CH_PUSH_DOWN_REVERSE:
        genericPushReverse(cur->me, 0, -1);
        break;

    case CH_DUST_2:
    case CH_DUST_ROCK_2:
        *cur->me = CH_BLANK;
        break;

    case CH_DUST_0:
    case CH_DUST_1:
    case CH_DUST_ROCK_0:
    case CH_DUST_ROCK_1:
        (*cur->me)++;
        break;

    case CH_CONVERT_GEODE_TO_DOGE:
        *cur->me = FLAG(CH_DOGE_00);
        chainReact_GeoDogeToDoge(cur->me);
        break;

    case CH_CONVERT_PIPE:
        chainReact_Pipe(cur->me);
        break;

    case CH_BLOCK: {

        unsigned char *next = cur->me + _BOARD_COLS * gravity;
        enum ObjectType typeDown = CharToType[GET(*next)];
        if (Attribute[typeDown] & ATT_BLANK) {
            *next = FLAG(*cur->me);
            *cur->me = FLAG(CH_DUST_0);
        }
        break;
    }

    case CH_ROCK_BONUS: {

        unsigned char *next = cur->me + _BOARD_COLS * gravity;
        if (Attribute[CharToType[GET(*next)]] & ATT_BLANK) {
            *next = FLAG(*cur->me);
            *cur->me = FLAG(CH_BLANK);
        }

        break;
    }


    case CH_STAR_FALLING_TOP:
        *cur->me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_STAR_FALLING_BOTTOM:
        *cur->me = FLAG(CH_STAR);
        break;


    case CH_ROCK:
        processCharGeoDogeAndRock(cur->me);
        break;

    case CH_ROCK_FALLING_TOP:
        *cur->me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_GEODOGE_FALLING_TOP:

        nDots(3, cur->col, cur->row, PT_TWO, 15, rangeRandom(CHAR_TRIX_X), rangeRandom(CHAR_TRIX_Y), 0, 2);

        *cur->me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_DOGE_SIDE_1:
    case CH_DOGE_SIDE_2:
        *cur->me = FLAG(CH_BLANK);
        break;

    case CH_DOGE_SIDE_3:
    case CH_DOGE_SIDE_4:

        *cur->me = FLAG(CH_DOGE_FALLING_TOP);
        *(cur->me + gravity * _BOARD_COLS) = FLAG(CH_DOGE_FALLING_BOTTOM);
        break;

    case CH_DOGE_FALLING_TOP2:
        *cur->me = FLAG(CH_DOGE_FALLING_TOP);
        *(cur->me + gravity * _BOARD_COLS) = FLAG(CH_DOGE_FALLING_BOTTOM);
        break;

    case CH_DOGE_FALLING_TOP:
        *cur->me = FLAG(CH_BLANK);
        break;

    case CH_DOGE_FALLING_BOTTOM:
        *cur->me = FLAG(CH_DOGE_FALLING);
        break;

    case CH_ROCK_FALLING_BOTTOM:
        *cur->me = FLAG(CH_ROCK_FALLING);
        break;

    case CH_GEODOGE_FALLING_BOTTOM:
        *cur->me = FLAG(CH_GEODOGE_FALLING);
        break;

    case CH_DOGE_FALLING:
    case CH_ROCK_FALLING:
    case CH_GEODOGE_FALLING: {

        processFallingThings(cur->me, cur->row, cur->col, creature);
        break;
    }

    case CH_DOORCLOSED:
        if (!doges) {
            *cur->me = CH_DOOROPEN_0;
            FLASH(0x28, 10);
        }
        break;

    case CH_DOOROPEN_0:
        FLASH(0xC4, 10);
        nDots(2, cur->col, cur->row, PT_TWO, 10, 3, 7, 100, 7);
        break;


    case CH_MELLON_HUSK_BIRTH:

        if (
#if ENABLE_SWIPE
            checkSwipeFinished() &&
#endif
            (!isScrolling())) {
            *cur->me = CH_MELLON_HUSK;
        }
        break;

    default:
        break;
    }
}


void restartBoardScan() {

    // Change insulator pattern

    setInsulatorPattern();


    if (!autoMoveFrameCount) {    // delay until fully in new square

        bool oldDead = playerDead;

        unsigned char *man = RAM + _BOARD + playerY * _BOARD_COLS + playerX;

        enum ChName what = GET(*man);

        if ((usableSWCHA & 0xF0) == 0xF0)
            waitRelease = false;

        enum ObjectType manType = CharToType[what];
        playerDead =
            (manType != TYPE_MELLON_HUSK && manType != TYPE_MELLON_HUSK_PRE && manType != TYPE_OUTBOX && !exitMode);


        if (oldDead != playerDead) {

            shakeTime = 40;
            FLASH(0x34, 20);

            attachment = 0;

            explodeCount = 6;
            explodeRadius = 10;

            initParticles();

            for (int i = 0; i < 10; i++)
                nDots(1, playerX, playerY, PT_TWO, 30 + rangeRandom(10), CHAR_CENTER_X, CHAR_CENTER_Y + 2, 30, 3);

            startPlayerAnimation(ID_Die);
            waitRelease = true;
            lives--;

            sound_max_volume = VOLUME_NONPLAYING;

#if ENABLE_SWIPE
            if (playerDead) {
                setSwipeType(SWIPE_CIRCLE);
                // Same world-to-screen math decodeCaves.c uses to centre the
                // grow -- here it's the player's CURRENT position (death can
                // happen anywhere on the board), not wherever the level
                // started. playerX/Y are the character CELL (top-left corner,
                // in trix), not the player's actual on-screen centre -- a
                // character is CHAR_TRIX_X x CHAR_TRIX_Y trix, so the middle of
                // that cell is +CHAR_CENTER_X/+CHAR_CENTER_Y from the corner
                // (same offset particle.c's baseX/baseY use to centre effects
                // on the player).
                startSwipeClose(playerX * CHAR_TRIX_X + CHAR_CENTER_X - (scrollX >> 16),
                                playerY * CHAR_TRIX_Y + CHAR_CENTER_Y - (scrollY >> 16));
            }
#endif
        }


        if (playerDead && !shakeTime && gameState == nextGameState
#if ENABLE_SWIPE
            && checkSwipeFinished()
#else
            && !(inpt4 & 0x80)
#endif
        ) {
            setGameState(GS_MENU);
            return;
        }


        if (explodeCount > 0) {

            explodeRadius++;

            for (int i = 0; i < 5; i++)
                nDots(1, playerX, playerY, PT_TWO, rangeRandom(20),
                      rangeRandom(explodeRadius) - (explodeRadius >> 1) + CHAR_CENTER_X,
                      rangeRandom(explodeRadius * 2) - explodeRadius + CHAR_CENTER_Y, rangeRandom(10),
                      rangeRandom(2) + 2);
            --explodeCount;
        }
    }
}


void processDoge(unsigned char *me, int row, int col) {

    unsigned char *next = me + _BOARD_COLS * gravity;
    const unsigned int attrNext = Attribute[CharToType[GET(*next)]];

    if (attrNext & ATT_BLANK) {
        *me = FLAG(CH_DOGE_FALLING_TOP);
        *next = FLAG(CH_DOGE_FALLING_BOTTOM);
    }

    else if (attrNext & ATT_ROLL)
        doRoll(me, row, col);
}


void processPebble(unsigned char *me, int row, int col) {

    // don't form way above player (but DO form 1 above)
    if (col == playerX && row < playerY - 1)
        return;

    int chance = 250;

    for (int i = 0; i < 4; i++)
        if (TYPEOF(*(me + dirOffset[i])) == TYPE_MELLON_HUSK) {
            chance = 50;
            break;
        }

    if (!rangeRandom((chance))) {
        *me = FLAG(CH_PEBBLE_ROCK);
        nDots(10, col, row, PT_SPIRAL, 20, 2, 5, 0x40, 7);
    }
}

void processWater(unsigned char *me, int row) {

    if ((row - 1) * CHAR_TRIX_Y >= lavaSurfaceTrixel) {
        unsigned char *neighbour = me + dirOffset[waterDir & 3];
        if (Attribute[CharToType[GET(*neighbour)]] & ATT_DISSOLVES) {
            *neighbour = CH_DUST_0;
        }
    }
}

void processWaterFlow(unsigned char *me, int row, int col) {

    // Lag the interruption of water flowing downwards
    unsigned char above = *(me - _BOARD_COLS);
    if (above < FLAG_THISFRAME && !(Attribute[CharToType[GET(above)]] & ATT_WATERFLOW)) {
        *me = FLAG(CH_BLANK);
        return;
    }

    int line = (row + 1) * CHAR_TRIX_Y;
    if (line < lavaSurfaceTrixel) {
        if (row < 20) {

            unsigned char *next = me + _BOARD_COLS * gravity;
            const unsigned int att = Attribute[CharToType[GET(*next)]];
            if (!(att & ATT_WATERFLOW)) {

                if (att & (ATT_DISSOLVES | ATT_BLANK)) {

                    unsigned char rollWater = *me - 1;

                    if (rollWater < CH_WATERFLOW_0)
                        rollWater = CH_WATERFLOW_4;

                    // flag enables slow leading edge
                    *next = FLAG(rollWater);

                } else

                    // Water has hit something below
                    nDots(3, col, row, PT_TWO + PARTICLE_GRAVITY_FLAG, 40, 2 + rangeRandom(3), 11, 100, 7);
#if ENABLE_SHAKE
                // setShake(20);
#endif
            }
        }
    }
}

void processCharBeltAndGrinder(unsigned char *me, unsigned char creature) {


    if (creature == CH_GRINDER_1)
        conveyorDirection = 1;
    else if (creature == CH_GRINDER_0)
        conveyorDirection = -1;

    unsigned char *up = me - _BOARD_COLS;
    unsigned char *up2 = me - _BOARD_COLS + conveyorDirection;

    if (ATTRIBUTE_BIT(*up, ATT_CONVEYOR) && (ATTRIBUTE_BIT(*up2, ATT_BLANK | ATT_DISSOLVES)) && *up < FLAG_THISFRAME) {
        *up2 = FLAG(*up);
        *up = CH_DUST_0;
    }
}

void processCharGeoDogeAndRock(unsigned char *me) {

    unsigned char *next = me + _BOARD_COLS * gravity;
    enum ObjectType typeDown = CharToType[GET(*next)];

    if (Attribute[typeDown] & ATT_BLANK) {

        if (ATTRIBUTE_BIT(*me, ATT_GEODOGE)) {
            *next = FLAG(CH_GEODOGE_FALLING_BOTTOM);
            *me = FLAG(CH_GEODOGE_FALLING_TOP);
        } else {
            *next = FLAG(CH_ROCK_FALLING_BOTTOM);
            *me = FLAG(CH_ROCK_FALLING_TOP);
        }
    }
}

void processFallingThings(unsigned char *me, int row, int col, unsigned char creature) {

    unsigned char *next = me + _BOARD_COLS * gravity;
    enum ObjectType typeDown = CharToType[GET(*next)];
    if (Attribute[typeDown] & ATT_BLANK) {

        switch (creature) {

        case CH_GEODOGE_FALLING:
            *me = FLAG(CH_GEODOGE_FALLING_TOP);
            *next = FLAG(CH_GEODOGE_FALLING_BOTTOM);
            break;

        case CH_DOGE_FALLING:
            *me = FLAG(CH_DOGE_FALLING_TOP);
            *next = FLAG(CH_DOGE_FALLING_BOTTOM);
            break;

        case CH_ROCK_FALLING:
            *me = FLAG(CH_ROCK_FALLING_TOP);
            *next = FLAG(CH_ROCK_FALLING_BOTTOM);
            break;
        }

        unsigned char *nextNext = next + _BOARD_COLS * gravity;
        enum ChName downCh = GET(*nextNext);
        typeDown = CharToType[downCh];
        const unsigned int attNextNext = Attribute[typeDown];

        if (downCh != CH_ROCK_FALLING && downCh != CH_DOGE_FALLING && downCh != CH_GEODOGE_FALLING) {

            int sfx = 0;

            if (attNextNext & ATT_HARD) {
                if (creature == CH_ROCK_FALLING || creature == CH_GEODOGE_FALLING) {

                    sfx = SFX_ROCK;

                    unsigned char *dL = me + _BOARD_COLS - 1;
                    unsigned char *dR = dL + 2;

                    if (!CharToType[GET(*dR)]) {
                        nDots(4, col, row + 1, PT_SPIRAL, 10, 3, 7, 100, 2);
                    }

                    if (!CharToType[GET(*dL)]) {
                        nDots(4, col, row + 1, PT_SPIRAL, 10, 3, 7, 100, 2);
                    }
                }
            }

            if (sfx)
                ADDAUDIO(sfx);
        }
    }

    else if (Attribute[typeDown] & ATT_SQUASHABLE_TO_BLANKS) {

        if (creature == CH_DOGE_FALLING) {
            *me = FLAG(CH_BLANK);
            pulsePlayerColour = 5;
            grabDoge();
            nDots(6, col, row + 1, PT_TWO, 40, 3, 1, 100, 7);
        }

        else {
            explode(next, FLAG(CH_DUST_0));
            initParticles();
        }
    } else {

        // stop falling
        unsigned char sfx = 0;
        int att = ATTRIBUTE(*next);

        switch (creature) {

        case CH_ROCK_FALLING: {
            *me = CH_ROCK;
            sfx = att & ATT_HARD ? SFX_ROCK : SFX_ROCK2;
            break;
        }

        case CH_GEODOGE_FALLING: {
            *me = CH_GEODOGE;
            break;
        }

        case CH_DOGE_FALLING: {
            *me = CH_DOGE_00;
            sfx = SFX_DOGE;
            break;
        }
        }

        if (creature != CH_DOGE_FALLING && CharToType[creature] != TYPE_GEODOGE_FALLING)
            nDots(6, col, row, PT_TWO, 20, 2, 10, 60, 7);

        if (sfx && !(att & ATT_NOROCKNOISE))
            ADDAUDIO(sfx);
    }
}

void genericPush(unsigned char *me, int row, int col, int offsetX, int offsetY) {

    bool atEdge = (col < 3) || (col > 36) || (row < 3) || (row > 18);
    unsigned char *playerPos = RAM + _BOARD + playerY * _BOARD_COLS + playerX;

    int adjustOffset = offsetY * _BOARD_COLS + offsetX;
    unsigned char *pushPos = me + adjustOffset;

    unsigned char alternate = CH_STEELWALL;
    unsigned char *pushPosFurther = atEdge ? &alternate : pushPos + adjustOffset;
    const unsigned int attPushPosFurther = Attribute[CharToType[GET(*pushPosFurther)]];

    //??
    if (playerPos == pushPos && (atEdge || !(attPushPosFurther & ATT_PERMEABLE))) {
        FLASH(0x42, 8);
        startPlayerAnimation(ID_Xray);
        nDots(6, col + offsetX, row + offsetY, PT_TWO, 50, 3, 4, 0x180, 7);
    }

    const unsigned int attPushPos = Attribute[CharToType[GET(*pushPos)]];

    if (GET(*pushPos) < FLAG_THISFRAME) {
        if ((attPushPos & ATT_PERMEABLE) || ((attPushPos & ATT_PUSH) && (attPushPosFurther & ATT_PERMEABLE))) {

            // Note we may have a lagging flag clear until next frame but who cares

            if (attPushPosFurther & ATT_PERMEABLE)
                *pushPosFurther = FLAG(*pushPos);

            *pushPos = FLAG(*me);
            *me = offsetX ? CH_HORIZONTAL_BAR : CH_VERTICAL_BAR;

            if (playerPos == pushPos) {

                pulsePlayerColour = 20;
                ADDAUDIO(SFX_EXPLODE_QUIET);

                if (attPushPosFurther & ATT_BLANK) {

                    autoMoveY = 0;
                    autoMoveY = 0;
                    autoMoveFrameCount = 0;
                    playerX += offsetX;
                    playerY += offsetY;
                }
            }

            //??
            if (!(attPushPos & ATT_PERMEABLE))
                nDots(6, col + offsetX, row + offsetY, PT_TWO, 150, 3, 4, 0x100, 7);
            return;
        }
    }

    *me = (*me) + 1;    // reverse
}

void genericPushReverse(unsigned char *me, int offsetX, int offsetY) {

    unsigned char *pushPos = me + offsetY * _BOARD_COLS + offsetX;
    enum ObjectType pushType = CharToType[GET(*pushPos)];

    if (pushType == TYPE_PUSHER) {
        *pushPos = FLAG(*me);
        *me = CH_BLANK;
    } else
        *me = (*me) - 1;
}


const unsigned char thisFrame[] = {0, FLAG_THISFRAME, FLAG_THISFRAME, 0};

void chainReact_GeoDogeToDoge(unsigned char *me) {


    bool ongoing = false;
    *me = FLAG(CH_DOGE_00);

    for (int i = 0; i < 4; i++) {

        unsigned char *newDogeCandidate = me + dirOffset[i];

        if (Attribute[CharToType[GET(*newDogeCandidate)]] & ATT_GEODOGE) {

            *newDogeCandidate = CH_CONVERT_GEODE_TO_DOGE | thisFrame[i];
            ADDAUDIO(SFX_UNCOVER);
            ongoing = true;
        }
    }

    if (!ongoing)
        killAudio(SFX_UNCOVER);
}

void chainReact_Pipe(unsigned char *me) {

    bool ongoing = false;
    *me = FLAG(CH_DUST_0);

    for (int i = 0; i < 4; i++) {

        unsigned char *newDogeCandidate = me + dirOffset[i];

        if (ATTRIBUTE_BIT(*newDogeCandidate, ATT_PIPE)) {

            *newDogeCandidate = CH_CONVERT_PIPE | thisFrame[i];
            ADDAUDIO(SFX_UNCOVER);
            ongoing = true;
        }
    }

    if (!ongoing)
        killAudio(SFX_UNCOVER);
}


void doRoll(unsigned char *me, int row, int col) {

    for (int offset = -1; offset < 2; offset += 2) {

        unsigned char *side = me + offset;
        if (*side < FLAG_THISFRAME) {

            enum ObjectType sideType = CharToType[GET(*side)];
            if (Attribute[sideType] & ATT_BLANK) {

                unsigned char *sideDown = side + gravity * _BOARD_COLS;
                enum ObjectType sideDownType = CharToType[GET(*sideDown)];
                if (Attribute[sideDownType] & ATT_BLANK) {

                    if (offset > 0) {
                        *me = FLAG(CH_DOGE_SIDE_1);
                        *(me + offset) = FLAG(CH_DOGE_SIDE_3);

                    } else {
                        *me = FLAG(CH_DOGE_SIDE_2);
                        *(me + offset) = FLAG(CH_DOGE_SIDE_4);
                    }

                    *(sideDown) = FLAG(CH_BLANK);

                    int off = offset < 0 ? 4 : 0;

                    nDots(1, col, row, PT_TWO, 15, offset * 2 + off, 4 * gravity, 0, 1);
                    nDots(1, col, row, PT_TWO, 20, offset * 4 + off, 4 * gravity, 0, 1);
                    nDots(1, col, row, PT_TWO, 25, offset * 6 + off, 7 * gravity, 0, 1);
                    nDots(1, col, row, PT_TWO, 30, offset * 7 + off, 10 * gravity, 0, 1);

                    return;
                }
            }
        }
    }
}


void explode(unsigned char *where, unsigned char explosionShape) {

    ADDAUDIO(SFX_EXPLODE);

    int offset[] = {-_BOARD_COLS - 1, -_BOARD_COLS, -_BOARD_COLS + 1, -1, 1,
                    _BOARD_COLS - 1,  _BOARD_COLS,  _BOARD_COLS + 1,  0};
    int shape[] = {CH_BLANK, explosionShape, CH_BLANK, explosionShape, explosionShape,
                   CH_BLANK, explosionShape, CH_BLANK, explosionShape};


    for (int i = 0; i < (int)(sizeof(offset) / sizeof(offset[0])); i++) {
        unsigned char *cell = where + offset[i];
        enum ObjectType cellType = CharToType[GET(*cell)];
        if (Attribute[cellType] & ATT_EXPLODABLE) {

            bool wasDoge = (cellType == TYPE_DOGE);
            bool becomesDoge = (shape[i] == CH_DOGE_00);

            if (wasDoge && !becomesDoge)
                totalDogePossible--;    // a doge sitting here is being destroyed
            else if (!wasDoge && becomesDoge)
                totalDogePossible++;    // this cell is being turned into a new doge
            // else: doge->doge (no change) or non-doge->non-doge (nothing to count)

            *cell = shape[i];
        }
    }

    FLASH(4, 4);
}


// EOF
