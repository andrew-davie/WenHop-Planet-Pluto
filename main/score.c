#include <stdbool.h>
#include <stdint.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "decodeCaves.h"
#include "main.h"
#include "mellon.h"
#include "random.h"
#include "reverseBits.h"
#include "score.h"

// #define RGB_BLACK       0
#define RGB_RED 1
#define RGB_BLUE 2
// #define RGB_PURPLE      3
#define RGB_GREEN 4
#define RGB_YELLOW 5
#define RGB_AQUA 6
// #define RGB_WHITE       7

#define SPARKLE 180 /* # frames to sparkle BG on extra life */

int actualScore;
int partialScore;
static int forceScoreDraw;

enum SCORE_MODE scoreCycle;

static unsigned char scoreLineNew[10];
static unsigned char scoreLineColour[10];

static int toggle = 0;

void addScore(int score) {

    actualScore += score;

    partialScore += score;

    while (partialScore >= 500) {
        partialScore -= 500;
        lives++;
        sparkleTimer = SPARKLE;
        setScoreCycle(SCORELINE_LIVES);
    }
}

const int pwr[] = {
    1, 10, 100, 1000, 10000, 100000,
};

// right-to-left, least-significant first digit position
const unsigned char mask[] = {
    0x0F, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0x0F, 0x0F,
};

const bool mirror[] = {
    1, 1, 0, 0, 1, 1, 1, 0, 0, 1,
};

const int base[] = {
    _BUF_GAME_PF2_RIGHT, _BUF_GAME_PF2_RIGHT, _BUF_GAME_PF1_RIGHT, _BUF_GAME_PF1_RIGHT, _BUF_GAME_PF0_RIGHT,
    _BUF_GAME_PF2_LEFT,  _BUF_GAME_PF2_LEFT,  _BUF_GAME_PF1_LEFT,  _BUF_GAME_PF1_LEFT,  _BUF_GAME_PF0_LEFT,
};

void setScoreCycle(enum SCORE_MODE cycle) {
    scoreCycle = cycle;
    forceScoreDraw = SCOREVISIBLETIME;
}


