#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "characterset.h"
#include "colour.h"
#include "main.h"
#include "random.h"
#include "savekey.h"
#include "scroll.h"

unsigned char bgPalette[_BOARD_ROWS];
unsigned char fgPalette[2];

// static int lastBgCol;
static int lastPfCharLine;
static int lastBgCharLine;
int currentPalette;

int flashTime = 1;

// static int lastBgCol;
int openSlot;
int roller;


void interleaveChronoColour(int *r) {
    if (++*r > saveKeyEnableICC)
        *r = 0;
}

unsigned char TranslateColour[] = {0x00, 0x20, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0,
                                   0xD0, 0xB0, 0x90, 0x70, 0x70, 0x50, 0x20, 0x40};

unsigned char TranslateSecamColour[] = {0, 0xE, 0xC, 4, 4, 6, 6, 2, 2, 2, 8, 8, 8, 8, 0xC, 0xC};

unsigned char secamConvert(unsigned char col) {
    unsigned char c = TranslateSecamColour[col >> 4];

    if ((col & 0xF) >= 4) {
        if (!c)    // black -> white
            c = 0xE;
        else if (c == 2)    // blue -> aqua
            c = 0xA;
    }
    return c;
}

unsigned char convertColour(unsigned char colour) {
    switch (tvSystem) {

    case _TV_SYSTEM_SECAM: {
        colour = secamConvert(colour);
        return colour;
        break;
    }

    case _TV_SYSTEM_PAL:
    case _TV_SYSTEM_PAL60:
        colour = TranslateColour[colour >> 4] | (colour & 0xF);
        break;

    default:
        break;
    }

    return colour;
}

void pulseBackgroundColour(unsigned char colour, int time) {
    colubk = convertColour(colour);
    flashTime = time;
}


void fadeBackgroundColour() {

    if (flashTime) {
        if (!--flashTime) {

            if (!(colubk & 0xF) || tvSystem == _TV_SYSTEM_SECAM)
                colubk = 0;
            else {

                colubk--;
                flashTime = 1;
            }
        }
    }
}

const unsigned char pfColour[] = {0x96, 0x24, 0xD6};

void setPFColours(unsigned char *colours) {

    unsigned char pfConvertedColour[3];
    for (int i = 0; i < 3; i++)
        pfConvertedColour[i] = convertColour(pfColour[i]);


    for (int i = 0; i < _SCANLINES; i++) {
        if (++roller > 2)
            roller = 0;
        colours[i] = pfConvertedColour[roller];
    }
}

#define FRANGE (0x100 / _BOARD_ROWS)
void setBackgroundPalette(unsigned char *c) {

    if (_tvSystem == _TV_SYSTEM_SECAM) {
        int colour = convertColour(c[2]);
        for (int bgLine = 0; bgLine < _BOARD_ROWS; bgLine++)
            bgPalette[bgLine] = colour;
    }

    else {

        int c1 = c[2] & 0xF0;
        int c2 = c[3] & 0xF0;
        int i1 = c[2] & 0xF;
        int i2 = c[3] & 0xF;

        int cStep = (c2 - c1) * FRANGE;
        int iStep = (i2 - i1) * FRANGE;

        i1 = i1 << 8;
        c1 = c1 << 8;

        for (int bgLine = 0; bgLine < _BOARD_ROWS; bgLine++) {

            bgPalette[bgLine] = convertColour(((c1 >> 8) & 0xF0) | (i1 >> 8));

            i1 += iStep;
            c1 += cStep;
        }
    }

    fgPalette[0] = convertColour(c[0]);
    fgPalette[1] = convertColour(c[1]);
}


