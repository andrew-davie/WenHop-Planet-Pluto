#include <stdbool.h>

#include "decodecaves.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "animations.h"
#include "attribute.h"
#include "board.h"
#include "caveData.h"
#include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "player.h"
#include "random.h"
#include "schedule.h"
#include "score.h"
#include "scroll.h"
#include "sound.h"
#include "wyrm.h"

#define PARTICLE_GRAVITY_FLAG 0x80


// must be init'd at startup
int selectorCounter;
int waterDir;
int explodeCount;
int explodeRadius;

// init'd locally
unsigned char creature;
enum ObjectType type;
int conveyorDirection;
int activeStar;
int lastActiveStar;
bool single;

void convertWaterAndLavaObjects();
void processTypes();
void processCreatures();
void restartBoardScan();

void processDoge();
void processPebble();
void processWater();
// void processLava();
void processWaterFlow();
void processCharBeltAndGrinder();
void processOutlet();
void processFallingThings();
void processCharGeoDogeAndRock();

void genericPush(int offsetX, int offsetY);
void genericPushReverse(int offsetX, int offsetY);
void chainReact_GeoDogeToDoge();
void chainReact_Pipe();
void doRoll();
void setInsulator(unsigned char *p, bool active);

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
    int eX = sX + 40;
    int x = (sX * (0x10000 / CHAR_TRIX_X)) >> 16;

    for (int i = sX; i < eX; i += CHAR_TRIX_X)
        onScreenX[x++] = true;

    int sY = scrollY >> 16;
    int eY = sY + _SCANLINES / 3;
    int y = (sY * (0x10000 / CHAR_TRIX_Y)) >> 16;

    for (int i = sY; i < eY; i += CHAR_TRIX_Y)
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
    conveyorDirection = -1;    // ?
    activeStar = lastActiveStar = 0;
    single = false;
}


void setupBoardScanner() {

    // After board scan complete, throttles until we're at correct FPS

    if (gameFrame >= gameSpeed) {    // && !autoMoveFrameCount) {

        //        doges = gameFrame;

        waterDir++;

        restartBoardScan();

        processWyrms();

        gameFrame = 0;
        gameTick++;

        lastActiveStar = activeStar;
        activeStar = 0;

        if (!single && !totalDogePossible) {
            single = true;
            FLASH(0xC8, 10);
            startCharAnimation(TYPE_STAR, AnimateStar);
        }

        // if (!totalDogePossible)
        //     FLASH(0xC2, 10);


        ++selectorCounter;
        //        activated = isActive[++selectorCounter & 3];

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

                if (((sinus[(waves >> 0) & 15] & 3) != 0) || (gameFrame & 31) == 0) {
                    ++waves;

                    //                  if (!(frame & 65535))
                    //                        lavaSurfaceTrixel--;
                }

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

        if (boardRow < 0) {
            boardRow = _BOARD_ROWS - 1;
            boardCol = _BOARD_COLS - 1;
        } else {
            boardCol = 0;
            boardRow = 0;
        }

        calculateVisibleMasks();

        setSchedule(SCHEDULE_PROCESS_BOARD);

        processBoardSquares();
    }
}


void processBoardSquares() {


    while (T1TC < availableIdleTime) {

        me = RAM + _BOARD + boardRow * _1ROW + boardCol;

        creature = *me;

        if (!(creature & FLAG_THISFRAME)) {

            type = CharToType[creature];

            if (isVisible(boardCol, boardRow))
                convertWaterAndLavaObjects();

            if (Attribute[type] & isActive[selectorCounter & 3]) {
                processTypes();
                processCreatures();
            }
        }


        // Clear any "scanned this frame" objects on the previous line
        // note: we need to also do the last row ... or do we? if it's steel wall, no

        if (gravity > 0) {
            if (boardRow > 1) {
                unsigned char *prev = me - _1ROW;
                *prev &= ~FLAG_THISFRAME;
            }
        }

        else {
            if (boardRow < _BOARD_ROWS - 1) {
                unsigned char *prev = me + _1ROW;
                *prev &= ~FLAG_THISFRAME;
            }
        }

        me += gravity;

        boardCol += gravity;

        if (gravity < 0) {
            if (boardCol < 0) {
                boardCol = _BOARD_COLS - 1;

                if (!--boardRow) {
                    setSchedule(SCHEDULE_START_SCAN);
                    //                    restartBoardScan();
                    return;
                }
            }
        }

        else {
            if (boardCol > (_BOARD_COLS - 1)) {
                boardCol = 0;
                if (++boardRow > _BOARD_ROWS - 1) {
                    setSchedule(SCHEDULE_START_SCAN);
                    //                    restartBoardScan();
                    return;
                }
            }
        }
    }
}


