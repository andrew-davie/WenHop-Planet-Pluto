#include "defines.h"
#include "defines_cdfj.h"

#include "main.h"

#include "colour.h"

static int lastBgCol;
// static int lastPfCharLine;
// static int lastBgCharLine;

int roller;

void initColours() {
    lastBgCol = 0xFF;
    //    lastPfCharLine = -1;
    //    lastDisplayMode = DISPLAY_NONE;
    roller = 0;
}

void interleaveColour() {
    if (++roller > 2 || !enableICC)
        roller = 0;
}

static const unsigned char xlate[] = {0x00, 0x0,  0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0,
                                      0xD0, 0xB0, 0x90, 0x70, 0x50, 0x30, 0x20, 0x40};

#if __ENABLE_SECAM
static const unsigned char xlateSecam[] = {0, 0xE, 0xC, 4, 4, 6, 6, 2, 2, 2, 8, 8, 8, 8, 0xC, 0xC};

int secamConvert(unsigned char col) {

    int c = xlateSecam[col >> 4];

    if ((col & 0xF) >= 4) {
        if (!c) // black -> white
            c = 0xE;
        else if (c == 2) // blue -> aqua
            c = 0xA;
    }
    return c;
}
#endif

int convertColour(unsigned char colour) {

    switch (mm_tv_type) {

#if __ENABLE_SECAM
    case SECAM: {
        colour = secamConvert(colour);
        break;
    }
#endif

    case PAL:
    case PAL_60:
//        colour = xlate[colour >> 4] | (colour & 0xF);
        break;

    default:
        break;
    }

    return colour;
}

void setFlash2(unsigned char col, int time) {
    ARENA_COLOUR = convertColour(col);
    flashTime = time;
}

void doFlash() {

    if (!--flashTime)
        if (mm_tv_type == SECAM || (flashTime = ARENA_COLOUR-- & 0xF) == 1)
            ARENA_COLOUR = 1;
}

#define FRANGE (0x100 / 22)
void setBackgroundPalette(const unsigned char *c) {

#if __ENABLE_SECAM
    if (mm_tv_type == SECAM)
        for (int bgLine = 0; bgLine < 22; bgLine++)
            bgPalette[bgLine] = convertColour(c[2]);

    else
#endif
    {

        int c1 = c[2] & 0xF0;
        int c2 = c[3] & 0xF0;
        int i1 = c[2] & 0xF;
        int i2 = c[3] & 0xF;

        int cStep = (c2 - c1) * FRANGE;
        int iStep = (i2 - i1) * FRANGE;

        i1 = i1 << 8;
        c1 = c1 << 8;

        for (int bgLine = 0; bgLine < 22; bgLine++) {

            bgPalette[bgLine] = convertColour(((c1 >> 8) & 0xF0) | (i1 >> 8));

            i1 += iStep;
            c1 += cStep;
        }
    }

    fgPalette[0] = convertColour(c[0]);
    fgPalette[1] = convertColour(c[1]);
}

void setPalette() {}

void loadPalette() {

    const unsigned char *c = EXTERNAL(__COLOUR_POOL);
    c += currentPalette; // & 60;
    setBackgroundPalette(c);
}

// EOF