void setPalette() {

    int shift = 16;

    //  interleaveColour();

    unsigned char bgCol = colubk;    // tmp flashTime ? ARENA_COLOUR : 0;

    int i = 0;
    unsigned char *pfCol = RAM + _BUF_GAME_COLUPF;
    unsigned char *bkCol = RAM + _BUF_GAME_COLUBK;

    int bgCharLine = (scrollY >> shift) * 3;
    int pfCharLine = 0;

    while (bgCharLine >= CHAR_Y) {
        bgCharLine -= CHAR_Y;
        pfCharLine++;
    }

    lastBgCharLine = bgCharLine;
    lastPfCharLine = pfCharLine;

    unsigned char rollColour[5];

    rollColour[0] = rollColour[3] = fgPalette[1];
    rollColour[1] = rollColour[4] = bgPalette[pfCharLine];
    rollColour[2] = fgPalette[0];

    int roll = roller;
    if (saveKeyEnableICC && --roll < 0)
        roll = 2;

    static unsigned char lavaColour[] = {0x28, 0x2A, 0x26, 0x28};
    static const unsigned char waterColour[] = {0x88, 0x78, 0xC8, 0x88};

    static const unsigned char wbg[] = {0x96, 0x96, 0x96, 0x94, 0x94, 0x94, 0x92, 0x92, 0x90, 0x90, 0x90};

    static const unsigned char lbg[] = {0x48, 0x48, 0x46, 0x46, 0x44, 0x44, 0x42, 0x42, 0x42, 0x42, 0x42};

    int lavaLine = (lavaSurfaceTrixel - (scrollY >> shift)) * 3;
    int lavab = 0;
    if (lavaLine < 0)
        lavab = -lavaLine;
    if (lavab >= (10 << 2))
        lavab = (10 << 2);

    while (i < lavaLine && i < _SCANLINES) {

        pfCol[0] = rollColour[roll];
        pfCol[1] = rollColour[roll + 1];
        pfCol[2] = rollColour[roll + 2];

        bkCol[0] = bgCol;
        bkCol[1] = bgCol;
        bkCol[2] = bgCol;

        pfCol += 3;
        bkCol += 3;

        bgCharLine += 3;
        if (bgCharLine >= CHAR_Y) {
            bgCharLine = 0;
            rollColour[1] = rollColour[4] = bgPalette[++pfCharLine];
        }

        i += 3;
    }

    const unsigned char *cl = showLava ? &lbg[0] : &wbg[0];
    const unsigned char *clava = showLava ? &lavaColour[0] : &waterColour[0];

    while (i < _SCANLINES) {

        unsigned char lbgCol = cl[(lavab >> 2)];

        if (lavab < (10 << 2))
            lavab += 3;

        pfCol[0] = clava[roll];
        pfCol[1] = clava[roll + 1];
        pfCol[2] = clava[roll + 2];

        bkCol[0] = lbgCol;
        bkCol[1] = lbgCol;
        bkCol[2] = lbgCol;

        pfCol += 3;
        bkCol += 3;

        i += 3;
    }
}