void convertWaterAndLavaObjects() {

    // Any blanks/dissolves underwater/lava gets transformed to water/lava chars

    if (boardRow * CHAR_TRIX_Y >= lavaSurfaceTrixel && Attribute[type] & ATT_CONVERT)
        *me = showLava ? CH_LAVA_BLANK : CH_WATER;
}


void processTypes() {

    switch (type) {


    case TYPE_STAR_EXPLODE: {

        activeStar++;
        if (*Animate[TYPE_STAR_EXPLODE] == CH_DUST_0) {

            ADDAUDIO(SFX_EXPLODE);
            // FLASH(0x26, 8);
            nDots(20, boardCol, boardRow, PT_TWO, 25, CHAR_CENTER_X, CHAR_CENTER_Y, 80, 7);
            *me = FLAG(CH_DUST_0);
        }

        else {
            nDots(1, boardCol, boardRow, PT_TWO, 25, CHAR_CENTER_X, CHAR_CENTER_Y, 50, 5);
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
                // ADDAUDIO(SFX_DOGE3);
            }

            if (--delay < 0) {
                *me = FLAG(CH_STAR_EXPLODE);
                startCharAnimation(TYPE_STAR_EXPLODE, AnimateStarExplode);
                delay = 30;
                break;
            }
        }
        // fall

        unsigned char *next = me + _1ROW;
        if (Attribute[CharToType[GET(*next)]] & ATT_BLANK) {
            *next = FLAG(CH_STAR_FALLING_BOTTOM);
            *me = FLAG(CH_STAR_FALLING_TOP);

            nDots(5, boardCol, boardRow, PT_TWO, 40, CHAR_CENTER_X, CHAR_CENTER_Y, 40, 2);
        }


        break;
    }

    case TYPE_ELECTRIC: {
        // if (!rangeRandom(10))
        //     *me = CH_ELECTRIC_0 + rangeRandom(4);

        // if (Attribute[CharToType[GET(*(me + _1ROW))]] & (ATT_DISSOLVES | ATT_BLANK))
        //     *(me + _1ROW) = CH_ELECTRIC_0 + rangeRandom(4);

        // if (Attribute[CharToType[GET(*(me - _1ROW))]] & (ATT_DISSOLVES | ATT_BLANK))
        //     *(me - _1ROW) = CH_ELECTRIC_0 + rangeRandom(4);

        *me = CH_ELECTRIC_0 + ((*me + 1 - CH_ELECTRIC_0) & 3);
    } break;

    case TYPE_OUTBOX:
        FLASH(0x28, 10);
        nDots(10, boardCol, boardRow, PT_SPIRAL, 40, 2, 5, 0x40, 7);    // untested speed
        break;

    case TYPE_DOGE:
        processDoge();
        break;

    case TYPE_PEBBLE1:
        processPebble();
        break;

    case TYPE_WATER:
        processWater();
        break;

        // case TYPE_LAVA:
        //     processLava();
        //     break;

    case TYPE_MELLON_HUSK:

        if (!exitMode)
            movePlayer(me);
        break;

    case TYPE_WATERFLOW_0:
    case TYPE_WATERFLOW_1:
    case TYPE_WATERFLOW_2:
    case TYPE_WATERFLOW_3:
    case TYPE_WATERFLOW_4:
        processWaterFlow();
        break;

    case TYPE_GRINDER:
    case TYPE_GRINDER_1:
        if (!(getRandom32() & 7)) {
            nDots(1, boardCol, boardRow, PT_TWO, 10, 3, 7, 100, 7);
        }

        __attribute__((fallthrough));

    case TYPE_BELT:
    case TYPE_BELT_1:
        processCharBeltAndGrinder();
        break;

    case TYPE_GEODOGE:

        if ((!(theCave->flags & CAVEDEF_STAR_STATIC)) && !rangeRandom(250)) {
            *me = FLAG(CH_ROCK_PEBBLE_1);
            // FLASH(0x42, 10);
            //  nDotsBackwards(10, boardCol, boardRow, PT_TWO, 25, 2, 5, 300);
            nDots(10, boardCol, boardRow, PT_TWO, 10, 3, 7, 100, 7);
        }

        // else if (!rangeRandom(3)) {

        //     static const int xy[] = {1, -1, _1ROW, -_1ROW};
        //     unsigned char *d = me + xy[getRandom32() & 3];
        //     if (GET(*d) == CH_DIRT)
        //         *d = rangeRandom(2) + CH_PEBBLE1;
        // }

        else
            processCharGeoDogeAndRock();

        surroundingConglomerate(boardCol, boardRow);
        break;

    default:
        break;
    }
}