const unsigned char _DIGIT_SHAPE[] = {


    0b00100100,
    0b00101110,
    0b01101110,
    0b01101110,
    0b01101010,
    0b01101010,
    0b00101010,
    0b00101010,
    0b00101010,
    0b00101010,
    0b00101010,
    0b00101010,
    0b00101010,
    0b00101010,
    0b00101110,
    0b00101110,
    0b00101110,
    0b00101110,
    0b00100100,
    0b00000000,


    0b01000100,
    0b11101110,
    0b11101110,
    0b11101110,
    0b10101010,
    0b00100010,
    0b00100010,
    0b01100010,
    0b01000110,
    0b01101110,
    0b01101100,
    0b00101000,
    0b00101000,
    0b10101010,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b01001110,
    0b00000000,


    0b11101000,
    0b11101000,
    0b11101000,
    0b10101010,
    0b10001010,
    0b10001010,
    0b11001010,
    0b11101010,
    0b11101010,
    0b11101110,
    0b00101110,
    0b00101110,
    0b10101110,
    0b10100010,
    0b11100010,
    0b11100010,
    0b11100010,
    0b11100010,
    0b01000010,
    0b00000000,


    0b11100100,
    0b11101110,
    0b11101110,
    0b11101110,
    0b10101010,
    0b00101000,
    0b00101100,
    0b00101110,
    0b00101110,
    0b01101110,
    0b01001010,
    0b01001010,
    0b01001010,
    0b01001010,
    0b01001110,
    0b01001110,
    0b01001110,
    0b01001110,
    0b01000100,
    0b00000000,


    0b01000100,
    0b11101110,
    0b11101110,
    0b11101110,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101110,
    0b11100100,
    0b11100100,
    0b01101110,
    0b00101010,
    0b00101010,
    0b10101010,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b01000100,
    0b00000000,


    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000


    //    0b01000100
    //    0b01000100
    //    0b01000100
    //    0b01000000
    //    0b01001110
    //    0b01001110
    //    0b01001110
    //    0b01001110
    //    0b01000100
    //    0b01000100
    //    0b01000100
    //    0b00000100
    //    0b00001010
    //    0b00001010
    //    0b01001010
    //    0b01001010
    //    0b01001010
    //    0b00000000
    //    0b00000000
    //    0b00000000


    0b00000100,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00001110,
    0b00001110,
    0b00001110,
    0b00001110,
    0b00001110,
    0b00001110,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000100,
    0b00000000,
    0b00000000,


    0b11000100,
    0b11100100,
    0b11100100,
    0b11101110,
    0b10101110,
    0b10101010,
    0b10101010,
    0b11101010,
    0b11001010,
    0b11101110,
    0b10101110,
    0b10101110,
    0b10101110,
    0b10101110,
    0b10101110,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11001010,
    0b00000000,


    0b10000110,
    0b11000110,
    0b11001110,
    0b11101110,
    0b11101100,
    0b10101000,
    0b10101000,
    0b10101000,
    0b10101000,
    0b10101000,
    0b10101000,
    0b10101000,
    0b10101010,
    0b10101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11000110,
    0b00000000,


    0b11101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b10001000,
    0b10001000,
    0b10001100,
    0b10001100,
    0b11001100,
    0b11001100,
    0b11001000,
    0b11001000,
    0b10001000,
    0b10001110,
    0b10001110,
    0b10001110,
    0b10001110,
    0b10001110,
    0b10001110,
    0b00000000,


    0b10100100,
    0b10101110,
    0b10101110,
    0b10101110,
    0b10101010,
    0b10101000,
    0b10101000,
    0b10101000,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b10101010,
    0b10101010,
    0b10101110,
    0b10101110,
    0b10101110,
    0b10101110,
    0b10100110,
    0b00000000,

    0b00101110,
    0b00101110,
    0b00101110,
    0b00101110,
    0b00100100,
    0b00100100,
    0b00100100,
    0b00100100,
    0b00100100,
    0b00100100,
    0b00100100,
    0b00100100,
    0b10100100,
    0b10101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b01001110,
    0b00000000,

    0b10001010,
    0b10001010,
    0b10001010,
    0b10001010,
    0b10001010,
    0b10001010,
    0b10001110,
    0b10001100,
    0b10001100,
    0b10001100,
    0b10001100,
    0b10001110,
    0b10101110,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b00000000,

    //    0b11001010
    //    0b11001010
    //    0b11001010
    //    0b11101110
    //    0b11101110
    //    0b11101110
    //    0b11101110
    //    0b10101110
    //    0b10101110
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b10101010

    0b00101010,
    0b00101010,
    0b10101010,
    0b10101110,
    0b10101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10001010,
    0b10001010,
    0b10001010,
    0b00000000,


    0b11000100,
    0b11101110,
    0b11101110,
    0b11101110,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b11001010,
    0b10001110,
    0b10001110,
    0b10001110,
    0b10001110,
    0b10000100,
    0b00000000,

    0b11000100,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101010,
    0b10101010,
    0b10101010,
    0b11101010,
    0b11101010,
    0b11001010,
    0b11001010,
    0b11101010,
    0b11101000,
    0b10101010,
    0b10101011,
    0b10101111,
    0b10101111,
    0b10101101,
    0b10100101,
    0b00000000,

    0b11100110,
    0b11101110,
    0b11101110,
    0b11101110,
    0b11101010,
    0b11001000,
    0b01001100,
    0b01001110,
    0b01001110,
    0b01000110,
    0b01000010,
    0b01000010,
    0b01000010,
    0b01001110,
    0b01001110,
    0b01001110,
    0b01001110,
    0b01001110,
    0b01001100,
    0b00000000,

    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b11101010,
    0b11101010,
    0b11101110,
    0b01001110,
    0b01001110,
    0b01001110,
    0b01001110,
    0b01000100,
    0b00000000,

    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b10101010,
    0b11101010,
    0b11101010,
    0b01001010,
    0b01001010,
    0b01001010,
    0b11101110,
    0b11101110,
    0b10101110,
    0b10101110,
    0b10101110,
    0b10101010,
    0b10101010,
    0b10101010,
    0b00000000,

    0b11101010,
    0b11101010,
    0b11101010,
    0b11101010,
    0b00101010,
    0b00101010,
    0b00101110,
    0b00101110,
    0b01101110,
    0b11101110,
    0b11100100,
    0b11000100,
    0b10000100,
    0b10000100,
    0b10000100,
    0b10000100,
    0b11100100,
    0b11100100,
    0b11100100,
    0b00000000,

    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b00000000
    //    0b11001100
    //    0b11101110
    //    0b11101110
    //    0b10101110
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b10101010
    //    0b11101010
    //    0b11001010
    //    0b10000000
    //    0b10000000
    //    0b10000000


};


