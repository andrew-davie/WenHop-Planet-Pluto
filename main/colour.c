#include <stdbool.h>

#include "colour.h"
#include "main.h"
#include "savekey.h"

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

const unsigned char pfColour[] = {0x94, 0x22, 0xD4};

void setPFColours(unsigned char *colours) {


    int roll = roller;

    // static int c;
    // if (!(frame & 255))
    //     c = (getRandom32() & 0xF0) | 4;


    unsigned char pfConvertedColour[3];
    for (int i = 0; i < 3; i++)
        pfConvertedColour[i] = convertColour(pfColour[i]);

    //    pfConvertedColour[1] = c;


    for (int i = 0; i < _SCANLINES; i++) {
        if (++roll > 2)
            roll -= 3;
        colours[i] = pfConvertedColour[roll];
    }
}


// EOF