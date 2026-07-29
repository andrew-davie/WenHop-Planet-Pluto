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
#include "score.h"
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
static int lastRockCount, bestRockCount;
static int rockCount;

static int lastConvertedGeodoge;
static int convertedGeodoge;


// init'd locally
static int conveyorDirection;
static int activeStar;
static int lastActiveStar;
static bool single;

static BoardCursor cursor;
// cur.row = boardRow;
// cur.col = boardCol;


bool processTypes(BoardCursor *cur, enum ObjectType type, unsigned char creature);
void processCreatures(BoardCursor *cur, unsigned char creature);
void restartBoardScan();

void processPebble(unsigned char *me, int row, int col);
void processWater(unsigned char *me, int row);
void processWaterFlow(unsigned char *me, int row, int col);
void processCharBeltAndGrinder(unsigned char *me, unsigned char creature);
void processFallingThings(unsigned char *me, int row, int col, unsigned char creature);
void processCharRock(unsigned char *me);

void genericPush(unsigned char *me, int row, int col, int offsetX, int offsetY);
void genericPushReverse(unsigned char *me, int offsetX, int offsetY);
void chainReact_Pipe(unsigned char *me);
void doRoll(unsigned char *me, int row, int col);
void setInsulator(unsigned char *p, int row, int col);

//------------------------------------------------------------------------------

// #define isVisible(x, y) (onScreenX[x] && onScreenY[y])

// #define W4 ((_BOARD_COLS + 3) & ~3)
// #define H4 ((_BOARD_ROWS + 3) & ~3)

// bool onScreenX[W4] __attribute__((aligned(4)));
// bool onScreenY[H4] __attribute__((aligned(4)));

// void calculateVisibleMasks() {

//     myMemsetInt((unsigned int *)onScreenX, 0, sizeof(onScreenX) / 4);
//     myMemsetInt((unsigned int *)onScreenY, 0, sizeof(onScreenY) / 4);

//     int sX = scrollX >> 16;
//     int eX = sX + 40 + CHAR_TRIX_X;
//     int x = (sX * (0x10000 / CHAR_TRIX_X)) >> 16;

//     // Bounded by the board's actual trixel width (BOARD_TRIX_X) rather than
//     // SCREEN_TRIX_X/_BOARD_COLS (a different unit that happens to share the
//     // value 40) -- otherwise this stops marking columns visible as soon as
//     // scrollX passes ~1/4 of the way across the level. Also cap the array
//     // index directly so a scroll near the right edge can't overrun onScreenX.
//     for (int i = sX; i <= eX && i < BOARD_TRIX_X && x < _BOARD_COLS; i += CHAR_TRIX_X)
//         onScreenX[x++] = true;

//     int sY = scrollY >> 16;
//     int eY = sY + _SCANLINES / 3;
//     int y = (sY * (0x10000 / CHAR_TRIX_Y)) >> 16;

//     for (int i = sY; i < eY && y < _BOARD_ROWS; i += CHAR_TRIX_Y)
//         onScreenY[y++] = true;
// }

//------------------------------------------------------------------------------

static const int isActive[] = {

    ATT_PHASE1 | ATT_PHASE2,    // 0
    ATT_PHASE1 | ATT_PHASE4,    // 1
    ATT_PHASE1 | ATT_PHASE2,    // 2
    ATT_PHASE1,                 // 3
};

void initBoard() {

    rockCount = bestRockCount = lastRockCount = 0;
    selectorCounter = 0;
    waterDir = 0;
    explodeCount = 0;
    explodeRadius = 0;
    conveyorDirection = -1;    // ?
    activeStar = lastActiveStar = 0;
    single = false;
    lastConvertedGeodoge = 0;
    convertedGeodoge = 0;
}


void displayFloatingNumber(int trixX, int trixY, int age, int value) {

    //    removeFloatingChars();
    int temp = value;

    int c = 0;
    while (temp >= 100) {
        c++;
        temp -= 100;
    }


    if (convertedGeodoge >= 100) {
        floatingCharacter(trixX, trixY, age, CH_A);    // + c);
        trixX += c == 1 ? 2 : 4;
    }

    // c = 0;
    // while (temp >= 10) {
    //     c++;
    //     temp -= 10;
    // }

    // if (convertedGeodoge >= 10) {
    //     floatingCharacter(trixX, trixY, age, CH_0 + c);
    //     trixX += c == 1 ? 2 : 4;
    // }


    // floatingCharacter(trixX, trixY, age, CH_0 + temp);
}