// void electric(unsigned char *p, unsigned char ch) {

//     p -= _1ROW;
//     while (CharToType[GET(*p)] != TYPE_INSULATOR) {
//         *p = ch;
//         p -= _1ROW;
//     }
// }


void setInsulator(unsigned char *p, bool active) {

    // ADDAUDIO(SFX_ZAP);
    // ADDAUDIO(SFX_ZAP2);

    int dir = _1ROW;
    p += dir;


    if (!active) {
        while (CharToType[GET(*p)] != TYPE_INSULATOR) {
            if (CharToType[GET(*p)] == TYPE_ELECTRIC) {
                *p = FLAG(CH_BLANK);
                // break;
            }
            p += dir;
        }
    }

    else {

        int x = boardCol;
        int y = boardRow + 1;

        while (CharToType[GET(*p)] != TYPE_INSULATOR) {
            unsigned int att = Attribute[CharToType[GET(*p)]];
            if (att & (ATT_EXPLODABLE | ATT_GEODOGE | ATT_DISSOLVES | ATT_BLANK)) {

                // if (att & ATT_EXPLODABLE && !(att & ATT_BLANK))
                //     FLASH(0x28, 10);

                *p = FLAG(CH_ELECTRIC_0 + rangeRandom(4));
                nDots(6, x, y, PT_SPIRAL, 10 + rangeRandom(10), CHAR_CENTER_X, 0, 40, 7);
                break;
            }
            p += dir;
            y++;
        }
    }
}


// void disableInsulator(unsigned char *p) {

//     ADDAUDIO(SFX_ZAP);
//     ADDAUDIO(SFX_ZAP2);

//     // FLASH(0x2C, 10);

//     //    bool disabled = false;
//     int dir = (GET(*p) == CH_INSULATOR_TOP) ? _1ROW : -_1ROW;
//     p += dir;

//     if (CharToType[GET(*p)] == TYPE_ELECTRIC) {
//         while (CharToType[GET(*p)] == TYPE_ELECTRIC) {
//             //          disabled = true;
//             *p = CH_BLANK;
//             p += dir;
//         }
//     }

//     else {

//         // while (CharToType[GET(*p)] == TYPE_BLANK)
//         //     p += dir;

//         if (Attribute[CharToType[GET(*p)]] & (ATT_DISSOLVES | ATT_BLANK))
//             *p = CH_ELECTRIC_0 + rangeRandom(4);

//         else
//             FLASH(0x44, 2);
//     }
// }


bool onOff[_BOARD_COLS] = {true};

void setInsulatorPattern() {

    static int s = 0;
    s++;


    for (int i = 0; i < _BOARD_COLS; i++) {

        bool oc = sin_cos[(s + i * 20) & 31] < 128;

        if (oc && !onOff[i]) {
            // ADDAUDIO(SFX_ZAP);
            // ADDAUDIO(SFX_ZAP2);
        }
        onOff[i] = oc;
    }
}