static unsigned char bigDigitBuffer[DIGIT_SIZE];

void drawBigDigit(int digit, int pos, int offset, int colour, bool blackBackground) {

    // colour 0x80 --> don't draw bottom blacknmess
    // colour 0x40 --> don't draw top blackness

    if (digit == DIGIT_SPACE)
        return;

    unsigned char pmask = mask[pos];
    unsigned char *p = RAM + base[pos] + offset;

    int shift2 = pmask == 0x0F ? 0 : 4;

    int shift = (digit & 1) << 2;
    digit >>= 1;

    if (!mirror[pos])
        shift2 ^= 4;

    if (offset && blackBackground && !(colour & 0x40)) {    // offset && (!digit & 0x40)) {
        p[0] &= pmask;
        p[1] &= pmask;
        p[2] &= pmask;
        p += 3;
    }

    const unsigned char *dig;
    if (!(digit & 0x40)) {
        dig = _DIGIT_SHAPE + digit * DIGIT_SIZE;
    } else
        dig = bigDigitBuffer;

    unsigned char rdl2;
    if (mirror[pos]) {
        for (int line = 0; line < DIGIT_SIZE; line++) {

            rdl2 = ((dig[line] >> shift) & 0xF) << shift2;
            p[line] = (p[line] & reverseBits[(unsigned char)~rdl2]) | reverseBits[rdl2];
        }
    } else {
        for (int line = 0; line < DIGIT_SIZE; line++) {

            rdl2 = ((dig[line] >> shift) & 0xF) << shift2;
            p[line] = (p[line] & ~rdl2) | rdl2;
        }
    }

    if (offset && blackBackground && !(colour & 0x80)) {    // offset && !(digit & 0x40)) {
        p += DIGIT_SIZE;
        p[0] &= pmask;
        p[1] &= pmask;
        p[2] &= pmask;
    }
}

void fillBit(int line, unsigned char b) {
    unsigned char *bdp = bigDigitBuffer + (line << 2);
    bdp[0] = bdp[1] = bdp[2] = bdp[3] = b;
}

void doubleSizeScore(int x, int y, int letter, int col) {


    static int let = 0;
    let++;

    if (let >= 10 << 4)
        let = 0;

    letter = (let >> 4);

    col = (let >> 4) & 7;
    x = 3;
    y = 120;

    extern unsigned char charAtoZ[];
    unsigned char *sample = charAtoZ + (letter + 26) * 10;

    for (int line = 0; line < 5; line++)
        fillBit(line, sample[line]);

    drawBigDigit(0x80, x, y, 0x80 | col, false);
    drawBigDigit(0x81, x + 1, y, 0x80 | col, false);

    for (int line = 0; line < 5; line++)
        fillBit(line, sample[line + 5]);

    drawBigDigit(0x80, x, y + DIGIT_SIZE, 0x40 | col, false);
    drawBigDigit(0x81, x + 1, y + DIGIT_SIZE, 0x40 | col, false);
}

unsigned char *drawDecimal2(unsigned char *buffer, unsigned char *colour_buffer, unsigned int colour, int cvt) {

    int forced = 0;
    for (int digit = 2; digit >= 0; digit--) {

        int displayDigit = 0;
        while (cvt >= pwr[digit]) {
            displayDigit++;
            cvt -= pwr[digit];
        }

        forced |= displayDigit;

        if (forced || !digit) {
            *buffer++ = displayDigit;

            if (colour_buffer)
                *colour_buffer++ = colour;
        }
    }

    return buffer;
}