/*
 * ascii_widths.h
 *
 * Per-character display width table for printable ASCII.
 * Range: ' ' (32) through 'z' (122). All widths default to 4.
 *
 * Index with: ascii_width[c - ASCII_WIDTH_BASE]
 * (bounds-check c against ASCII_WIDTH_BASE/ASCII_WIDTH_MAX first)
 */

#define ASCII_WIDTH_BASE 32 /* ' ' */
#define ASCII_WIDTH_MAX 122 /* 'z' */

static const unsigned char ascii_width[] = {
    4,    //  32  SPACE
    4,    //  33  !   alt 0
    2,    //  34  "   alt 1
    4,    //  35  #   alt 2
    4,    //  36  $   alt 3
    4,    //  37  %   alt 4
    4,    //  38  &   alt 5
    4,    //  39  '   alt 6
    4,    //  40  (   alt 7
    4,    //  41  )   alt 8
    4,    //  42  *   alt 9
    4,    //  43  +
    4,    //  44  ,
    4,    //  45  -
    4,    //  46  .
    4,    //  47  /
    5,    //  48  0
    3,    //  49  1
    5,    //  50  2
    5,    //  51  3
    5,    //  52  4
    5,    //  53  5
    5,    //  54  6
    5,    //  55  7
    5,    //  56  8
    5,    //  57  9
    4,    //  58  :
    4,    //  59  ;
    4,    //  60  <
    4,    //  61  =
    4,    //  62  >
    4,    //  63  ?
    4,    //  64  @
    4,    //  65  A
    4,    //  66  B
    4,    //  67  C
    4,    //  68  D
    4,    //  69  E
    4,    //  70  F
    4,    //  71  G
    4,    //  72  H
    2,    //  73  I
    4,    //  74  J
    4,    //  75  K
    4,    //  76  L
    6,    //  77  M
    4,    //  78  N
    4,    //  79  O
    4,    //  80  P
    4,    //  81  Q
    4,    //  82  R
    4,    //  83  S
    4,    //  84  T
    4,    //  85  U
    4,    //  86  V
    6,    //  87  W
    4,    //  88  X
    4,    //  89  Y
    4,    //  90  Z
    4,    //  91  [
    4,    //  92  BACKSLASH
    4,    //  93  ]
    4,    //  94  ^
    4,    //  95  _
    4,    //  96  `
    4,    //  97  a
    4,    //  98  b
    4,    //  99  c
    4,    // 100  d
    4,    // 101  e
    4,    // 102  f
    4,    // 103  g
    4,    // 104  h
    2,    // 105  i
    4,    // 106  j
    4,    // 107  k
    2,    // 108  l
    6,    // 109  m
    4,    // 110  n
    4,    // 111  o
    4,    // 112  p
    4,    // 113  q
    4,    // 114  r
    4,    // 115  s
    3,    // 116  t
    4,    // 117  u
    4,    // 118  v
    6,    // 119  w
    4,    // 120  x
    4,    // 121  y
    4,    // 122  z
};


int widthOf(char ch) {
    return ascii_width[ch - ASCII_WIDTH_BASE];
}


char convt(char ch) {

    if (ch >= 'A' && ch <= 'Z')
        return CH_CAP_A + ch - 'A';

    else if (ch >= 'a' && ch <= 'z')
        return CH_A + ch - 'a';

    else if (ch >= '0' && ch <= '9')
        return CH_BIG_0 + ch - '0';

    else if (ch >= '!' && ch <= '*')    // alernate small numbers
        return CH_0 + ch - '!';

    return ch;
}