void processCreatures() {

    switch (creature) {

    case CH_INSULATOR_TOP:
        setInsulator(me, onOff[boardCol]);
        break;

    case CH_OUTLET:
        processOutlet();
        break;

    case CH_ROCK_PEBBLE_1:
        *me = FLAG(CH_ROCK_PEBBLE);
        // shakeTime += 5;
        break;

    case CH_ROCK_PEBBLE:
        *me = FLAG(CH_BLANK);
        break;

    case CH_PEBBLE_ROCK:
        *me = FLAG(CH_GEODOGE);
        surroundingConglomerate(boardCol, boardRow);
        break;

    case CH_PUSH_LEFT:
        genericPush(-1, 0);
        break;

    case CH_PUSH_LEFT_REVERSE:
        genericPushReverse(1, 0);
        break;

    case CH_PUSH_RIGHT:
        genericPush(1, 0);
        break;

    case CH_PUSH_RIGHT_REVERSE:
        genericPushReverse(-1, 0);
        break;

    case CH_PUSH_UP:
        genericPush(0, -1);
        break;

    case CH_PUSH_UP_REVERSE:
        genericPushReverse(0, 1);
        break;

    case CH_PUSH_DOWN:
        genericPush(0, 1);
        break;

    case CH_PUSH_DOWN_REVERSE:
        genericPushReverse(0, -1);
        break;

    case CH_DUST_2:
    case CH_DUST_ROCK_2:
        *me = CH_BLANK;
        break;

    case CH_DUST_0:
    case CH_DUST_1:
    case CH_DUST_ROCK_0:
    case CH_DUST_ROCK_1:
        (*me)++;
        break;

        // case CH_EXPLODETOBLANK_0:
        // case CH_EXPLODETOBLANK_1:
        // case CH_EXPLODETOBLANK_2:
        // case CH_EXPLODETOBLANK_3:
        //     *me = FLAG(creature + 1);
        //     break;

        // case CH_EXPLODETOBLANK_4:
        //     *me = FLAG(CH_BLANK);
        //     break;

    case CH_CONVERT_GEODE_TO_DOGE:
        *me = FLAG(CH_DOGE_00);
        chainReact_GeoDogeToDoge();
        break;

    case CH_CONVERT_PIPE:
        chainReact_Pipe();
        break;

    case CH_BLOCK: {

        unsigned char *next = me + _1ROW * gravity;
        enum ObjectType typeDown = CharToType[GET(*next)];
        if (Attribute[typeDown] & ATT_BLANK) {
            *next = FLAG(*me);
            *me = FLAG(CH_DUST_0);
        }
        break;
    }

    case CH_ROCK_BONUS: {

        unsigned char *next = me + _1ROW * gravity;
        if (Attribute[CharToType[GET(*next)]] & ATT_BLANK) {
            *next = FLAG(*me);
            *me = FLAG(CH_BLANK);
            startCharAnimation(TYPE_ROCK_BONUS, AnimateRockBonus + 2);
        }

        break;
    }


    case CH_STAR_FALLING_TOP:
        *me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_STAR_FALLING_BOTTOM:
        *me = FLAG(CH_STAR);
        break;


    case CH_ROCK:
        processCharGeoDogeAndRock();
        break;

    case CH_ROCK_FALLING_TOP:
        *me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_GEODOGE_FALLING_TOP:

        nDots(3, boardCol, boardRow, PT_TWO, 15, rangeRandom(CHAR_TRIX_X), rangeRandom(CHAR_TRIX_Y), 0, 2);

        *me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_DOGE_SIDE_1:
    case CH_DOGE_SIDE_2:
        *me = FLAG(CH_BLANK);
        break;

    case CH_DOGE_SIDE_3:
    case CH_DOGE_SIDE_4:

        *me = FLAG(CH_DOGE_FALLING_TOP);
        *(me + gravity * _1ROW) = FLAG(CH_DOGE_FALLING_BOTTOM);
        break;

    case CH_DOGE_FALLING_TOP2:
        *me = FLAG(CH_DOGE_FALLING_TOP);
        *(me + gravity * _1ROW) = FLAG(CH_DOGE_FALLING_BOTTOM);
        break;

    case CH_DOGE_FALLING_TOP:
        *me = FLAG(CH_BLANK);
        break;

    case CH_DOGE_FALLING_BOTTOM:
        *me = FLAG(CH_DOGE_FALLING);
        break;

    case CH_ROCK_FALLING_BOTTOM:
        *me = FLAG(CH_ROCK_FALLING);
        break;

    case CH_GEODOGE_FALLING_BOTTOM:
        *me = FLAG(CH_GEODOGE_FALLING);
        break;

    case CH_DOGE_FALLING:
    case CH_ROCK_FALLING:
    case CH_GEODOGE_FALLING: {

        processFallingThings();
        break;
    }

    case CH_DOORCLOSED:
        if (!doges) {
            *me = CH_DOOROPEN_0;
            FLASH(0x28, 10);
        }
        break;

    case CH_DOOROPEN_0:
        FLASH(0xC4, 10);
        nDots(2, boardCol, boardRow, PT_TWO, 10, 3, 7, 100, 7);
        break;


    case CH_MELLON_HUSK_BIRTH:

        if (
#if ENABLE_SWIPE
            checkSwipeFinished() &&
#endif
            (!isScrolling())) {
            *me = CH_MELLON_HUSK;
        }
        break;

    default:
        break;
    }
}


