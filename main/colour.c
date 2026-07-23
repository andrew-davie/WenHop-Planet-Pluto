#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

// #include "characterset.h"
#include "colour.h"
#include "main.h"
#include "random.h"
#include "savekey.h"
#include "scroll.h"

static unsigned char bgPalette[_BOARD_ROWS];
static unsigned char fgPalette[2];

static int lastPfCharLine;
static int lastBgCharLine;
static int currentPalette;
int luminance;
int lumTarget = 0;

static int flashTime = 1;

int roller;


void interleaveChronoColour(int *r) {
    if (++*r > 1)    // saveKeyEnableICC)
        *r = 0;
}


const unsigned char TranslateColour[] = {0x00, 0x20, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0,
                                         0xD0, 0xB0, 0x90, 0x70, 0x70, 0x50, 0x20, 0x40};

const unsigned char TranslateSecamColour[] = {0, 0xC, 0xC, 4, 4, 6, 6, 2, 2, 2, 8, 8, 8, 8, 0xC, 0xC};

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


unsigned char adjustBrightness(unsigned char colour) {

    // Remember if iCC is operating, we will see blended colours (with black/no pixel)
    // So even if we set all white, we WILL see shades of grey.  So we can fade to black OK
    // But we cannot fade to white - only fade to bright.

    int lum = (colour & 0xF) + luminance;

    if (lum < 0) {
        lum = 0;
        colour = 0;
    }

    if (lum > 14) {
        colour = 0;
        lum = 14;
    }

    return luminance == -15 ? 0 : (colour & 0xF0) | lum;
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

    return adjustBrightness(colour);
}