void displayFloatingString(int trixX, int trixY, int age, char *s) {

    int len = 0;
    char *w = s;
    while (*w) {
        len += widthOf(*w);
        w++;
    }

    // if (center)
    //     trixX = (40 - len) >> 1;

    while (*s) {
        floatingCharacter(trixX, trixY, age, convt(*s));
        trixX += widthOf(*s);
        s++;
    }
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


        if (convertedGeodoge && lastConvertedGeodoge == convertedGeodoge) {
            // nothing happened this scan, so we record the conversion

            if (convertedGeodoge > 2) {

                int y = playerY * CHAR_TRIX_Y - (scrollY >> 16) - CHAR_TRIX_Y;
                int x = playerX * CHAR_TRIX_X - (scrollX >> 16) + CHAR_CENTER_X;

                char str[6];
                drawDecimalToString(str, '!', convertedGeodoge);
                displayFloatingString(x, y, 40, str);
            }

            killAudio(SFX_UNCOVER);
            convertedGeodoge = 0;
        }

        lastConvertedGeodoge = convertedGeodoge;


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

        if (debris)
            debris--;

        if (rockCount > bestRockCount)
            bestRockCount = rockCount;

        if (rockCount > lastRockCount) {
            shakeTime += 3;
            debris = 6;
        }

        lastRockCount = rockCount;
        rockCount = 0;


        // gameFrame++;
        // gravity = nextGravity;

        usableSWCHA = bufferedSWCHA;
        bufferedSWCHA = 0xFF;

        // if (boardRow < 0) {
        //     boardRow = _BOARD_ROWS - 1;
        //     boardCol = _BOARD_COLS - 1;
        // } else {
        cursor.col = 0;
        cursor.row = 0;
        cursor.me = RAM + _BOARD;

        // }

        // calculateVisibleMasks();

        // restartBoardScan() may have called setGameState() (e.g. player death), arming
        // SCHEDULE_INIT_STATE -- don't overwrite that back to SCHEDULE_PROCESS_BOARD or the
        // transition deadlocks.
        if (gameState == nextGameState) {
            setSchedule(SCHEDULE_PROCESS_BOARD);
            processBoardSquares();
        }
    }
}


// Worst-case T1TC ticks per board character, indexed by raw char number
// (0-127, matches debug[]). Throttles processBoardSquares()'s loop.
// _untimed_ = no measurement yet. _B = global margin added to all.
// Auto-updated from DEBUG_TIMES CSV exports -- see tools/update_budget_from_csv.py.

#define _untimed_ 12500
#define _B 100