void restartBoardScan() {

    // // randomly place a new pebble
    if (rangeRandom(256) < theCave->millingTime) {


        int dX = playerX + rangeRandom(11) - 5;
        int dY = playerY + rangeRandom(11) - 5;

        if (dX >= 0 && dX < _BOARD_COLS && dY >= 0 && dY < _BOARD_ROWS) {
            unsigned char *sq = RAM + _BOARD + dY * _1ROW + dX;
            if (GET(*sq) == CH_DIRT)
                *sq = FLAG(CH_PEBBLE1 + (getRandom32() & 1));
        }
    }


    if (rangeRandom(256) < theCave->weather)
        setShake(rangeRandom(20));

    // Change insulator pattern

    setInsulatorPattern();


    if (!autoMoveFrameCount) {    // delay until fully in new square


        //                    chooseIdleAnimation();

        bool oldDead = playerDead;

        unsigned char *man = RAM + _BOARD + playerY * _1ROW + playerX;

        enum ChName what = GET(*man);

        if ((usableSWCHA & 0xF0) == 0xF0)
            waitRelease = false;

        enum ObjectType type = CharToType[what];
        playerDead = (type != TYPE_MELLON_HUSK && type != TYPE_MELLON_HUSK_PRE && !exitMode);


        if (oldDead != playerDead) {

            explodeCount = 6;
            explodeRadius = 10;

            initParticles();

            for (int i = 0; i < 10; i++)
                nDots(1, playerX, playerY, PT_TWO, 30 + rangeRandom(10), CHAR_CENTER_X, CHAR_CENTER_Y + 2, 30, 3);

            startPlayerAnimation(ID_Die);
            waitRelease = true;
            lives--;
            // lockDisplay = false;

            sound_max_volume = VOLUME_NONPLAYING;
        }

        if (explodeCount > 0) {

            explodeRadius++;

            for (int i = 0; i < PARTICLE_COUNT / 2; i++)
                nDots(1, playerX, playerY, PT_TWO, rangeRandom(20),
                      rangeRandom(explodeRadius) - (explodeRadius >> 1) + CHAR_CENTER_X,
                      rangeRandom(explodeRadius * 2) - explodeRadius + CHAR_CENTER_Y, rangeRandom(10),
                      rangeRandom(2) + 2);
            --explodeCount;
        }
    }
}


void processDoge() {

    unsigned char *next = me + _1ROW * gravity;
    const unsigned int attrNext = Attribute[CharToType[GET(*next)]];

    if (attrNext & ATT_BLANK) {
        *me = FLAG(CH_DOGE_FALLING_TOP);
        *next = FLAG(CH_DOGE_FALLING_BOTTOM);
    }

    else if (attrNext & ATT_ROLL)
        doRoll();
}


void processPebble() {

    // don't form way above player (but DO form 1 above)
    if (boardCol == playerX && boardRow < playerY - 1)
        return;

    int chance = 250;

    for (int i = 0; i < 4; i++)
        if (TYPEOF(*(me + dirOffset[i])) == TYPE_MELLON_HUSK) {
            chance = 50;
            break;
        }

    if (!rangeRandom((chance))) {
        *me = FLAG(CH_PEBBLE_ROCK);
        nDots(10, boardCol, boardRow, PT_SPIRAL, 20, 2, 5, 0x40, 7);
    }
}

void processWater() {

    if ((boardRow - 1) * CHAR_TRIX_Y >= lavaSurfaceTrixel) {
        unsigned char *neighbour = me + dirOffset[waterDir & 3];
        if (Attribute[CharToType[GET(*neighbour)]] & ATT_DISSOLVES) {
            // FLASH(0x44, 2);
            *neighbour = CH_DUST_0;
        }
    }
}