void pulseBackgroundColour(unsigned char colour, int ftime) {
    colubk = convertColour(colour);
    flashTime = ftime;
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

// const unsigned char pfColour[] = {0xA8, 0x28, 0xC8, 0xC8};    //{0x88, 0x28, 0xD8, 0xD8};

void setPFColours(const unsigned char *palette, unsigned char *colourBuffer) {

    unsigned char pfConvertedColour[3];
    for (int i = 0; i < 3; i++)
        pfConvertedColour[i] = convertColour(palette[i]);


    for (int i = 0; i < _SCANLINES; i++) {
        if (++roller > 2)
            roller = 0;
        colourBuffer[i] = pfConvertedColour[roller];
    }
}

void adjustLuminance(int speed) {

    static signed char lumSpeed = 0;

    if (--lumSpeed < 0) {
        luminance = approach(luminance, lumTarget, 1);
        lumSpeed = speed;
    }
}


#define FRANGE (0x100 / (_BOARD_ROWS - 1))
void setBackgroundPalette(unsigned char *cx) {

    unsigned char c[4];
    for (int i = 0; i < 4; i++)
        c[i] = cx[i];


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


void setPalette(int buf) {

    unsigned char bgCol = colubk;
    const int shift = 16;

    int i = 0;
    unsigned char *pfCol = RAM + buf;
    unsigned char *bkCol = RAM + buf;

    int bgCharLine = (scrollY >> 16) * 3;
    int pfCharLine = 0;

    while (bgCharLine >= CHAR_Y) {
        bgCharLine -= CHAR_Y;
        pfCharLine++;
    }

    lastBgCharLine = bgCharLine;
    lastPfCharLine = pfCharLine;

    unsigned char rollColour[5];

    rollColour[0] = rollColour[3] = bgPalette[pfCharLine];
    rollColour[1] = rollColour[4] = fgPalette[1];
    rollColour[2] = fgPalette[0];

    int roll = roller;
    if (saveKeyEnableICC && --roll < 0)
        roll = 2;

    static const unsigned char lavaColour[] = {0x28, 0x2A, 0x26, 0x28};
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
            rollColour[0] = rollColour[3] = bgPalette[++pfCharLine];
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

    // 0
    {0x04, 0x08, 0x0A, 0x0A},    //
    {0x06, 0x34, 0x96, 0xB6},    //
    {0x06, 0xD4, 0xA6, 0x96},    //
    {0x08, 0x04, 0x0A, 0x0A},    //
    {0x08, 0x46, 0x52, 0x56},    //
    {0x0A, 0x36, 0xA4, 0xA6},    //

    // 1
    {0x14, 0xA4, 0x04, 0x06},    //
    {0x16, 0x96, 0x08, 0x06},    //

    // 2
    {0x26, 0x36, 0xA8, 0x46},    //
    {0x26, 0x46, 0x98, 0x66},    //
    {0x26, 0x66, 0x98, 0xB8},    //
    {0x26, 0xD4, 0x54, 0x06},    //
    {0x26, 0xD4, 0x84, 0xE6},    //
    {0x28, 0x06, 0x52, 0x56},    //
    {0x2A, 0xA8, 4, 6},          //

    // 3
    {0x34, 0x56, 0xC6, 0x06},    //
    {0x34, 0xD4, 0x54, 0x06},    //
    {0x36, 0x94, 0xC6, 0xb6},    //
    {0x36, 0xA6, 0x54, 0x56},    //
    {0x36, 0xD6, 0x56, 0x56},    //
    {0x38, 0xA6, 0x66, 0x46},    //
    {0x38, 0xD6, 0x46, 0xC6},    //
    {0x3A, 0x96, 0x46, 0x46},    //
    {0x3A, 0xA6, 0x56, 0x56},    //

    // 4
    {0x46, 0xA6, 0xC6, 0xE6},    //
    {0x46, 0xD6, 0xB8, 0x88},    //
    {0x46, 0x96, 0xC6, 0xC6},    //
    {0x46, 0xA6, 0xA6, 0xA6},    //
    {0x48, 0xA6, 0xC6, 0xC6},    //
    {0x4A, 0x96, 0xA6, 0xA6},    //
    {0x4A, 0xB6, 0x66, 0x66},    //

    // 5
    {0x50, 0x08, 0x06, 0x06},    // interesting
    {0x56, 0x26, 0xD6, 0xC6},    //
    {0x56, 0xC6, 0x36, 0x26},    //
    {0x58, 0xD8, 0x28, 0xC6},    //

    // 6
    {0x66, 0xB6, 0xC6, 0xC6},    //
    {0x68, 0x76, 0x56, 0x56},    //

    // 7
    {0x78, 0xC6, 0x26, 0xB6},    //

    // 8
    {0x88, 0xD6, 0x16, 0x16},    //

    // 9
    {0x96, 0x26, 0xD6, 0xD6},    //
    {0x96, 0x36, 6, 6},          //
    {0x96, 0x26, 0xF6, 0xC6},    //
    {0x96, 0x28, 0x56, 0x56},    //
    {0x96, 0x46, 0xA6, 0xA6},    //
    {0x98, 0x26, 6, 6},          //
    {0x98, 0x26, 0x86, 0x46},    //

    // A
    {0xA6, 0xC6, 0x36, 0x36},    //
    {0xA6, 0x26, 0x36, 0x56},    //
    {0xA6, 0x26, 0xF6, 0xC6},    //
    {0xA6, 0x26, 0xD6, 0xD6},    //
    {0xA6, 0x36, 0x56, 0x56},    //
    {0xA6, 0x48, 0xA6, 0xA6},    //
    {0xA6, 0xC6, 0x46, 0x26},    //
    {0xA8, 0x26, 6, 6},          //
    {0xA8, 0x26, 0x56, 0x36},    //
    {0xA8, 0x36, 0x96, 0x96},    //
    {0xA8, 0xC6, 0x26, 0x96},    //

    // B
    {0xB6, 0x26, 0x36, 0x36},    //
    {0xB6, 0x28, 0xB6, 0xB6},    //

    // C
    {0xC6, 0x28, 0xB8, 0xB8},    //

    // D
    {0xD6, 0x36, 0xA6, 0x76},    //
    {0xD6, 0x36, 18, 18},        //
    {0xD6, 0xD6, 0x56, 0xA6},    //
    {0xD8, 0x56, 0x46, 0x36},    //
    {0xD8, 0xD6, 0x36, 0xC6},    //

    // E
    {0xEA, 0x46, 0x96, 0x96},    //

    // F
};


void loadPalette() {

    lumTarget = 0;

    unsigned char *c = (unsigned char *)colourPool;
    currentPalette = rangeRandom(sizeof(colourPool) / sizeof(colourPool[0]));
    setBackgroundPalette(c + (currentPalette << 2));
}

// EOF