// Last updated: 2026-07-29 17:24 AEST
static const unsigned short budget[128] = {
    _untimed_,    //   0 CH_BLANK
    _untimed_,    //   1 CH_PLACEHOLDER
    _untimed_,    //   2 CH_DIRT
    _untimed_,    //   3 CH_BRICKWALL
    _B + 336,     //   4 CH_DOORCLOSED -- updated 2026-07-28 18:21 AEST (was 201)
    _untimed_,    //   5 CH_DOOROPEN_0
    _untimed_,    //   6 CH_EXITBLANK
    _untimed_,    //   7 CH_STEELWALL
    _untimed_,    //   8 CH_PEBBLE1
    _untimed_,    //   9 CH_PEBBLE2
    _B + 292,     //  10 CH_ROCK -- updated 2026-07-28 16:26 AEST (was 291)
    _B + 4065,    //  11 CH_ROCK_FALLING -- updated 2026-07-29 00:48 AEST (was 3965)
    _B + 2032,    //  12 CH_DOGE_00 -- updated 2026-07-29 00:41 AEST (was 2031)
    _B + 2474,    //  13 CH_DOGE_FALLING -- updated 2026-07-28 00:07 AEST
    _B + 282,     //  14 CH_MELLON_HUSK_BIRTH -- updated 2026-07-28 16:26 AEST (was 281)
    _untimed_,    //  15 CH_LAVA_BLANK
    _untimed_,    //  16 CH_LAVA_SMALL
    _untimed_,    //  17 CH_LAVA_MEDIUM
    _untimed_,    //  18 CH_LAVA_LARGE
    _B + 9600,    //  19 CH_MELLON_HUSK -- updated 2026-07-29 00:45 AEST (was 9587)
    _untimed_,    //  20 CH_DOGE_STATIC
    _untimed_,    //  21 CH_PEBBLE_ROCK
    _untimed_,    //  22 CH_ROCK_PEBBLE
    _untimed_,    //  23 CH_ROCK_PEBBLE_1
    _B + 209,     //  24 CH_DUST_0 -- updated 2026-07-28 16:35 AEST (was untimed)
    _B + 209,     //  25 CH_DUST_1 -- updated 2026-07-28 16:35 AEST (was untimed)
    _B + 207,     //  26 CH_DUST_2 -- updated 2026-07-28 17:17 AEST (was 206)
    _B + 265,     //  27 CH_GEODOGE -- updated 2026-07-28 16:30 AEST (was untimed)
    _B + 242,     //  28 CH_DUST_ROCK_0 -- updated 2026-07-28 00:07 AEST
    _B + 242,     //  29 CH_DUST_ROCK_1 -- updated 2026-07-28 00:07 AEST
    _B + 207,     //  30 CH_DUST_ROCK_2 -- updated 2026-07-28 16:35 AEST (was untimed)
    _B + 434,     //  31 CH_CONVERT_GEODE_TO_DOGE -- updated 2026-07-29 00:41 AEST (was 433)
    _untimed_,    //  32 CH_HORIZONTAL_BAR
    _untimed_,    //  33 CH_PUSH_LEFT
    _untimed_,    //  34 CH_PUSH_LEFT_REVERSE
    _untimed_,    //  35 CH_PUSH_RIGHT
    _untimed_,    //  36 CH_PUSH_RIGHT_REVERSE
    _untimed_,    //  37 CH_VERTICAL_BAR
    _untimed_,    //  38 CH_PUSH_UP
    _untimed_,    //  39 CH_PUSH_UP_REVERSE
    _untimed_,    //  40 CH_PUSH_DOWN
    _untimed_,    //  41 CH_PUSH_DOWN_REVERSE
    _untimed_,    //  42 CH_WYRM_BODY
    _untimed_,    //  43 CH_WYRM_VERT_BODY
    _untimed_,    //  44 CH_WYRM_CORNER_LD
    _untimed_,    //  45 CH_WYRM_CORNER_RD
    _untimed_,    //  46 CH_WYRM_CORNER_LU
    _untimed_,    //  47 CH_WYRM_CORNER_RU
    _B + 221,     //  48 CH_WYRM_HEAD_U -- updated 2026-07-27 23:24 AEST
    _untimed_,    //  49 CH_WYRM_HEAD_R
    _untimed_,    //  50 CH_WYRM_HEAD_D
    _untimed_,    //  51 CH_WYRM_HEAD_L
    _B + 3360,    //  52 CH_GEODOGE_FALLING -- updated 2026-07-27 23:24 AEST
    _untimed_,    //  53 CH_FLIP_GRAVITY_0
    _untimed_,    //  54 CH_FLIP_GRAVITY_1
    _untimed_,    //  55 CH_FLIP_GRAVITY_2
    _untimed_,    //  56 CH_BLOCK
    _untimed_,    //  57 CH_GRINDER_0
    _untimed_,    //  58 CH_GRINDER_1
    _untimed_,    //  59 CH_HUB
    _untimed_,    //  60 CH_WATER
    _untimed_,    //  61 CH_WATERFLOW_0
    _untimed_,    //  62 CH_WATERFLOW_1
    _untimed_,    //  63 CH_WATERFLOW_2
    _untimed_,    //  64 CH_WATERFLOW_3
    _untimed_,    //  65 CH_WATERFLOW_4
    _untimed_,    //  66 CH_HUB_1
    _untimed_,    //  67 CH_OUTLET
    _untimed_,    //  68 CH_BELT_0
    _untimed_,    //  69 CH_BELT_1
    _untimed_,    //  70 CH_PUSH_DOWN2
    _untimed_,    //  71 CH_GEODOGE_CONVERT
    _untimed_,    //  72 CH_CONVERT_PIPE
    _untimed_,    //  73 CH_WYRM_TAIL_U
    _untimed_,    //  74 CH_WYRM_TAIL_R
    _untimed_,    //  75 CH_WYRM_TAIL_D
    _untimed_,    //  76 CH_WYRM_TAIL_L
    _B + 240,     //  77 CH_DOGE_FALLING_TOP -- updated 2026-07-28 00:07 AEST
    _B + 240,     //  78 CH_DOGE_FALLING_BOTTOM -- updated 2026-07-28 16:25 AEST (was 234)
    _B + 240,     //  79 CH_ROCK_FALLING_TOP -- updated 2026-07-28 16:26 AEST (was 239)
    _B + 240,     //  80 CH_ROCK_FALLING_BOTTOM -- updated 2026-07-28 00:07 AEST
    _B + 1487,    //  81 CH_GEODOGE_FALLING_TOP -- updated 2026-07-28 00:07 AEST
    _B + 240,     //  82 CH_GEODOGE_FALLING_BOTTOM -- updated 2026-07-28 16:26 AEST (was 234)
    _untimed_,    //  83 CH_DOGE_FALLING_TOP2
    _untimed_,    //  84 CH_DOGE_FALLING_BOTTOM2
    _B + 249,     //  85 CH_DOGE_SIDE_1 -- updated 2026-07-28 16:30 AEST (was 240)
    _B + 272,     //  86 CH_DOGE_SIDE_3 -- updated 2026-07-28 16:30 AEST (was 263)
    _B + 249,     //  87 CH_DOGE_SIDE_2 -- updated 2026-07-28 16:30 AEST (was 240)
    _B + 272,     //  88 CH_DOGE_SIDE_4 -- updated 2026-07-28 16:30 AEST (was 263)
    _B + 476,     //  89 CH_ELECTRIC_0 -- updated 2026-07-29 00:37 AEST (was 467)
    _B + 391,     //  90 CH_ELECTRIC_1 -- updated 2026-07-29 00:37 AEST (was 382)
    _B + 391,     //  91 CH_ELECTRIC_2 -- updated 2026-07-29 00:37 AEST (was 382)
    _B + 393,     //  92 CH_ELECTRIC_3 -- updated 2026-07-29 00:37 AEST (was 384)
    _untimed_,    //  93 CH_BROKEN_DIRT
    _B + 4085,    //  94 CH_INSULATOR_TOP -- updated 2026-07-29 00:45 AEST (was 4047)
    _B + 236,     //  95 CH_INSULATOR_BOTTOM -- updated 2026-07-29 03:45 AEST (was 230)
    _B + 2003,    //  96 CH_STAR -- updated 2026-07-29 00:37 AEST (was 1985)
    _B + 249,     //  97 CH_STAR_FALLING_TOP -- updated 2026-07-28 17:27 AEST (was 234)
    _B + 249,     //  98 CH_STAR_FALLING_BOTTOM -- updated 2026-07-29 03:45 AEST (was 248)
    _untimed_,    //  99 CH_ROCK_BONUS
    _untimed_,    // 100 CH_STAR_EXPLODE
    _B + 6736,    // 101 CH_INSULATOR_L -- updated 2026-07-29 00:45 AEST (was 3771)
    _B + 236,     // 102 CH_INSULATOR_R -- updated 2026-07-29 03:45 AEST (was 230)
    _B + 476,     // 103 CH_ELECTRIC_H0 -- updated 2026-07-29 00:37 AEST (was 467)
    _B + 391,     // 104 CH_ELECTRIC_H1 -- updated 2026-07-29 00:37 AEST (was 382)
    _B + 391,     // 105 CH_ELECTRIC_H2 -- updated 2026-07-29 00:37 AEST (was 382)
    _B + 393,     // 106 CH_ELECTRIC_H3 -- updated 2026-07-29 00:37 AEST (was 384)
    _B + 197,     // 107 CH_CROSSED_STREAMS -- updated 2026-07-29 03:45 AEST (was 192)
    _untimed_,    // 108 CH_MOUNT_U
    _untimed_,    // 109 CH_MOUNT_D
    _untimed_,    // 110 CH_MOUNT_L
    _untimed_,    // 111 CH_MOUNT_R
    _untimed_,    // 112 CH_PIT_L0
    _untimed_,    // 113 CH_PIT_R0
    _B + 1366,    // 114 CH_BOMB -- updated 2026-07-29 03:45 AEST (was 1351)
    _B + 4594,    // 115 CH_CRACKED_BRICK -- updated 2026-07-29 15:01 AEST (was untimed)
    _untimed_,    // 116 CH_CONCRETE
    _untimed_,    // 117 (unused)
    _untimed_,    // 118 (unused)
    _untimed_,    // 119 (unused)
    _untimed_,    // 120 (unused)
    _untimed_,    // 121 (unused)
    _untimed_,    // 122 (unused)
    _untimed_,    // 123 (unused)
    _untimed_,    // 124 (unused)
    _untimed_,    // 125 (unused)
    _untimed_,    // 126 (unused)
    _untimed_,    // 127 (unused)
};