void processLava() {

    if (isVisible(boardCol, boardRow)) {

        int rand = getRandom32();
        if ((boardRow - 1) * CHAR_TRIX_Y >= lavaSurfaceTrixel) {

            int i = waterDir & 3;

            unsigned char *neighbour = me + dirOffset[i];
            const unsigned int att = Attribute[CharToType[GET(*neighbour)]];

            if (att & ATT_DISSOLVES) {
                *neighbour = CH_LAVA_BLANK;
            }

            else if (att & ATT_MELTS && !(rand & 63)) {

                *neighbour = FLAG((att & ATT_GEODOGE) ? CH_DOGE_00 : CH_DUST_0);
                nDots(8, boardCol + xdir[i], boardRow + ydir[i], PT_TWO, 50, 3, 4, 0x100, 7);
            }
        }

        // Animate lava bubble
        switch (*me) {
        case CH_LAVA_BLANK: {
            if (!(rand & (15 << 7))) {
                (*me)++;
                nDots(3, boardCol, boardRow, PT_SPIRAL, 30, 2, 5, 0xC0, 7);
            }
        } break;
        case CH_LAVA_SMALL:
            *me = CH_LAVA_MEDIUM;
            break;
        case CH_LAVA_MEDIUM:
            *me = CH_LAVA_BLANK;
            break;
        case CH_LAVA_LARGE:
            *me = CH_LAVA_BLANK;
            break;
        }
    }
}

void processWaterFlow() {

    // Lag the interruption of water flowing downwards
    unsigned char above = *(me - _1ROW);
    if (!(above & FLAG_THISFRAME) && !(Attribute[CharToType[GET(above)]] & ATT_WATERFLOW)) {
        *me = FLAG(CH_BLANK);
        return;
    }

    int line = (boardRow + 1) * CHAR_TRIX_Y;
    if (line < lavaSurfaceTrixel) {
        if (boardRow < 20) {

            unsigned char *next = me + _1ROW * gravity;
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
                    nDots(3, boardCol, boardRow, PT_TWO + PARTICLE_GRAVITY_FLAG, 40, 2 + rangeRandom(3), 11, 100, 7);
#if ENABLE_SHAKE
                // setShake(20);
#endif
            }
        }
    }
}

void processCharBeltAndGrinder() {


    if (creature == CH_GRINDER_1)
        conveyorDirection = 1;
    else if (creature == CH_GRINDER_0)
        conveyorDirection = -1;

    unsigned char *up = me - _1ROW;
    unsigned char *up2 = me - _1ROW + conveyorDirection;

    if (ATTRIBUTE_BIT(*up, ATT_CONVEYOR) && (ATTRIBUTE_BIT(*up2, ATT_BLANK | ATT_DISSOLVES)) &&
        !(*up & FLAG_THISFRAME)) {
        *up2 = FLAG(*up);
        *up = CH_DUST_0;
    }
}

void processOutlet() {

    if ((boardRow + 2) * CHAR_TRIX_Y <= lavaSurfaceTrixel) {

        if (GET(*(me - _1ROW * 2)) == CH_TAP_0) {

            enum ChName under = GET(*(me + _1ROW));

            if (!(Attribute[CharToType[under]] & ATT_WATERFLOW)) {

                if (Attribute[CharToType[under]] & ATT_PERMEABLE)
                    *(me + _1ROW) = CH_WATERFLOW_0 + (getRandom32() & 3);

                else
                    nDots(1, boardCol, boardRow, PT_TWO, 30, rangeRandom(3) + 2, 10, 0x80, 7);
            }
        }

        else if (GET(*(me - _1ROW * 2)) == CH_TAP_1) {
            if (Attribute[CharToType[GET(*(me + _1ROW))]] & ATT_BLANK)
                *(me + _1ROW) = CH_BLANK;
        }
    }
}

