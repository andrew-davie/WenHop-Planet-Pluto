#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "animations.h"
#include "attribute.h"
#include "board.h"

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

// How long the teleport tile's spiral dots spin alone before the fade-to-black starts --
// board.c's TYPE_MELLON_HUSK case ticks once per full board-scan (one tick every
// SPEED_BASE real frames -- see setupBoardScanner()), i.e. ~12 ticks/sec at NTSC's 60fps.
// 12 ticks ~= 1 second.

#define TELEPORT_SWIRL_TICKS 12

// Idle ambience for a teleport tile nobody's using: TYPE_TELEPORT is PH4 (attribute.c),
// so this case is only reached ~3 times/sec per tile (1 of every 4 board scans, and full
// scans run ~12/sec -- see TELEPORT_SWIRL_TICKS's comment above). 1-in-20 of those makes
// a single spiral dot land roughly every 6-7 seconds on average -- occasional, not a swirl.

#define TELEPORT_IDLE_SPARKLE_CHANCE 20

// Very short compared to SHAKE_TICKS (mellon.c, half a second -- the player-triggered
// shove/pickup feedback): this one repeats every scan for as long as the player stays put
// underneath a blocked rock/geodoge (case CH_ROCK/CH_GEODOGE, below), so each individual shake
// only needs to read as a single strained flinch, not a sustained vibration.
#define HAZARD_SHAKE_TICKS 10


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
void convertWaterAndLavaObjects(unsigned char *me, int row, enum ObjectType type);
void processCharBeltAndGrinder(unsigned char *me, unsigned char creature);
void processFallingThings(unsigned char *me, int row, int col, unsigned char creature);
void processCharRock(unsigned char *me);

void genericPush(unsigned char *me, int row, int col, int offsetX, int offsetY);
void genericPushReverse(unsigned char *me, int offsetX, int offsetY);
void chainReact_Pipe(unsigned char *me);
void doRoll(unsigned char *me, int row, int col);
void doRollRock(unsigned char *me, int row, int col);
void doRollGeodoge(unsigned char *me, int row, int col);
void doRollMotionArc(int col, int row, int trailSide);
void spawnBaseRaindrops(int col, int row);
enum RollEligibility { ROLL_NONE, ROLL_LOOSE, ROLL_STRICT };
enum RollEligibility rollEligibility(unsigned char *me, int offset);
bool blockedByPlayerBelow(unsigned char *me, int offset);
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
    3,    //  34  "   alt 1
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
    5,    //  65  A
    5,    //  66  B
    5,    //  67  C
    5,    //  68  D
    5,    //  69  E
    5,    //  70  F
    5,    //  71  G
    5,    //  72  H
    4,    //  73  I
    5,    //  74  J
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


void displayFloatingString(int trixX, int trixY, int age, char *s, bool centered) {

    if (centered) {
        int len = 0;
        char *w = s;
        while (*w) {
            len += widthOf(*w);
            w++;
        }

        trixX -= (len >> 1);
    }

    while (*s) {
        floatingCharacter(trixX, trixY, age, convt(*s));
        trixX += widthOf(*s);
        s++;
    }
}