void processBoardSquares() {


#ifdef DEBUG_TIMES
    int chIndex = -1;
    unsigned int chStart = 0;
#endif

    int select = isActive[selectorCounter & 3];

    while (T1TC < availableIdleTime) {

#ifdef DEBUG_TIMES
        if (chIndex >= 0) {
            unsigned int elapsed = T1TC - chStart;
            if ((int)elapsed > debug[chIndex])
                debug[chIndex] = elapsed;
            chIndex = -1;
        }
#endif
        unsigned char creature = *cursor.me;
        if (creature < FLAG_THISFRAME) {

            enum ObjectType type = CharToType[creature];
            if (Attribute[type] & select) {

                if (T1TC + budget[creature] > availableIdleTime)
                    break;

#ifdef DEBUG_TIMES
                chStart = T1TC;
                chIndex = GET(creature);
#endif

                if (!processTypes(&cursor, type, creature))
                    processCreatures(&cursor, creature);

                if (T1TC > availableIdleTime) {
                    debug[200] = T1TC - availableIdleTime;
                    debug[201] = creature;
                    FLASH(0xD6, 12);
                }
            }
        }

        // Clears "scanned this frame" flags on the previous row (last row is not cleared here).
        if (cursor.row) {
            unsigned char *prev = cursor.me - _BOARD_COLS;
            *prev &= ~FLAG_THISFRAME;
        }

        if (++cursor.col > (_BOARD_COLS - 1)) {
            cursor.col = 0;
            if (++cursor.row > _BOARD_ROWS - 1) {
                // Same guard as setupBoardScanner() -- a cell processed earlier
                // this pass may have just called setGameState().
                if (gameState == nextGameState)
                    setSchedule(SCHEDULE_START_SCAN);
                return;
            }
        }

        cursor.me++;
    }

#ifdef DEBUG_TIMES
    if (chIndex >= 0) {
        unsigned int elapsed = T1TC - chStart;
        if ((int)elapsed > debug[chIndex])
            debug[chIndex] = elapsed;
        chIndex = -1;
    }
#endif
}