void processCharGeoDogeAndRock() {

    unsigned char *next = me + _1ROW * gravity;
    enum ObjectType typeDown = CharToType[GET(*next)];
    // int typeofMe = CharToType[GET(*me)];

    if (Attribute[typeDown] & ATT_BLANK) {

        // if (*me == CH_ROCK_BONUS) {
        //     *me = FLAG(CH_STAR);
        //     return;
        // }

        if (ATTRIBUTE_BIT(*me, ATT_GEODOGE)) {
            *next = FLAG(CH_GEODOGE_FALLING_BOTTOM);
            *me = FLAG(CH_GEODOGE_FALLING_TOP);
        } else {
            *next = FLAG(CH_ROCK_FALLING_BOTTOM);
            *me = FLAG(CH_ROCK_FALLING_TOP);
        }

        surroundingConglomerate(boardCol, boardRow);
    }

    else if (Attribute[typeDown] & ATT_ROLL) {

        // TODO: allow rock to roll in earthquake or on gear

        //     if (shakeTime && !rangeRandom(20)) {
        //         if (ATTRIBUTE_BIT(*me, ATT_GEODOGE))
        //             doRoll(me, CH_GEODOGE_FALLING);
        //         else {

        //             doRoll(me, CH_ROCK_FALLING);
        //         }
        // }
    }
}

void processFallingThings() {

    unsigned char *next = me + _1ROW * gravity;
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

        unsigned char *nextNext = next + _1ROW * gravity;
        enum ChName downCh = GET(*nextNext);
        typeDown = CharToType[downCh];
        const unsigned int attNextNext = Attribute[typeDown];

        if (downCh != CH_ROCK_FALLING && downCh != CH_DOGE_FALLING && downCh != CH_GEODOGE_FALLING) {

            int sfx = 0;

            // if (!(attNextNext & ATT_NOROCKNOISE)) {

            //     if (creature == CH_ROCK_FALLING)
            //         sfx = attNextNext & ATT_HARD ? SFX_ROCK :
            //         SFX_ROCK2;
            //     else
            //         sfx = SFX_DOGE;
            // }

            if (attNextNext & ATT_HARD) {
                if (creature == CH_ROCK_FALLING || creature == CH_GEODOGE_FALLING) {

                    sfx = SFX_ROCK;

                    unsigned char *dL = me + _1ROW - 1;
                    unsigned char *dR = dL + 2;

                    if (!CharToType[GET(*dR)]) {
                        nDots(4, boardCol, boardRow + 1, PT_SPIRAL, 10, 3, 7, 100, 2);
                    }

                    if (!CharToType[GET(*dL)]) {
                        nDots(4, boardCol, boardRow + 1, PT_SPIRAL, 10, 3, 7, 100, 2);
                    }
                }
                // else
                //     sfx = SFX_DOGE;
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
            nDots(6, boardCol, boardRow + 1, PT_TWO, 40, 3, 1, 100, 7);
        }

        else {
            explode(next, FLAG(CH_DUST_0));
            initParticles();
            //            nDots(PARTICLE_COUNT, boardCol, boardRow + 1, PT_TWO, 100, CHAR_CENTER_X, CHAR_CENTER_Y,
            //            50, 7);
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
            surroundingConglomerate(boardCol, boardRow);
            break;
        }

        case CH_DOGE_FALLING: {
            *me = CH_DOGE_00;
            sfx = SFX_DOGE;
            break;
        }
        }

        if (creature != CH_DOGE_FALLING && CharToType[creature] != TYPE_GEODOGE_FALLING)
            nDots(6, boardCol, boardRow, PT_TWO, 20, 2, 10, 60, 7);

        // if (att & ATT_ROLL && creature == CH_DOGE_FALLING)
        //     doRoll(me, creature);

        if (sfx && !(att & ATT_NOROCKNOISE))
            ADDAUDIO(sfx);
    }
}