// clang-format off

const char *planetName[] = {

    "MERCURY",
    "VENUS",
    "EARTH",
    "MARS",
    "JUPITER",
    "SATURN",
    "URANUS",
    "NEPTUNE",
    "PLUTO",
    "X"
};

// clang-format on

void drawPlanetName() {

    static int col = 0;
    col++;

    int caveCount = sizeof(planetName) / sizeof(planetName[0]);

    const char *p = planetName[cave < caveCount ? cave : caveCount - 1];
    for (int i = 2; *p; i++) {
        scoreLineNew[i] = LETTER(*p++);
        scoreLineColour[i] = col++ & 7;
    }
}

void drawSpeedRun() {

    static int speedCycle;
    speedCycle -= 12;
    for (int i = 0; i < 8; i++) {
        scoreLineNew[i + 1] = LETTER("SPEEDRUN"[i]);
        scoreLineColour[i + 1] = ((speedCycle + i * 0x30) >> 8) & 7;
    }
}

void drawDoge() {

    if (doges < 0) {
        scoreLineNew[1] = DIGIT_PLUS;
        scoreLineColour[1] = scoreLineColour[0] = rangeRandom(8);
    }

    int offset = 2;
    if (RAM[_P0_X] < 80)
        offset = 8;

    drawDecimal2(scoreLineNew + offset, scoreLineColour + offset, rangeRandom(8), doges < 0 ? -doges : doges);
}

void drawTime() {

    int tPos = 0;    // time >= 0xA00 ? time >= 0x6400 ? 5 : 6 : 7;

    scoreLineNew[tPos] = LETTER('T');
    scoreLineColour[tPos++] = RGB_BLUE;

    if (time > 0xA00 || ++toggle & 16)
        drawDecimal2(scoreLineNew + tPos, scoreLineColour + tPos, time < 0xA00 ? RGB_RED : RGB_AQUA, time >> 8);
}

void drawLives() {

    scoreLineNew[1] = LETTER('L');
    scoreLineColour[1] = RGB_BLUE;

    drawDecimal2(scoreLineNew + 2, scoreLineColour + 2, RGB_YELLOW, lives);
}

void drawTheScore(int score) {

    int notLeadingZero = 0;
    for (int digit = 5; digit >= 0; digit--) {

        int displayDigit = 0;
        while (score >= pwr[digit]) {
            displayDigit++;
            score -= pwr[digit];
        }

        notLeadingZero |= displayDigit;

        if (!digit || notLeadingZero) {
            scoreLineNew[9 - digit] = displayDigit;
            scoreLineColour[9 - digit] = RGB_YELLOW;    // digit + 1;
        }
    }
}

void drawScore() {

    static int scc = 0;
    scc++;

    if (!--forceScoreDraw) {
        forceScoreDraw = SCOREVISIBLETIME;
        if (++scoreCycle >= SCORELINE_END)
            scoreCycle = playerDead ? SCORELINE_LIVES : SCORELINE_START;
    }

    if (!exitMode && !playerDead && time < 0xA00)
        setScoreCycle(SCORELINE_TIME);

    else {

        // occasionally show score when idle
        if (idleTimer > IDLE_TIME) {
            idleTimer = 0;
            setScoreCycle(SCORELINE_SCORE);
        }
    }

    for (int i = 0; i < 10; i++)
        scoreLineNew[i] = DIGIT_SPACE;


    switch (scoreCycle) {
    case SCORELINE_TIME:
    case SCORELINE_SCORE:
        drawDoge();
        break;
    case SCORELINE_LIVES:
        drawLives();
        break;
    case SCORELINE_CAVELEVEL:
        drawPlanetName();
        break;
    case SCORELINE_SPEEDRUN:
        if (!theCave->dogeRequired[level] && !playerDead)
            drawSpeedRun();
        else
            setScoreCycle(SCORELINE_LIVES);
        break;

    default:
        break;
    }

    for (int i = 0; i < 10; i++) {
        drawBigDigit(scoreLineNew[i], 9 - i, 14, 7, false);
    }
}

// EOF