void setupBoardScanner() {

#if ENABLE_SWIPE
    // Freeze all board processing until the swipe reveal (level intro/outro iris) has fully
    // finished -- nothing should fall/roll/move while the screen is still being revealed.
    // gameFrame just keeps counting up past gameSpeed in the meantime (harmless -- it's only
    // ever compared against that threshold below), so the very next call after the swipe
    // finishes resumes immediately, with no extra lag tacked on.
    if (!checkSwipeFinished())
        return;
#endif

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
                displayFloatingString(x, y, 50, str, false);
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

            if (gravity > 0 && liquidTrixel_8 && !ss)
                liquidTrixel_8 -= gravity << 8;

            // Surface lava "bubbles"
            int posX = ((scrollX * 5) >> 16) + rangeRandom(_BOARD_COLS);
            nDotsAtTrixel(1, posX, (liquidTrixel_8 >> 8) - 2, PT_SPIRAL, 120, 0x10, 7);    // untested speed
        }

        if (showWater) {
            // static const signed char sinus[] = {0,    25,   49,   71,   90,   106,  117, 125, 127, 125, 117,
            //                                     106,  90,   71,   49,   25,   0,    -25, -49, -71, -90, -106,
            //                                     -117, -125, -127, -125, -117, -106, -90, -71, -49, -25};
            // static int waves = 0;

            if (isStormActive() && liquidTrixel_8 > 0) {

                if (!rangeRandom(20)) {
                    ADDAUDIO(SFX_THUNDER);
                    FLASH(0x0F, 1);
                }

                // lavaSurfaceTrixel -= sinus[(waves >> 5) & 15];

                // if (((sinus[(waves >> 5) & 15] & 3) != 0) || (gameFrame & 31) == 0) {
                //     ++waves;

                // if (!(gameFrame & 7))
                //     lavaSurfaceTrixel--;
                // }

                // liquidTrixel_8 += sinus[(waves >> 5) & 31];
                // waves++;

                liquidTrixel_8 -= 20;
            }

            for (int i = 0; i < 4; i++) {
                int posX = ((scrollX * 5) >> 16) + rangeRandom(_BOARD_COLS);
                int deep = (liquidTrixel_8 >> 8) + rangeRandom(21 * CHAR_Y - (liquidTrixel_8 >> 8));
                if (deep > 0 && deep < _SCANLINES)
                    bubbles(1, posX, deep, 2240, 0x8000);
            }
        }

        if (debris)
            debris--;

        if (rockCount > bestRockCount)
            bestRockCount = rockCount;

        if (rockCount > lastRockCount) {
            shakeTime += 6;
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

// Last updated: 2026-08-07 00:03 AEST
static const unsigned short budget[128] = {
    _untimed_,     //   0 CH_BLANK
    _untimed_,     //   1 CH_PLACEHOLDER
    _untimed_,     //   2 CH_DIRT
    _untimed_,     //   3 CH_BRICKWALL
    _B + 336,      //   4 CH_DOORCLOSED -- updated 2026-07-28 18:21 AEST (was 201)
    _untimed_,     //   5 CH_DOOROPEN_0
    _untimed_,     //   6 CH_EXITBLANK
    _untimed_,     //   7 CH_STEELWALL
    _B + 346,      //   8 CH_PEBBLE1 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 528,      //   9 CH_PEBBLE2 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 2325,    //  10 CH_ROCK -- updated 2026-08-07 00:03 AEST (was 2323)
    _B + 2429,     //  11 CH_ROCK_FALLING -- updated 2026-08-07 00:00 AEST (was 2407)
    _B + 2064,    //  12 CH_DOGE_00 -- updated 2026-08-07 00:03 AEST (was 2063)
    _B + 2467,     //  13 CH_DOGE_FALLING -- updated 2026-08-07 00:00 AEST (was 390)
    _B + 292,      //  14 CH_MELLON_HUSK_BIRTH -- updated 2026-08-06 23:32 AEST (was 291)
    _untimed_,     //  15 CH_LAVA_BLANK
    _untimed_,     //  16 CH_LAVA_SMALL
    _untimed_,     //  17 CH_LAVA_MEDIUM
    _untimed_,     //  18 CH_LAVA_LARGE
    _B + 10123,    //  19 CH_MELLON_HUSK -- updated 2026-08-06 23:32 AEST (was 10122)
    _B + 195,      //  20 CH_DOGE_STATIC -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 181,      //  21 CH_PEBBLE_ROCK -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 210,      //  22 CH_ROCK_PEBBLE -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 210,      //  23 CH_ROCK_PEBBLE_1 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 252,      //  24 CH_DUST_0 -- updated 2026-08-06 23:27 AEST (was 209)
    _B + 252,      //  25 CH_DUST_1 -- updated 2026-08-06 23:57 AEST (was 251)
    _B + 250,      //  26 CH_DUST_2 -- updated 2026-08-06 23:57 AEST (was 249)
    _B + 2366,    //  27 CH_GEODOGE -- updated 2026-08-07 00:03 AEST (was 1608)
    _B + 252,      //  28 CH_DUST_ROCK_0 -- updated 2026-08-06 23:27 AEST (was 242)
    _B + 252,      //  29 CH_DUST_ROCK_1 -- updated 2026-08-06 23:27 AEST (was 242)
    _B + 250,      //  30 CH_DUST_ROCK_2 -- updated 2026-08-06 23:27 AEST (was 207)
    _B + 466,      //  31 CH_CONVERT_GEODE_TO_DOGE -- updated 2026-08-06 23:32 AEST (was 434)
    _B + 165,      //  32 CH_HORIZONTAL_BAR -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 2319,     //  33 CH_PUSH_LEFT -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 235,      //  34 CH_PUSH_LEFT_REVERSE -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 2142,     //  35 CH_PUSH_RIGHT -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 236,      //  36 CH_PUSH_RIGHT_REVERSE -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 165,      //  37 CH_VERTICAL_BAR -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 2319,     //  38 CH_PUSH_UP -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 235,      //  39 CH_PUSH_UP_REVERSE -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 395,      //  40 CH_PUSH_DOWN -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 236,      //  41 CH_PUSH_DOWN_REVERSE -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 235,      //  42 CH_WYRM_BODY -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  43 CH_WYRM_VERT_BODY -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  44 CH_WYRM_CORNER_LD -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 234,      //  45 CH_WYRM_CORNER_RD -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  46 CH_WYRM_CORNER_LU -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  47 CH_WYRM_CORNER_RU -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  48 CH_WYRM_HEAD_U -- updated 2026-08-06 23:32 AEST (was 221)
    _B + 235,      //  49 CH_WYRM_HEAD_R -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  50 CH_WYRM_HEAD_D -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  51 CH_WYRM_HEAD_L -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 1814,    //  52 CH_GEODOGE_FALLING -- updated 2026-08-07 00:03 AEST (was 632)
    _untimed_,     //  53 CH_FLIP_GRAVITY_0
    _untimed_,     //  54 CH_FLIP_GRAVITY_1
    _untimed_,     //  55 CH_FLIP_GRAVITY_2
    _B + 228,      //  56 CH_BLOCK -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 452,      //  57 CH_GRINDER_0 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 436,      //  58 CH_GRINDER_1 -- updated 2026-07-31 13:42 AEST (was untimed)
    _untimed_,     //  59 CH_HUB
    _B + 283,      //  60 CH_WATER -- updated 2026-08-06 23:27 AEST (was 232)
    _B + 292,      //  61 CH_WATERFLOW_0 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 296,      //  62 CH_WATERFLOW_1 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 1256,     //  63 CH_WATERFLOW_2 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 1256,     //  64 CH_WATERFLOW_3 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 296,      //  65 CH_WATERFLOW_4 -- updated 2026-07-31 13:42 AEST (was untimed)
    _untimed_,     //  66 CH_HUB_1
    _B + 198,      //  67 CH_OUTLET -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 268,      //  68 CH_BELT_0 -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 268,      //  69 CH_BELT_1 -- updated 2026-07-31 13:42 AEST (was untimed)
    _untimed_,     //  70 CH_PUSH_DOWN2
    _untimed_,     //  71 CH_GEODOGE_CONVERT
    _B + 514,      //  72 CH_CONVERT_PIPE -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 235,      //  73 CH_WYRM_TAIL_U -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  74 CH_WYRM_TAIL_R -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  75 CH_WYRM_TAIL_D -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 235,      //  76 CH_WYRM_TAIL_L -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 250,      //  77 CH_DOGE_FALLING_TOP -- updated 2026-08-07 00:00 AEST (was 249)
    _B + 250,      //  78 CH_DOGE_FALLING_BOTTOM -- updated 2026-08-06 23:32 AEST (was 240)
    _B + 250,      //  79 CH_ROCK_FALLING_TOP -- updated 2026-08-06 23:27 AEST (was 240)
    _B + 256,      //  80 CH_ROCK_FALLING_BOTTOM -- updated 2026-08-06 23:27 AEST (was 240)
    _B + 1495,     //  81 CH_GEODOGE_FALLING_TOP -- updated 2026-08-06 23:32 AEST (was 1487)
    _B + 250,      //  82 CH_GEODOGE_FALLING_BOTTOM -- updated 2026-08-06 23:32 AEST (was 240)
    _B + 226,      //  83 CH_DOGE_FALLING_TOP2 -- updated 2026-07-31 13:42 AEST (was untimed)
    _untimed_,     //  84 CH_DOGE_FALLING_BOTTOM2
    _B + 250,     //  85 CH_DOGE_SIDE_1 -- updated 2026-08-07 00:03 AEST (was 249)
    _B + 272,      //  86 CH_DOGE_SIDE_3 -- updated 2026-07-28 16:30 AEST (was 263)
    _B + 249,      //  87 CH_DOGE_SIDE_2 -- updated 2026-07-28 16:30 AEST (was 240)
    _B + 272,      //  88 CH_DOGE_SIDE_4 -- updated 2026-07-28 16:30 AEST (was 263)
    _B + 483,      //  89 CH_ELECTRIC_0 -- updated 2026-08-06 23:32 AEST (was 476)
    _B + 398,      //  90 CH_ELECTRIC_1 -- updated 2026-08-06 23:32 AEST (was 391)
    _B + 398,      //  91 CH_ELECTRIC_2 -- updated 2026-08-06 23:32 AEST (was 391)
    _B + 400,      //  92 CH_ELECTRIC_3 -- updated 2026-08-06 23:32 AEST (was 393)
    _untimed_,     //  93 CH_BROKEN_DIRT
    _B + 4085,     //  94 CH_INSULATOR_TOP -- updated 2026-07-29 00:45 AEST (was 4047)
    _B + 236,      //  95 CH_INSULATOR_BOTTOM -- updated 2026-07-29 03:45 AEST (was 230)
    _B + 2003,     //  96 CH_STAR -- updated 2026-07-29 00:37 AEST (was 1985)
    _B + 250,      //  97 CH_STAR_FALLING_TOP -- updated 2026-08-07 00:00 AEST (was 249)
    _B + 250,      //  98 CH_STAR_FALLING_BOTTOM -- updated 2026-08-07 00:00 AEST (was 249)
    _untimed_,     //  99 CH_ROCK_BONUS
    _B + 374,      // 100 CH_STAR_EXPLODE -- updated 2026-07-31 13:42 AEST (was untimed)
    _B + 6736,     // 101 CH_INSULATOR_L -- updated 2026-07-29 00:45 AEST (was 3771)
    _B + 236,      // 102 CH_INSULATOR_R -- updated 2026-07-29 03:45 AEST (was 230)
    _B + 483,      // 103 CH_ELECTRIC_H0 -- updated 2026-08-06 23:32 AEST (was 476)
    _B + 398,      // 104 CH_ELECTRIC_H1 -- updated 2026-08-06 23:32 AEST (was 391)
    _B + 398,      // 105 CH_ELECTRIC_H2 -- updated 2026-08-06 23:32 AEST (was 391)
    _B + 400,      // 106 CH_ELECTRIC_H3 -- updated 2026-08-06 23:32 AEST (was 393)
    _B + 235,      // 107 CH_CROSSED_STREAMS -- updated 2026-08-06 23:32 AEST (was 197)
    _B + 1366,     // 108 CH_BOMB -- updated 2026-07-29 03:45 AEST (was 1351)
    _B + 4594,     // 109 CH_CRACKED_BRICK -- updated 2026-07-29 15:01 AEST (was untimed)
    _untimed_,     // 110 CH_CONCRETE
    _B + 883,      // 111 CH_TELEPORT (PH4, idle sparkle -- not yet measured) -- updated 2026-08-06 23:27 AEST (was 766)
    _untimed_,     // 112 CH_KEY
    _untimed_,     // 113 CH_DOOROPEN_STATIC
    _B + 424,      // 114 CH_IMMOVABLE -- updated 2026-08-06 23:32 AEST (was 261)
    _B + 2299,     // 115 CH_IMMOVABLE_FALLING -- updated 2026-08-06 23:32 AEST (was 2024)
    _B + 213,      // 116 CH_IMMOVABLE_FALLING_TOP -- updated 2026-08-06 23:32 AEST (was 169)
    _B + 213,      // 117 CH_IMMOVABLE_FALLING_BOTTOM -- updated 2026-08-06 23:32 AEST (was 169)
    _B + 213,      // 118 CH_ROCK_SIDE_1 -- updated 2026-08-06 23:27 AEST (was 170)
    _B + 213,      // 119 CH_ROCK_SIDE_2 -- updated 2026-08-06 23:27 AEST (was 170)
    _B + 281,      // 120 CH_ROCK_SIDE_3 -- updated 2026-08-06 23:32 AEST (was 277)
    _B + 281,     // 121 CH_ROCK_SIDE_4 -- updated 2026-08-07 00:03 AEST (was 277)
    _B + 213,      // 122 CH_GEODOGE_SIDE_1 -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 213,      // 123 CH_GEODOGE_SIDE_2 -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 275,      // 124 CH_GEODOGE_SIDE_3 -- updated 2026-08-06 23:32 AEST (was untimed)
    _B + 276,      // 125 CH_GEODOGE_SIDE_4 -- updated 2026-08-06 23:32 AEST (was untimed)
    _untimed_,     // 126 (unused)
    _untimed_,     // 127 (unused)
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

            convertWaterAndLavaObjects(cursor.me, cursor.row, type);

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

        else if (debris) {
            int idx = nDots(1, cur->col, cur->row, PT_TWO, rangeRandom(20) + 20, rangeRandom(CHAR_TRIX_X), 1,
                            20 + rangeRandom(15), (getRandom32() & 1) ? 1 : 7);
            if (idx >= 0)
                particle[idx].dir = 0;
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
                //                ADDAUDIO(SFX_SPACE);
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
        // Suppressed once the player has actually stepped onto the exit (exitMode) -- this fires
        // on every board-scan visit to the cell, i.e. continuously for the whole walk-off/fade
        // sequence, and was colliding with the door-glyph animation and the player's own fade,
        // reading as the door and player randomly flashing colour and the door's final closed
        // frame taking longer than it should to actually settle in. Still fires normally for any
        // other TYPE_OUTBOX cell (decorative pre-opened doors, etc.) that isn't mid-exit.
        if (!exitMode) {
            FLASH(0x28, 10);
            nDots(10, cur->col, cur->row, PT_SPIRAL, 40, 2, 5, 0x40, 7);    // untested speed
        }
        break;

    case TYPE_TELEPORT:
        // Quiescent tile only -- teleportLocked means THIS tile is mid-departure (its board
        // cell stays CH_TELEPORT throughout, see checkHighPriorityMove()'s comment), which
        // already has its own dedicated swirl (updateTeleportDepartSwirl()); skip so the two
        // don't stack. Otherwise just an occasional single dot so it doesn't look completely
        // dead between uses -- same particle type/age as the real swirls
        // (TELEPORT_SWIRL_PARTICLE_AGE, mellon.h) for a consistent look, just one dot instead
        // of a batch.
        if (!teleportLocked && !rangeRandom(TELEPORT_IDLE_SPARKLE_CHANCE))
            nDots(1, cur->col, cur->row, PT_SPIRAL2, TELEPORT_SWIRL_PARTICLE_AGE, CHAR_CENTER_X, CHAR_CENTER_Y, 32, 7);
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
        if (exitMode) {

            exitMode--;
            if (exitMode < 20) {
                lumTarget = -15;
                if (lumTarget == luminance) {
                    setGameState(GS_MENU);
                    return true;
                }
            }

            // Once the walk-in glide has fully settled (autoMoveFrameCount == 0, same gate
            // teleportLocked uses below) -- i.e. the player is now standing squarely on the exit
            // tile, not still sliding in -- start them walking on the spot as if continuing on
            // through the doorway, instead of leaving them frozen mid-stride. ID_WalkUpSlow, not
            // ID_WalkUp -- same 4 frames, held much longer each (playerAnimation.c), so the leg
            // cycle matches the gentle drift below instead of looking like a fast walk/jog in
            // place; ID_WalkUp itself is untouched for ordinary movement elsewhere.
            // updatePlayerAnimation() (playerAnimation.c) does the actual "walking off into the
            // distance" work, easing frameAdjustY up for as long as exitMode and this settled
            // state both hold. movePlayer() (which would normally settle the animation itself once
            // movement stops) is never called while exitMode is set, so nothing else does it; the
            // != guard keeps this a one-shot rather than restarting the walk cycle every tick for
            // the rest of the countdown.
            if (!autoMoveFrameCount && playerAnimationID != ID_WalkUpSlow)
                startPlayerAnimation(ID_WalkUpSlow);
        }

        else if (teleportLocked) {

            // The player is locked here from the instant they stepped onto the teleport tile
            // (see checkHighPriorityMove(), mellon.c) -- movePlayer() isn't called at all while
            // this is set, so joystick input is simply ignored, same as exitMode above. Wait for
            // the walk-in glide (autoMoveFrameCount) to fully settle, then let the spiral dots
            // spin alone for TELEPORT_SWIRL_TICKS before fading to black exactly like exitMode
            // does above. Once the fade completes, switch cave -- loadCave() (gameState_Game.c)
            // fades back up from there on its own, same as any level start.
            // cur->col/cur->row are the driver husk square the player *left* -- moveHusk()
            // is skipped for the teleport tile itself (mellon.c) so it keeps showing its
            // RAM-static glyph, which means cur no longer tracks where the player actually
            // is. Use playerX/playerY directly so the swirl is centred on the teleport tile.
            //
            // Unlike everything below, this isn't gated on !autoMoveFrameCount -- it starts
            // the instant teleportLocked goes true (startTeleportDepartSwirl() is called
            // right there, in checkHighPriorityMove()), i.e. as soon as the player starts
            // moving onto the tile, so the swirl is already well under way by the time the
            // sprite actually vanishes below instead of only beginning at that instant.
            updateTeleportDepartSwirl(playerX, playerY);

            if (!autoMoveFrameCount) {

                if (!teleportCountingDown) {

                    teleportCountingDown = true;
                    teleportSwirlTicks = TELEPORT_SWIRL_TICKS;

                    // movePlayer() would normally settle this back to standing itself once
                    // movement stops -- it's skipped entirely while locked, so do it here instead.
                    if (playerAnimationID == ID_WalkUp || playerAnimationID == ID_MineUp)
                        startPlayerAnimation(ID_StandUp);
                    else if (playerAnimationID == ID_Walk || playerAnimationID == ID_Mine)
                        startPlayerAnimation(ID_StandLR);
                    else if (playerAnimationID == ID_WalkDown || playerAnimationID == ID_MineDown)
                        startPlayerAnimation(ID_Stand);
                }

                if (teleportSwirlTicks) {

                    if (!--teleportSwirlTicks)
                        lumTarget = -15;
                }

                else if (lumTarget == luminance)
                    teleportRequested = true;
            }
        }

        else {

            // Unlike teleportLocked/exitMode above, doorLocked does NOT skip movePlayer() --
            // the player stays fully controllable while the key settles into the door and it
            // swings open (mellon.c's doorLocked comment has the full reasoning); updateDoorUnlock()
            // just needs to keep ticking alongside it every frame it's set.
            if (doorLocked)
                updateDoorUnlock();

            movePlayer(cur);
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
        int attNext = Attribute[CharToType[GET(*next)]];

        if (attNext & ATT_BLANK) {
            *next = FLAG(CH_ROCK_FALLING_BOTTOM);
            *cursor.me = FLAG(CH_ROCK_FALLING_TOP);

            // Same under-object raindrop cue doRollRock() uses when it settles without rolling
            // (see spawnBaseRaindrops()'s own comment) -- here it's the "just committed to
            // falling straight down" moment instead.
            spawnBaseRaindrops(cursor.col, cursor.row);
        }

        // Can't fall straight down (whatever's below isn't blank), but if it's something a
        // rock can roll off (ATT_ROLL -- another rock, a wall, concrete, etc, same convention
        // TYPE_DOGE already uses above) and one side is open with room to keep falling past it,
        // start rolling that way instead of just sitting there. See doRollRock()'s own comment.
        //
        // Gated to only fire on a PH2-active pass (same phase TYPE_ROCK_ROLLING's own
        // CH_ROCK_SIDE_1-4 are processed on): CH_ROCK itself is PH1 (checked every pass), so
        // without this the roll could START on either phase parity, and since the transition
        // only actually gets looked at again on PH2 passes, that made the hang time an
        // inconsistent 1-or-2-pass wait depending on luck. Starting only on a phase this check
        // itself is active makes the very next PH2 pass exactly 2 passes away, always -- a
        // consistent wait instead of a coin flip.
        //
        // !(selectorCounter & 1), not isActive[selectorCounter & 3] & ATT_PHASE2 -- equivalent
        // (isActive[] only sets PH2 at indices 0/2, i.e. exactly the even values of
        // selectorCounter & 3), but cheaper: a straight parity check on selectorCounter itself
        // instead of a table lookup. Tied to PH2's CURRENT layout in isActive[] above, not
        // symbolically to ATT_PHASE2 -- if that table's PH2 slots ever move off the even indices
        // this needs updating alongside it.
        else if ((attNext & ATT_ROLL) && !(selectorCounter & 1))
            doRollRock(cursor.me, cursor.row, cursor.col);

        // Can't fall AND can't roll off because the thing directly underneath is specifically
        // the player (TYPE_MELLON_HUSK isn't ATT_BLANK or ATT_ROLL) -- strain against them: a
        // very short shake (findFreeHazardSlot(), mellon.h) plus the same under-object raindrop
        // cue as the genuine-fall case above, same mechanism as the roll-blocked case
        // (doRollRock()'s ROLL_LOOSE). Raindrops fire regardless of whether a hazard slot was
        // actually free -- "something's straining here" reads fine even on the rare pass where
        // both slots are already busy elsewhere and the shake itself has to wait.
        else if (CharToType[GET(*next)] == TYPE_MELLON_HUSK) {

            int slot = findFreeHazardSlot(cursor.col, cursor.row);
            if (slot >= 0)
                shakeObject(slot, cursor.me, cursor.col, cursor.row, HAZARD_SHAKE_TICKS);

            spawnBaseRaindrops(cursor.col, cursor.row);
        }

        break;
    }

    case CH_GEODOGE: {
        unsigned char *next = cursor.me + _BOARD_COLS;
        int attNext = Attribute[CharToType[GET(*next)]];

        if (attNext & ATT_BLANK) {
            *next = FLAG(CH_GEODOGE_FALLING_BOTTOM);
            *cursor.me = FLAG(CH_GEODOGE_FALLING_TOP);

            // Same under-object raindrop cue as case CH_ROCK, above.
            spawnBaseRaindrops(cursor.col, cursor.row);
        }

        // Same roll mechanic as CH_ROCK (see doRollRock()'s comment), same PH2 phase gate for
        // the same consistent-timing reason -- see case CH_ROCK, above.
        else if ((attNext & ATT_ROLL) && !(selectorCounter & 1))
            doRollGeodoge(cursor.me, cursor.row, cursor.col);

        // Same "blocked specifically by the player" strain as CH_ROCK, above.
        else if (CharToType[GET(*next)] == TYPE_MELLON_HUSK) {

            int slot = findFreeHazardSlot(cursor.col, cursor.row);
            if (slot >= 0)
                shakeObject(slot, cursor.me, cursor.col, cursor.row, HAZARD_SHAKE_TICKS);

            spawnBaseRaindrops(cursor.col, cursor.row);
        }

        break;
    }

    // Same ambient "should I start falling" check as CH_ROCK/CH_GEODOGE above -- an immovable
    // block can't be pushed or mined away, but gravity doesn't ask permission.
    case CH_IMMOVABLE: {
        unsigned char *next = cursor.me + _BOARD_COLS;
        int attNext = Attribute[CharToType[GET(*next)]];

        if (attNext & ATT_BLANK) {
            *next = FLAG(CH_IMMOVABLE_FALLING_BOTTOM);
            *cursor.me = FLAG(CH_IMMOVABLE_FALLING_TOP);
        }

        // Can't fall because the thing directly underneath is specifically the player -- no
        // roll option for an immovable (unlike CH_ROCK/CH_GEODOGE, it never rolls off anything),
        // so this is the only "can't fall" case it has.
        else if (CharToType[GET(*next)] == TYPE_MELLON_HUSK) {

            int slot = findFreeHazardSlot(cursor.col, cursor.row);
            if (slot >= 0)
                shakeObject(slot, cursor.me, cursor.col, cursor.row, HAZARD_SHAKE_TICKS);
        }

        break;
    }


    case CH_ROCK_FALLING_TOP:
        *cur->me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_IMMOVABLE_FALLING_TOP:
        *cur->me = FLAG(CH_DUST_ROCK_0);    // same departure-dust cosmetic as a falling rock
        break;

    case CH_GEODOGE_FALLING_TOP:

        nDots(3, cur->col, cur->row, PT_TWO, 15, rangeRandom(CHAR_TRIX_X), rangeRandom(CHAR_TRIX_Y), 0, 2);

        *cur->me = FLAG(CH_DUST_ROCK_0);
        break;

    case CH_DOGE_SIDE_1:
    case CH_DOGE_SIDE_2:
        *cur->me = FLAG(CH_BLANK);
        break;

    case CH_ROCK_SIDE_1:
    case CH_ROCK_SIDE_2:
        *cur->me = FLAG(CH_BLANK);
        break;

    // Arrived in the new column -- re-enter the ordinary vertical-fall pipeline exactly as if
    // support had just vanished from under a settled CH_ROCK (case CH_ROCK, above): TOP dissolves
    // to dust next scan while BOTTOM (one row down, normally already confirmed clear or crushed
    // by doRollRock()) becomes the real CH_ROCK_FALLING.
    //
    // That "one row down is clear" assumption breaks when TWO rocks roll into the SAME column
    // from vertically adjacent rows on the same tick (e.g. a tall gap beside a rock column --
    // every rock along that edge is independently roll-eligible at once): the lower rock can be
    // anywhere along ITS OWN transition -- still CH_ROCK_SIDE_3/4 (TYPE_ROCK_ROLLING, PH2 --
    // lingers 2 scans), or already past that into CH_ROCK_FALLING_TOP/BOTTOM (TYPE_ROCK_FALLING,
    // PH1 -- resolves every scan) -- by the time this upper rock gets looked at again. Either way
    // it's still mid-transition through this exact cell, not really settled there yet, so defer
    // (do nothing, stay CH_ROCK_SIDE_3/4) rather than risk clobbering whatever it's about to
    // become. Once the cell below is neither, it's either genuinely blank (the lower rock has
    // moved on) -- commit normally -- or it raced ahead and actually SETTLED there (landed on
    // solid ground one row down and stopped) -- in which case this rock can't fall through after
    // all either; settle it as an ordinary resting CH_ROCK instead of overwriting whatever
    // that turned out to be.
    case CH_ROCK_SIDE_3:
    case CH_ROCK_SIDE_4: {
        unsigned char *below = cur->me + _BOARD_COLS;
        enum ObjectType belowType = CharToType[GET(*below)];

        if (belowType == TYPE_ROCK_ROLLING || belowType == TYPE_ROCK_FALLING)
            break;

        if (Attribute[belowType] & ATT_BLANK) {
            *cur->me = FLAG(CH_ROCK_FALLING_TOP);
            *below = FLAG(CH_ROCK_FALLING_BOTTOM);
        } else
            *cur->me = FLAG(CH_ROCK);

        break;
    }

    case CH_GEODOGE_SIDE_1:
    case CH_GEODOGE_SIDE_2:
        *cur->me = FLAG(CH_BLANK);
        break;

    // Same idea as CH_ROCK_SIDE_3/4 above, but resolving into the geodoge's own falling pair --
    // see its own comment for the same "two rocks roll into one column" collision this guards
    // against.
    case CH_GEODOGE_SIDE_3:
    case CH_GEODOGE_SIDE_4: {
        unsigned char *below = cur->me + _BOARD_COLS;
        enum ObjectType belowType = CharToType[GET(*below)];

        if (belowType == TYPE_GEODOGE_ROLLING || belowType == TYPE_GEODOGE_FALLING)
            break;

        if (Attribute[belowType] & ATT_BLANK) {
            *cur->me = FLAG(CH_GEODOGE_FALLING_TOP);
            *below = FLAG(CH_GEODOGE_FALLING_BOTTOM);
        } else
            *cur->me = FLAG(CH_GEODOGE);

        break;
    }

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

    case CH_IMMOVABLE_FALLING_BOTTOM:
        *cur->me = FLAG(CH_IMMOVABLE_FALLING);
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

            // Unlike a deliberate carried-drop (mellon.c), a rock that fell here on its own
            // should stay completely silent, shake-wise, UNLESS it lands directly on a
            // cracked brick, lands on a massive object sitting on a cracked brick, or lands
            // on a massive object sitting on a massive object sitting on a cracked brick --
            // i.e. a brick within the top 3 squares of the stack it just landed on/against.
            // Landing on dirt/wall/steel/anything else, or on a deeper stack with no brick
            // that close, is a total non-event here.
            //
            // Unrolled to exactly 3 checks (next, one below it, one below that) instead of a
            // loop that walks however far the actual chain goes -- breaking a cracked brick is
            // designed around at most 3 rocks stacked on it, so nothing legitimate ever needs
            // to look further than that anyway. This matters beyond just average-case speed:
            // board.c's time-sliced scheduler (processBoardSquares()) reserves
            // budget[CH_ROCK_FALLING]'s registered WORST case before it'll even attempt to
            // process a falling rock at all -- an unbounded walk would inflate that one shared
            // reservation to match the tallest possible chain anywhere on the board, and EVERY
            // falling rock would have to pay it just to get scheduled, even ones that will
            // never walk past the first cell. A fixed 3-check cap keeps that reservation small
            // and constant regardless of board layout, so screens with dozens of rocks falling
            // at once don't start starving each other of processing time over a cost that
            // (per the 3-rock design rule) was never actually going to be paid.
            enum ObjectType belowType = CharToType[GET(*next)];
            if (belowType == TYPE_CRACKED_BRICK)
                shakeTime += 4;

            else if (Attribute[belowType] & ATT_MASSIVE) {

                unsigned char *below = next + _BOARD_COLS;
                belowType = CharToType[GET(*below)];

                if (belowType == TYPE_CRACKED_BRICK)
                    shakeTime += 4;

                else if (Attribute[belowType] & ATT_MASSIVE) {
                    below += _BOARD_COLS;
                    if (CharToType[GET(*below)] == TYPE_CRACKED_BRICK)
                        shakeTime += 4;
                }
            }
        }
        break;
    }

    // Same fall/land mechanics as CH_ROCK_FALLING (support check, squash whatever's
    // squashable underneath, settle with a landing sound) but without the cracked-brick
    // chain-shake -- that was a bespoke rock/geodoge feature, not asked for here. An
    // immovable block still can't be mined or exploded once it's fallen and settled: that's
    // enforced by TYPE_IMMOVABLE simply never having ATT_MINE/ATT_EXPLODABLE (attribute.c),
    // not by anything special in this case.
    case CH_IMMOVABLE_FALLING: {

        unsigned char *next = cursor.me + _BOARD_COLS;
        int att = Attribute[CharToType[GET(*next)]];

        if (att & ATT_BLANK) {
            *cursor.me = FLAG(CH_IMMOVABLE_FALLING_TOP);
            *next = FLAG(CH_IMMOVABLE_FALLING_BOTTOM);
        }

        else if (att & ATT_SQUASHABLE_TO_BLANKS) {
            explode(next, FLAG(CH_DUST_0));
            initParticles();

        } else {

            // stop falling
            *cursor.me = FLAG(CH_IMMOVABLE);
            ADDAUDIO(att & ATT_HARD ? SFX_ROCK : SFX_ROCK2);
            nDots(6, cursor.col, cursor.row, PT_TWO, 20, 2, 10, 60, 7);
        }
        break;
    }

    case CH_DOORCLOSED:
        if (!doges) {

            // Kick off the shared TYPE_DOOR split-open animation the first time any closed door
            // is revisited after the last doge is collected -- Animate[TYPE_DOOR] still points at
            // AnimateDoor[0] (CH_DOORCLOSED, delay 0, holds forever) until this fires, so checking
            // against that same starting pointer is enough to trigger exactly once, no separate
            // latch needed. Every CH_DOORCLOSED cell in the cave shares this one animation (see
            // AnimateBase[]'s "animate in unison" warning), so they'll all split open together,
            // not just whichever cell the scanner happens to be on.
            if (Animate[TYPE_DOOR] == AnimateDoor) {
                startCharAnimation(TYPE_DOOR, AnimateDoor + 2);
                ADDAUDIO(SFX_DOOR);
            }

            // Only commit THIS cell to the real open/walkable state once the shared animation has
            // actually finished -- same idiom as TYPE_BOMB/TYPE_STAR_EXPLODE above (*Animate[type]
            // == <terminal frame>), so a door can't become walkable before it looks open. Commits
            // to CH_DOOROPEN_STATIC, not CH_DOOROPEN_0 -- see AnimateDoor's comment (animations.c)
            // for why: CH_DOOROPEN_0 is TYPE_OUTBOX, which flashes forever (AnimFlashOut); this
            // cell just needs to sit still on the open glyph. No FLASH() either -- just the
            // animation settling. Cells the scanner revisits before then just fall through and
            // try again next pass.
            if (*Animate[TYPE_DOOR] == CH_DOOROPEN_STATIC)
                *cursor.me = CH_DOOROPEN_STATIC;
        }
        break;

    // Mirror of CH_DOORCLOSED's own case above, but running in reverse: mellon.c's
    // TYPE_TELEPORT trigger kicks off the shared TYPE_DOOR_OPEN closing animation
    // (startCharAnimation(TYPE_DOOR_OPEN, AnimateDoorClose + 2)) the instant the player steps
    // onto a teleport tile, so this cell just waits for that shared animation to reach its own
    // terminal frame (CH_DOORCLOSED, held forever) before committing the real board write --
    // same "cells the scanner revisits before then just fall through and try again" idiom.
    // No trigger-latch check needed here (unlike CH_DOORCLOSED's Animate[TYPE_DOOR] ==
    // AnimateDoor test) since nothing about revisiting this cell before a teleport ever starts
    // that animation on its own -- TYPE_DOOR_OPEN's AnimateBase entry (animations.c) is
    // AnimateDoorClose's own idle landing pad (frame 0, holds forever), which never equals
    // CH_DOORCLOSED, so this is a no-op every frame until mellon.c actually triggers it.
    case CH_DOOROPEN_STATIC:
        if (*Animate[TYPE_DOOR_OPEN] == CH_DOORCLOSED)
            *cursor.me = CH_DOORCLOSED;
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
        playerDead = (manType != TYPE_MELLON_HUSK && manType != TYPE_MELLON_HUSK_PRE && manType != TYPE_OUTBOX &&
                      !exitMode && !teleportLocked);


        if (oldDead != playerDead) {

            shakeTime = 40;
            FLASH(0x34, 20);

            attachments[SLOT_CARRY].type = 0;

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

    if ((row - 1) * CHAR_TRIX_Y >= (liquidTrixel_8 >> 8)) {
        unsigned char *neighbour = me + dirOffset[waterDir & 3];
        if (Attribute[CharToType[GET(*neighbour)]] & ATT_DISSOLVES) {
            *neighbour = CH_DUST_0;
        }
    }
}

void convertWaterAndLavaObjects(unsigned char *me, int row, enum ObjectType type) {

    // Any blanks/dissolves below the rising surface convert to lava or water.
    if ((showLava || showWater) && row * CHAR_TRIX_Y >= (liquidTrixel_8 >> 8) && (Attribute[type] & ATT_CONVERT))
        *me = showLava ? CH_LAVA_BLANK : CH_WATER;
}

void processWaterFlow(unsigned char *me, int row, int col) {

    // Lag the interruption of water flowing downwards
    unsigned char above = *(me - _BOARD_COLS);
    if (above < FLAG_THISFRAME && !(Attribute[CharToType[GET(above)]] & ATT_WATERFLOW)) {
        *me = FLAG(CH_BLANK);
        return;
    }

    int line = (row + 1) * CHAR_TRIX_Y;
    if (line < (liquidTrixel_8 >> 8)) {
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

            // Same chain-to-cracked-brick shake as board.c's CH_ROCK_FALLING landing case --
            // see its own comment for the full reasoning (3-rock design rule, fixed 3-check
            // unrolled cap instead of a loop, why that matters for budget[]'s worst-case
            // reservation). TYPE_GEODOGE is ATT_MASSIVE same as TYPE_ROCK, so a falling
            // geodoge landing directly on a cracked brick, or on a massive object (rock or
            // geodoge) that's on one, or two deep, shakes exactly the same way a falling rock
            // does. TYPE_DOGE is NOT massive (checked directly against attribute.c), so plain
            // doges staying out of this is correct, not an oversight.
            enum ObjectType belowType = CharToType[GET(*next)];
            if (belowType == TYPE_CRACKED_BRICK)
                shakeTime += 4;

            else if (Attribute[belowType] & ATT_MASSIVE) {

                unsigned char *below = next + _BOARD_COLS;
                belowType = CharToType[GET(*below)];

                if (belowType == TYPE_CRACKED_BRICK)
                    shakeTime += 4;

                else if (Attribute[belowType] & ATT_MASSIVE) {
                    below += _BOARD_COLS;
                    if (CharToType[GET(*below)] == TYPE_CRACKED_BRICK)
                        shakeTime += 4;
                }
            }
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


// Comic-style motion arc: 3 concentric arcs of short-lived stationary dots on the side a rolling
// rock/geodoge rolled IN FROM (trailSide, the opposite of the roll's direction of travel), each
// arc a copy of the same shallow curve shifted further out, like classic comic-book speed lines
// stacked behind a rolling wheel. Speed 0 throughout -- see nDots()/drawParticles()'s "distance
// += speed" update -- so these never drift, they just sit at their spawn spot and fade, reading
// as "something was just here" rather than another outward-flying spark (that's what the
// existing PT_TWO burst in doRollRock()/doRollGeodoge() already does, on the opposite side).
// Ages are short and fixed (no randomness) since this is meant as a quick one-frame-of-motion
// cue, not a lingering effect; farther-out arcs are given a little extra age so the whole trail
// doesn't wink out in one frame.
void doRollMotionArc(int col, int row, int trailSide) {

    int off = trailSide < 0 ? 4 : 0;

    for (int arc = 0; arc < 3; arc++) {
        int radius = arc * 3;
        int age = 8 + arc * 3;

        nDots(1, col, row, PT_TWO, age, trailSide * (2 + radius) + off, 3, 0, 1);
        nDots(1, col, row, PT_TWO, age + 3, trailSide * (4 + radius) + off, 5, 0, 1);
        nDots(1, col, row, PT_TWO, age + 6, trailSide * (6 + radius) + off, 7, 0, 1);
    }
}


// Roughly 1-in-2 chance of a single literal raindrop along the object's own base (bottom row of
// its own cell), at any of its 5 trixel columns (picked at random). Same spawn makeRain()
// (particle.c) uses for real weather -- sphereDot() directly (not nDots()), same dir/distance/
// speed reset right after (PT_RAIN repurposes dir as a fall-velocity accumulator that has to
// start at rest, not sphereDot()'s randomized compass angle -- see its own comment) -- but a much
// shorter 30-frame age; makeRain()'s own 200 (appropriate for a drop falling the length of the
// whole screen) left this one lingering far too long for something already spawned at ground
// level. Shared by doRollRock()/doRollGeodoge() (settled without rolling) and case
// CH_ROCK/CH_GEODOGE (just committed to falling straight down) -- same "something's happening
// here" cue either way.
void spawnBaseRaindrops(int col, int row) {

    if (rangeRandom(2))
        return;

    // 3 drops per trigger, each independently at a random column. Middle columns (1-3) sit
    // one trixel lower than the edge columns (0, 4) -- the object's own base glyph isn't flat,
    // so this keeps each drop's spawn hugging it visually.
    for (int n = 0; n < 3; n++) {

        int rc = rangeRandom(CHAR_TRIX_X);
        int ry = CHAR_TRIX_Y - 1 + (rc >= 1 && rc <= 3 ? 1 : 0);

        int idx = sphereDot(col * CHAR_TRIX_X + rc, row * CHAR_TRIX_Y + ry, PT_RAIN, 30, 5);
        if (idx >= 0) {
            particle[idx].dir = 0;
            particle[idx].distance = 0;
            particle[idx].speed = 0;
            particle[idx].silent = true;    // no splash SFX/burst -- see struct Particle's own comment
        }
    }
}


// Classifies one side (offset -1/+1) in a SINGLE pass over the two relevant cells -- avoids
// doing the same GET()/CharToType[]/Attribute[] lookups twice over via separate strict+loose
// checks, and lets the caller bail out immediately when neither side is even worth a second
// glance (the common case: a rock boxed in solid on both sides, which never gets past here).
//
// Checks the DIAGONAL-BELOW cell (me + offset + _BOARD_COLS) before the side cell (me + offset),
// deliberately the opposite of reading order -- in a typical cave, solid ground one row down is
// far more likely than an open side, so this early-outs to ROLL_NONE on the more-likely-to-fail
// check first, skipping the side lookup entirely in the common "boxed in below" case. Same final
// result either order; this is purely about which lookup happens first.
//
// ROLL_STRICT: side is ATT_BLANK AND the square diagonally below THAT is ALSO ATT_BLANK -- a
// real roll is physically possible this way. TYPE_MELLON_HUSK/_PRE (the player's own board
// placeholder) is never ATT_BLANK, so the player standing in either cell already fails this on
// its own merits.
//
// ROLL_LOOSE: side is blank-or-player AND below-that is blank-or-player, but not both plain
// blank (i.e. blocked only by the player somewhere) -- not enough to actually roll, but enough
// to show the near-miss particle burst instead of nothing visible happening at all.
//
// ROLL_NONE: solid on this side, full stop.
enum RollEligibility rollEligibility(unsigned char *me, int offset) {

    enum ObjectType downType = CharToType[GET(*(me + offset + _BOARD_COLS))];
    bool downBlank = Attribute[downType] & ATT_BLANK;
    if (!downBlank && downType != TYPE_MELLON_HUSK && downType != TYPE_MELLON_HUSK_PRE)
        return ROLL_NONE;

    enum ObjectType sideType = CharToType[GET(*(me + offset))];
    bool sideBlank = Attribute[sideType] & ATT_BLANK;
    if (!sideBlank && sideType != TYPE_MELLON_HUSK && sideType != TYPE_MELLON_HUSK_PRE)
        return ROLL_NONE;

    return (sideBlank && downBlank) ? ROLL_STRICT : ROLL_LOOSE;
}

// Narrower than "rollEligibility() == ROLL_LOOSE": that also fires when the player is standing
// directly BESIDE the object (side itself is the player, same row) -- which is exactly the
// position needed to fire-button pick the object up, so using it as the hazard-shake trigger made
// simply walking up to a rock resting on another ATT_ROLL surface immediately shake it into
// CH_PLACEHOLDER, breaking pickup entirely. This only returns true for the specific "would roll
// down into the player's own column" case the shake is meant for: the side square itself is
// genuinely open (not the player), but the square diagonally below THAT is the player (or their
// pre-materialization placeholder) blocking the landing spot -- standing beside the object in the
// same row never matches this.
bool blockedByPlayerBelow(unsigned char *me, int offset) {

    if (!(Attribute[CharToType[GET(*(me + offset))]] & ATT_BLANK))
        return false;

    enum ObjectType downType = CharToType[GET(*(me + offset + _BOARD_COLS))];
    return downType == TYPE_MELLON_HUSK || downType == TYPE_MELLON_HUSK_PRE;
}


// A settled CH_ROCK that can't fall straight down (case CH_ROCK, above) but is resting on
// something with ATT_ROLL classifies each side with rollEligibility(), above, and bails out
// immediately if neither is even ROLL_LOOSE (the common case for a rock boxed in solid on both
// sides) -- every resting rock on the board pays this cost every PH2 pass regardless of whether
// anything is ever going to happen, so an ASAP early-out here matters far more than anything
// downstream of it. If a side IS at least ROLL_STRICT, it always rolls -- no dice -- picking at
// random between left/right if BOTH reached ROLL_STRICT. If neither side reached ROLL_STRICT
// (only blocked by the player, or truly boxed in), a single raindrop particle at the rock's own
// base reads as "it just shifted/settled without going anywhere" instead of nothing visible
// happening at all.
//
// When it DOES commit: if that side square is blank AND the square diagonally below it is blank
// OR crushable, the rock commences a one-frame diagonal-roll transition into that column
// (CH_ROCK_SIDE_1/2 here, CH_ROCK_SIDE_3/4 in the side square), which then re-enters the ordinary
// fall pipeline from its new column next scan (case CH_ROCK_SIDE_3/4) exactly as if it had just
// lost support underneath -- see their own comments. Same shape as doRoll() (TYPE_DOGE's
// equivalent, above) but: (a) uses the rock's own transition characters instead of doge's, (b)
// also accepts a crushable landing spot, exploding it out of the way rather than requiring it to
// already be blank -- a doge rolls off things gently, a multi-tonne rock does not.

void doRollRock(unsigned char *me, int row, int col) {

    enum RollEligibility left = rollEligibility(me, -1);
    enum RollEligibility right = rollEligibility(me, 1);

    if (left == ROLL_NONE && right == ROLL_NONE)
        return;

    // Always rolls when at least one side is ROLL_STRICT -- no dice. If BOTH sides are, pick
    // between them at random instead of always favouring left.
    int offset;
    if (left == ROLL_STRICT && right == ROLL_STRICT)
        offset = rangeRandom(2) ? 1 : -1;
    else if (left == ROLL_STRICT)
        offset = -1;
    else if (right == ROLL_STRICT)
        offset = 1;
    else
        offset = 0;

    if (offset) {

        unsigned char *side = me + offset;
        unsigned char *sideDown = side + _BOARD_COLS;
        unsigned char sd = *sideDown;
        enum ObjectType sideDownType = CharToType[sd];
        int attSideDown = Attribute[sideDownType];

        // rollEligibility() above already guarantees sideDown is ATT_BLANK (and never the
        // player, who's never ATT_BLANK) whenever it returned ROLL_STRICT, so this re-check
        // only matters for the extra ATT_SQUASHABLE_TO_BLANKS landing-spot case that doesn't
        // reach ROLL_STRICT -- still excludes TYPE_MELLON_HUSK/_PRE explicitly since
        // ATT_SQUASHABLE_TO_BLANKS is also set on the player's own placeholder for unrelated
        // reasons (see mellon.c).
        if (sd < FLAG_THISFRAME && sideDownType != TYPE_MELLON_HUSK && sideDownType != TYPE_MELLON_HUSK_PRE &&
            (attSideDown & (ATT_BLANK | ATT_SQUASHABLE_TO_BLANKS))) {

            if (offset > 0) {
                *me = FLAG(CH_ROCK_SIDE_1);
                *(me + offset) = FLAG(CH_ROCK_SIDE_3);

            } else {
                *me = FLAG(CH_ROCK_SIDE_2);
                *(me + offset) = FLAG(CH_ROCK_SIDE_4);
            }

            // Single-cell crush, not explode() -- explode() hits the full 3x3 area around
            // sideDown, and TYPE_MELLON_HUSK carries ATT_EXPLODABLE (so a bomb's blast can
            // kill the player standing in it), which would let a rock rolling past crush the
            // player via splash damage from an ADJACENT cell even with the direct-target
            // exclusion above. A rock rolling over one specific squashable thing has no
            // business having a blast radius at all.
            if (attSideDown & ATT_SQUASHABLE_TO_BLANKS) {
                ADDAUDIO(SFX_ROCK2);
                *(sideDown) = FLAG(CH_DUST_0);
            } else
                *(sideDown) = FLAG(CH_BLANK);

            int off = offset < 0 ? 4 : 0;

            nDots(1, col, row, PT_TWO, 15, offset * 2 + off, 4, 0, 1);
            nDots(1, col, row, PT_TWO, 20, offset * 4 + off, 4, 0, 1);
            nDots(1, col, row, PT_TWO, 25, offset * 6 + off, 7, 0, 1);
            nDots(1, col, row, PT_TWO, 30, offset * 7 + off, 10, 0, 1);

            // Comic-style motion arc: a few stationary (speed 0, so distance never grows --
            // see nDots()/drawParticles()) dots trailing on the side the rock rolled IN
            // FROM, not the spark burst's direction of travel above. Fixed short fades, no
            // randomness -- just a "this was here a moment ago" cue, not another burst.
            // Temporarily disabled for evaluation -- see doRollGeodoge()'s own call site too.
            // doRollMotionArc(col, row, -offset);

            return;
        }
    }

    // Blocked from rolling specifically because the player is standing in the LANDING spot one
    // row down (blockedByPlayerBelow(), above) -- deliberately narrower than "rollEligibility()
    // returned ROLL_LOOSE", which also fires when the player is standing directly BESIDE this
    // rock in the same row (exactly the position needed to fire-button pick it up) -- using that
    // broader check here used to shake (and CH_PLACEHOLDER) a rock the instant the player walked
    // up next to it, breaking pickup outright. Strain in place: a very short shake
    // (findFreeHazardSlot(), mellon.h). Doesn't skip or replace the under-rock raindrop cue
    // below -- that fires independently on its own chance regardless of whether this also fires.
    // A DIFFERENT rock could simultaneously be blocked this way from the other side (one flanking
    // the player on the left, a completely different one on the right) -- that's what the second
    // hazard slot is for.
    if (blockedByPlayerBelow(me, -1) || blockedByPlayerBelow(me, 1)) {

        int slot = findFreeHazardSlot(col, row);
        if (slot >= 0)
            shakeObject(slot, me, col, row, HAZARD_SHAKE_TICKS);
    }

    // Didn't roll this tick -- see spawnBaseRaindrops()'s own comment.
    spawnBaseRaindrops(col, row);
}


// Same as doRollRock() above, but for a settled CH_GEODOGE rolling off an ATT_ROLL surface --
// see its comment for the full reasoning (single-cell crush not explode(), player-square
// exclusion, near-miss base burst, ASAP early-out, etc). Only the transition characters differ
// (CH_GEODOGE_SIDE_1-4, resolving into CH_GEODOGE_FALLING_TOP/BOTTOM instead of the rock's own
// pair).
void doRollGeodoge(unsigned char *me, int row, int col) {

    enum RollEligibility left = rollEligibility(me, -1);
    enum RollEligibility right = rollEligibility(me, 1);

    if (left == ROLL_NONE && right == ROLL_NONE)
        return;

    // Always rolls when at least one side is ROLL_STRICT -- no dice. If BOTH sides are, pick
    // between them at random instead of always favouring left.
    int offset;
    if (left == ROLL_STRICT && right == ROLL_STRICT)
        offset = rangeRandom(2) ? 1 : -1;
    else if (left == ROLL_STRICT)
        offset = -1;
    else if (right == ROLL_STRICT)
        offset = 1;
    else
        offset = 0;

    if (offset) {

        unsigned char *side = me + offset;
        unsigned char *sideDown = side + _BOARD_COLS;
        unsigned char sd = *sideDown;
        enum ObjectType sideDownType = CharToType[sd];
        int attSideDown = Attribute[sideDownType];

        if (sd < FLAG_THISFRAME && sideDownType != TYPE_MELLON_HUSK && sideDownType != TYPE_MELLON_HUSK_PRE &&
            (attSideDown & (ATT_BLANK | ATT_SQUASHABLE_TO_BLANKS))) {

            if (offset > 0) {
                *me = FLAG(CH_GEODOGE_SIDE_1);
                *(me + offset) = FLAG(CH_GEODOGE_SIDE_3);

            } else {
                *me = FLAG(CH_GEODOGE_SIDE_2);
                *(me + offset) = FLAG(CH_GEODOGE_SIDE_4);
            }

            if (attSideDown & ATT_SQUASHABLE_TO_BLANKS) {
                ADDAUDIO(SFX_ROCK2);
                *(sideDown) = FLAG(CH_DUST_0);
            } else
                *(sideDown) = FLAG(CH_BLANK);

            int off = offset < 0 ? 4 : 0;

            nDots(1, col, row, PT_TWO, 15, offset * 2 + off, 4, 0, 1);
            nDots(1, col, row, PT_TWO, 20, offset * 4 + off, 4, 0, 1);
            nDots(1, col, row, PT_TWO, 25, offset * 6 + off, 7, 0, 1);
            nDots(1, col, row, PT_TWO, 30, offset * 7 + off, 10, 0, 1);

            // Temporarily disabled for evaluation -- see doRollRock()'s own call site too.
            // doRollMotionArc(col, row, -offset);

            return;
        }
    }

    // Blocked from rolling specifically because the player is standing in the landing spot one
    // row down -- see blockedByPlayerBelow()'s own comment (doRollRock(), above) for why this is
    // narrower than a plain ROLL_LOOSE check.
    if (blockedByPlayerBelow(me, -1) || blockedByPlayerBelow(me, 1)) {

        int slot = findFreeHazardSlot(col, row);
        if (slot >= 0)
            shakeObject(slot, me, col, row, HAZARD_SHAKE_TICKS);
    }

    // Same literal-raindrop spawn as doRollRock() -- see spawnBaseRaindrops()'s own comment.
    spawnBaseRaindrops(col, row);
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

        else if (cellType == TYPE_DOOR)
            *cell = CH_DOOROPEN_STATIC;    // blast damage -- blown open instantly, no slide
                                           // animation; static, not CH_DOOROPEN_0, so it doesn't
                                           // flash forever either (see AnimateDoor's comment)
    }

    FLASH(4, 4);
}


// EOF