const unsigned char colourPool[][4] = {
    // clang-format off

    {0xC4, 0x28, 0xB8, 0xB8},    // 00
    {0x98, 0x24, 0x86, 0x46},    // 01
    {0x16, 0x96, 8, 2},          // 02
    {0xA6, 0x24, 0x32, 0x52},    // 03
    {0x88, 0xD6, 20, 20},        // 04
    {0xA6, 0xC4, 0x46, 0x22},    // 05
    {0x98, 0x24, 4, 4},          // 06
    {0x26, 0xD4, 0x84, 0xE4},    // 07

    /*;    dc 0xB6, 0x28, 0x82, 0x82
    ;    dc 10, 12, 8, 8

        ; 7
        dc 0x44, 0xA6, 0xC6, 0xE6
    ;    dc 0x4A, 0xB4, 0x64, 0x64 ;0x66, 0xB4, 0xC2, 0xC2
    ;    dc 10, 12, 8, 8

        ; 8
        dc 0x96, 0x24, 0xF6, 0xC6
    ;    dc 0x68, 0x76, 0x54, 0x54 ;0xB4, 0x24, 0x32, 0x32
    ;    dc 10, 12, 8, 8

        ; 9=J
        dc 0x26, 0x46, 0x98, 0x66
    ;    dc 0x96, 0x84, 0x82, 0x82
    ;    dc 10, 12, 8, 8

        ; 10=K
        dc 0x36, 0x94, 0xC6, 0xb4
     ;   dc 0x46, 0x32, 0xA2, 0xA2
     ;   dc 10, 12, 8, 8

        ; 11=L
        dc 0x06, 0x34, 0x96, 0xB4
     ;   dc 0xEA, 0x46, 0x94, 0x94
     ;   dc 10, 12, 8, 8

        ; 12
        dc 0xD6, 0x34, 0xA6, 0x74
     ;   dc 0x96, 0x46, 0xA2, 0xA2
     ;   dc 8, 4, 10, 10

        ; 13
        dc 0x38, 0xD4, 0x44, 0xC4
     ;   dc 0x66, 0x72, 0x92, 0x92
     ;   dc 10, 8, 12, 12

        ; 14 tan rock purple soil aqua doge good glint
        dc 0x38, 0xA4, 0x64, 0x44
     ;   dc 0x96, 0x68, 0x92, 0x92 ;0x66, 0x72, 0x92, 0x92
     ;   dc 4, 8, 10, 10

        ; 15 purple rock light blue doge green mortar good glint
        dc 0x78, 0xC4, 0x26, 0xB4
     ;   dc 0x4A, 0x94, 0xA4, 0xA4 ;0x66, 0x72, 0x92, 0x92
     ;   dc 0xA0, 0x08, 0x04, 0x04






    ;     ;0

    ; ;    dc $14, 0xA4, 4, 4
    ;     dc 0x2A, 0xA8, 4, 4
    ; ;    dc 12, 10, 14, 14

    ;     ;1
    ; ;    dc 0x56, 0x24, 0xD2, 0xC2
    ;     dc 0xA6, 0x26, 0xD4, 0xD4
    ; ;    dc 10, 12, 8, 8

    ;     ; 2
    ; ;    dc 0x58, 0yD8, 0x28, 0xC4
    ;     dc  0x08, 0x46, 0x52, 0x52 ;0yD6, 0x36, 18, 18
    ; ;    dc 6, 10, 12, 12

    ;     ; 3*
    ; ;    dc 0x56, 0yC4, 0x36, 0x22
    ;     dc 0x2A, 0x48, 0x34, 0x34 ;0xA4, 0xC4, 0x34, 0x34
    ; ;    dc 10, 12, 4, 4

    ;     ; 4
    ; ;    dc 0xA8, 0x24, 4, 4
    ;     dc 0x94, 0x36, 2, 2
    ; ;    dc 10, 12, 8, 8

    ;     ; 5*
    ;     dc 0x26, 0xD4, 0x54, 0x04     ; NTSC  BOO's preferred
    ; ;        dc 0x34, 0xA4, 0x52, 0x52
    ; ;    dc 0x34, 0xD4, 0x54, 0x04

    ; ;x    dc 0xA8, 0x36, 0x92, 0x92
    ; ;    dc 10, 12, 8, 8

    ;     ; 6
    ; ;    dc 0xA8, 0x24, 0x56, 0x36
    ;     dc 0x96, 0x28, 0x52, 0x52
    ; ;    dc 10, 12, 8, 8

    ;     ; 7
    ; ;    dc 0x34, 0x56, 0yC6, 0x06
    ;     dc 0x3A, 0x94, 0x44, 0x44 ;0x46, 0x94, 0yC2, 0yC2
    ; ;    dc 10, 12, 8, 8

    ;     ; 8
    ; ;    dc 0xA6, 0x24, 0yF6, 0yC6
    ;     dc 0x48, 0xA6, 0xC4, 0xC4 ;0x94, 0x24, 0xD2, 0xD2
    ; ;    dc 10, 12, 8, 8

    ;     ; 9=J
    ; ;    dc 0x26, 0x36, 0xA8, 0x46
    ;     dc 0xA6, 0x54, 0x52, 0x52
    ; ;    dc 10, 12, 8, 8

    ;     ; 10=K
    ; ;    dc 0xD6, 0xA4, 0yC6, 0x94
    ; ;    dc 0x36, 0xD2, 0x52, 0x52
    ;     dc 0x28, 0x06, 0x52, 0x52
    ; ;    dc 10, 12, 8, 8

    ;     ; 11=L
    ; ;    dc 0y06, 0xD4, 0xA6, 0x94
    ;     dc 0x0A, 0x36, 0xA4, 0xA4
    ; ;    dc 10, 12, 8, 8

    ;     ; 12
    ; ;    dc 0yD6, 0xD4, 0x56, 0xA4
    ;     dc 0xA6, 0x36, 0x52, 0x52
    ; ;    dc 8, 4, 10, 10

    ;     ; 13
    ;     dc 0xD8, 0xD4, 0x34, 0xC4
    ; ;x    dc 0x46, 0xA2, 0xA2, 0xA2
    ; ;    dc 10, 8, 12, 12

    ;     ; 14 tan rock purple soil aqua doge good glint
    ; ;    dc 0xD8, 0x54, 0x44, 0x34
    ;     dc ooCOMPATIBLE_COMPATIBLE_PALETTE + 0xA6, 0x48, 0xA2, 0xA2 ;0x46, 0xA2, 0xA2, 0xA2
    ; ;    dc 4, 8, 10, 10

    ;     ; 15 purple rock light blue doge green mortar googlint *
    ;     dc ooCOMPATIBLE_COMPATIBLE_PALETTE + 0xA8, 0xC4, 0x26, 0x94
    ; ;x    dc 0x3A, 0xA4, 0x54, 0x54 ;0x46, 0xA2, 0xA2, 0xA2
    ; ;    dc 0x50, 0y08, 0y04, 0y04

    */
    // clang-format on

};


void loadPalette() {

    //    unsigned char *c = (unsigned char *)colourPool;

    unsigned char rp[4];
    for (int i = 0; i < 4; i++)
        rp[i] = (getRandom32() & 0xF0) | 6;


    rp[3] += 2;
    //    currentPalette = 6;    // rangeRandom(sizeof(colourPool) / sizeof(colourPool[0]));    // tmp

    // if (currentPalette > sizeof(colourPool) / sizeof(colourPool[0]))
    //     currentPalette = 0;


    //  c += ((currentPalette) << 2);
    setBackgroundPalette(rp);
}

// EOF