void genericPush(int offsetX, int offsetY) {

    bool atEdge = (boardCol < 3) || (boardCol > 36) || (boardRow < 3) || (boardRow > 18);
    unsigned char *playerPos = RAM + _BOARD + playerY * _1ROW + playerX;

    int adjustOffset = offsetY * _1ROW + offsetX;
    unsigned char *pushPos = me + adjustOffset;

    unsigned char alternate = CH_STEELWALL;
    unsigned char *pushPosFurther = atEdge ? &alternate : pushPos + adjustOffset;
    const unsigned int attPushPosFurther = Attribute[CharToType[GET(*pushPosFurther)]];

    //??
    if (playerPos == pushPos && (atEdge || !(attPushPosFurther & ATT_PERMEABLE))) {
        // setShake(20);
        FLASH(0x42, 8);
        startPlayerAnimation(ID_Xray);
        nDots(6, boardCol + offsetX, boardRow + offsetY, PT_TWO, 50, 3, 4, 0x180, 7);
    }

    const unsigned int attPushPos = Attribute[CharToType[GET(*pushPos)]];

    if (!(GET(*pushPos) & FLAG_THISFRAME)) {
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

            surroundingConglomerate(boardCol, boardRow);
            surroundingConglomerate(boardCol + offsetX, boardRow + offsetY);

            //??
            if (!(attPushPos & ATT_PERMEABLE))
                nDots(6, boardCol + offsetX, boardRow + offsetY, PT_TWO, 150, 3, 4, 0x100, 7);
            return;
        }
    }

    *me = (*me) + 1;    // reverse
}

void genericPushReverse(int offsetX, int offsetY) {

    unsigned char *pushPos = me + offsetY * _BOARD_COLS + offsetX;
    enum ObjectType type = CharToType[GET(*pushPos)];

    if (type == TYPE_PUSHER) {
        *pushPos = FLAG(*me);
        *me = CH_BLANK;
    } else
        *me = (*me) - 1;
}


const unsigned char thisFrame[] = {0, FLAG_THISFRAME, FLAG_THISFRAME, 0};

void chainReact_GeoDogeToDoge() {


    bool ongoing = false;
    *me = FLAG(CH_DOGE_00);

    for (int i = 0; i < 4; i++) {

        unsigned char *newDogeCandidate = me + dirOffset[i];

        if (Attribute[CharToType[GET(*newDogeCandidate)]] & ATT_GEODOGE) {

            *newDogeCandidate = CH_CONVERT_GEODE_TO_DOGE | thisFrame[i];
            ADDAUDIO(SFX_UNCOVER);
            ongoing = true;

            surroundingConglomerate(boardCol + xdir[i], boardRow + ydir[i]);
        }
    }

    surroundingConglomerate(boardCol, boardRow);

    if (!ongoing)
        killAudio(SFX_UNCOVER);
}

void chainReact_Pipe() {

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


void doRoll() {

    for (int offset = -1; offset < 2; offset += 2) {

        unsigned char *side = me + offset;
        if (!(*side & FLAG_THISFRAME)) {

            enum ObjectType sideType = CharToType[GET(*side)];
            if (Attribute[sideType] & ATT_BLANK) {

                unsigned char *sideDown = side + gravity * _1ROW;
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

                    nDots(1, boardCol, boardRow, PT_TWO, 15, offset * 2 + off, 4 * gravity, 0, 1);
                    nDots(1, boardCol, boardRow, PT_TWO, 20, offset * 4 + off, 4 * gravity, 0, 1);
                    nDots(1, boardCol, boardRow, PT_TWO, 25, offset * 6 + off, 7 * gravity, 0, 1);
                    nDots(1, boardCol, boardRow, PT_TWO, 30, offset * 7 + off, 10 * gravity, 0, 1);

                    surroundingConglomerate(boardCol, boardRow);
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
        enum ObjectType type = CharToType[GET(*cell)];
        if (Attribute[type] & ATT_EXPLODABLE) {

            if (CharToType[GET(*cell)] == TYPE_DOGE)
                totalDogePossible--;

            *cell = shape[i];
            if (explosionShape == CH_DOGE_00)
                totalDogePossible++;
        }
    }

    FLASH(4, 4);
}


void surroundingConglomerate(int col, int row) {

    if (isVisible(col, row)) {

        //        if (rangeRandom(10))
        //          return;

        unsigned char *pos = RAM + _BOARD + row * _1ROW + col;
        for (int i = 0; i < 5; i++) {

            unsigned char *offsetPos = pos + dirOffset[i];
            if (Attribute[CharToType[GET(*offsetPos)]] & ATT_GEODOGE) {

                int cong = CH_GEODOGE;

                for (int j = 0; j < 4; j++)
                    if (Attribute[CharToType[GET(*(offsetPos + dirOffset[j]))]] & ATT_GEODOGE)
                        cong += 1 << j;

                *offsetPos = cong;
            }
        }
    }
}


// EOF