bool processTypes(BoardCursor *cur, enum ObjectType type, unsigned char creature) {

    switch (type) {

    case TYPE_BOMB:

        if (*Animate[type] == CH_BLANK) {
            *cur->me = CH_BLANK;

            shakeTime = 50;
            FLASH(0x48, 30);
            ADDAUDIO(SFX_EXPLODE);
            explode(cur->me, CH_DUST_ROCK_0);


        } else

            // if (!rangeRandom(30))
            // nDots(1, cur->col, cur->row, PT_CHARACTER, 30, CHAR_CENTER_X, CHAR_CENTER_Y, 2500, CH_RUBBLE);


            nDots(2, cur->col, cur->row, PT_SPIRAL, 10, 4, 1, rangeRandom(50) + 20, 1 + (getRandom32() & 2));
        break;


    case TYPE_CRACKED_BRICK: {

        // tally stacked massive objects above
        unsigned char *above = cur->me - _BOARD_COLS;
        while (Attribute[CharToType[GET(*above)]] & ATT_MASSIVE) {
            rockCount++;
            above -= _BOARD_COLS;
        }

        int rchar = bestRockCount;
        if (rchar > 7)
            rchar = 7;

        startCharAnimation(TYPE_CRACKED_BRICK, AnimateCrackedBrick + rchar * 2);


        if (bestRockCount > 7) {
            if (!rangeRandom(7)) {
                FLASH(0x16, 4);
                ADDAUDIO(SFX_EXPLODE_QUIET);
                nDots(10, cur->col, cur->row, PT_SPIRAL, 40, CHAR_CENTER_X, CHAR_CENTER_Y, 40, 3);
                *cur->me = FLAG(CH_DUST_ROCK_0);
            }
        }

        else {


            if (debris)
                for (int i = 0; i < 6; i++) {
                    int idx = nDots(1, cur->col, cur->row, PT_TWO, rangeRandom(20) + 20, rangeRandom(CHAR_TRIX_X), 1,
                                    20 + rangeRandom(15), (getRandom32() & 1) ? 1 : 7);
                    if (idx >= 0)
                        particle[idx].dir = 0;
                }
        }
        break;
    }

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

    case TYPE_DOGE: {

        unsigned char *next = cur->me + _BOARD_COLS;    // * gravity;
        const unsigned int attrNext = Attribute[CharToType[GET(*next)]];

        if (attrNext & ATT_BLANK) {
            *cur->me = FLAG(CH_DOGE_FALLING_TOP);
            *next = FLAG(CH_DOGE_FALLING_BOTTOM);
        }

        else if (attrNext & ATT_ROLL)
            doRoll(cur->me, cur->row, cur->col);

        break;
    }

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
                if (lumTarget == luminance) {
                    setGameState(GS_MENU);
                    return true;
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

const unsigned char thisFrame[] = {0, FLAG_THISFRAME, FLAG_THISFRAME, 0};

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
        *cursor.me = CH_BLANK;
        break;

    case CH_DUST_0:
    case CH_DUST_1:
    case CH_DUST_ROCK_0:
    case CH_DUST_ROCK_1:
        (*cursor.me)++;
        break;

    case CH_CONVERT_GEODE_TO_DOGE: {

        convertedGeodoge++;
        *cursor.me = CH_DOGE_00;

        // const unsigned char thisFrame[] = {0, FLAG_THISFRAME, FLAG_THISFRAME, 0};

#define CVTGEO(flag, offset)                                                                                           \
    newDogeCandidate = cursor.me + offset;                                                                             \
    if (Attribute[CharToType[GET(*newDogeCandidate)]] & ATT_GEODOGE) {                                                 \
        *newDogeCandidate = CH_CONVERT_GEODE_TO_DOGE | flag;                                                           \
        ADDAUDIO(SFX_UNCOVER);                                                                                         \
    }

        unsigned char *newDogeCandidate;

        CVTGEO(0, -_BOARD_COLS)
        CVTGEO(FLAG_THISFRAME, 1)
        CVTGEO(FLAG_THISFRAME, _BOARD_COLS)
        CVTGEO(0, -1)

        break;
    }

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


    case CH_ROCK: {
        unsigned char *next = cursor.me + _BOARD_COLS;
        if (Attribute[CharToType[GET(*next)]] & ATT_BLANK) {
            *next = FLAG(CH_ROCK_FALLING_BOTTOM);
            *cursor.me = FLAG(CH_ROCK_FALLING_TOP);
        }
        break;
    }

    case CH_GEODOGE: {
        unsigned char *next = cursor.me + _BOARD_COLS;
        if (Attribute[CharToType[GET(*next)]] & ATT_BLANK) {
            *next = FLAG(CH_GEODOGE_FALLING_BOTTOM);
            *cursor.me = FLAG(CH_GEODOGE_FALLING_TOP);
        }
        break;
    }


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
    case CH_GEODOGE_FALLING: {

        processFallingThings(cur->me, cur->row, cur->col, creature);
        break;
    }


    case CH_ROCK_FALLING: {

        unsigned char *next = cursor.me + _BOARD_COLS;
        int att = Attribute[CharToType[GET(*next)]];

        if (att & ATT_BLANK) {
            *cursor.me = FLAG(CH_ROCK_FALLING_TOP);
            *next = FLAG(CH_ROCK_FALLING_BOTTOM);

            // unsigned char *nextNext = next + _BOARD_COLS;
            // enum ChName downCh = GET(*nextNext);
            // typeDown = CharToType[downCh];
            // const unsigned int attNextNext = Attribute[typeDown];

            // if (downCh != CH_ROCK_FALLING && downCh != CH_DOGE_FALLING && downCh != CH_GEODOGE_FALLING) {

            //     if (attNextNext & ATT_HARD) {

            //         ADDAUDIO(SFX_ROCK);

            //         unsigned char *dL = cursor.me + _BOARD_COLS - 1;
            //         unsigned char *dR = dL + 2;

            //         if (!CharToType[GET(*dR)]) {
            //             nDots(4, cursor.col, cursor.row + 1, PT_SPIRAL, 10, 3, 7, 100, 2);
            //         }

            //         if (!CharToType[GET(*dL)]) {
            //             nDots(4, cursor.col, cursor.row + 1, PT_SPIRAL, 10, 3, 7, 100, 2);
            //         }
            //     }
            // }
        }

        else if (att & ATT_SQUASHABLE_TO_BLANKS) {
            explode(next, FLAG(CH_DUST_0));
            initParticles();    //??

        } else {

            // stop falling
            *cursor.me = FLAG(CH_ROCK);
            ADDAUDIO(att & ATT_HARD ? SFX_ROCK : SFX_ROCK2);
            nDots(6, cursor.col, cursor.row, PT_TWO, 20, 2, 10, 60, 7);
        }
        break;
    }

    case CH_DOORCLOSED:
        if (!doges) {
            *cursor.me = CH_DOOROPEN_0;
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
                // Same world-to-screen math as decodeCaves.c, but centred on the player's
                // CURRENT position (death can happen anywhere), not the level start.
                // playerX/Y is the cell's top-left corner in trix, not its centre -- add
                // CHAR_CENTER_X/Y to get the middle (same offset particle.c uses).
                startSwipeClose(playerX * CHAR_TRIX_X + CHAR_CENTER_X - (scrollX >> 16),
                                playerY * CHAR_TRIX_Y + CHAR_CENTER_Y - (scrollY >> 16));
            }
#endif
        }


        if (playerDead && !shakeTime
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


void processFallingThings(unsigned char *me, int row, int col, unsigned char creature) {

    unsigned char *next = me + _BOARD_COLS;
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

            // case CH_ROCK_FALLING:
            //     *me = FLAG(CH_ROCK_FALLING_TOP);
            //     *next = FLAG(CH_ROCK_FALLING_BOTTOM);
            //     break;
        }

        unsigned char *nextNext = next + _BOARD_COLS;
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

            // case CH_ROCK_FALLING: {
            //     *me = CH_ROCK;
            //     sfx = att & ATT_HARD ? SFX_ROCK : SFX_ROCK2;
            //     break;
            // }

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
        unsigned char sc = *side;
        if (sc < FLAG_THISFRAME && (Attribute[CharToType[sc]] & ATT_BLANK)) {

            unsigned char *sideDown = side + _BOARD_COLS;
            unsigned char sd = *sideDown;
            if (sd < FLAG_THISFRAME && (Attribute[CharToType[sd]] & ATT_BLANK)) {

                if (offset > 0) {
                    *me = FLAG(CH_DOGE_SIDE_1);
                    *(me + offset) = FLAG(CH_DOGE_SIDE_3);

                } else {
                    *me = FLAG(CH_DOGE_SIDE_2);
                    *(me + offset) = FLAG(CH_DOGE_SIDE_4);
                }

                *(sideDown) = FLAG(CH_BLANK);

                int off = offset < 0 ? 4 : 0;

                nDots(1, col, row, PT_TWO, 15, offset * 2 + off, 4, 0, 1);
                nDots(1, col, row, PT_TWO, 20, offset * 4 + off, 4, 0, 1);
                nDots(1, col, row, PT_TWO, 25, offset * 6 + off, 7, 0, 1);
                nDots(1, col, row, PT_TWO, 30, offset * 7 + off, 10, 0, 1);

                return;
            }
        }
    }
}


void explode(unsigned char *where, unsigned char explosionShape) {

    ADDAUDIO(SFX_EXPLODE);

    int offset[] = {-_BOARD_COLS - 1, -_BOARD_COLS, -_BOARD_COLS + 1, -1, 1,
                    _BOARD_COLS - 1,  _BOARD_COLS,  _BOARD_COLS + 1,  0};
    int shape[] = {explosionShape, explosionShape, explosionShape, explosionShape, explosionShape,
                   explosionShape, explosionShape, explosionShape, explosionShape};


    for (int i = 0; i < (int)(sizeof(offset) / sizeof(offset[0])); i++) {
        unsigned char *cell = where + offset[i];
        enum ObjectType cellType = CharToType[GET(*cell)];
        if (Attribute[cellType] & ATT_EXPLODABLE) {

            bool wasDoge = (cellType == TYPE_DOGE);
            bool becomesDoge = (shape[i] == CH_DOGE_00);

            // nDots(5, , y, PT_SPIRAL, 30, CHAR_CENTER_X, CHAR_CENTER_Y, 50, 7);

            if (wasDoge && !becomesDoge)
                totalDogePossible--;    // a doge sitting here is being destroyed
            else if (!wasDoge && becomesDoge)
                totalDogePossible++;    // this cell is being turned into a new doge
            // else: doge->doge (no change) or non-doge->non-doge (nothing to count)

            *cell = shape[i];
        }

        else if (cellType == TYPE_OUTBOX_PRE)
            *cell = CH_DOOROPEN_0;
    }

    FLASH(4, 4);
}


// EOF
