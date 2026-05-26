#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "characterset.h"
#include "colour.h"
#include "main.h"
#include "savekey.h"
#include "scroll.h"


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

    unsigned char pfConvertedColour[3];
    for (int i = 0; i < 3; i++)
        pfConvertedColour[i] = convertColour(pfColour[i]);


    for (int i = 0; i < _SCANLINES; i++) {
        if (++roller > 2)
            roller = 0;
        colours[i] = pfConvertedColour[roller];
    }
}

// void setPalette() {

//   int shift = 16;

// //  interleaveColour();

//   unsigned char bgCol = 0; // tmp flashTime ? ARENA_COLOUR : 0;

//   int i = 0;
//   unsigned char *pfCol = RAM + _BUF_GAME_COLUPF;
//   unsigned char *bkCol = RAM + _BUF_GAME_COLUBK;

//   int bgCharLine = (scrollY >> shift) * 3;
//   int pfCharLine = 0;

//   while (bgCharLine >= PIECE_DEPTH) {
//     bgCharLine -= PIECE_DEPTH;
//     pfCharLine++;
//   }

//   lastBgCharLine = bgCharLine;
//   lastPfCharLine = pfCharLine;

//   unsigned char rollColour[5];

//   rollColour[0] = rollColour[3] = fgPalette[1];
//   rollColour[1] = rollColour[4] = bgPalette[pfCharLine];
//   rollColour[2] = fgPalette[0];

//   int roll = roller;
//   if (interleavedColour && --roll < 0)
//     roll = 2;

//   static unsigned char lavaColour[] = {0x28, 0x2A, 0x26, 0x28};
//   static const unsigned char waterColour[] = {0x88, 0x78, 0xC8, 0x88};

//   static const unsigned char wbg[] = {0x96, 0x96, 0x96, 0x94, 0x94, 0x94,
//                                       0x92, 0x92, 0x90, 0x90, 0x90};

//   static const unsigned char lbg[] = {0x48, 0x48, 0x46, 0x46, 0x44, 0x44,
//                                       0x42, 0x42, 0x42, 0x42, 0x42};

//   int lavaLine = (lavaSurfaceTrixel - (scrollY >> shift)) * 3;
//   int lavab = 0;
//   if (lavaLine < 0)
//     lavab = -lavaLine;
//   if (lavab >= (10 << 2))
//     lavab = (10 << 2);

//   while (i < lavaLine && i < _SCANLINES) {

//     pfCol[0] = rollColour[roll];
//     pfCol[1] = rollColour[roll + 1];
//     pfCol[2] = rollColour[roll + 2];

//     bkCol[0] = bgCol;
//     bkCol[1] = bgCol;
//     bkCol[2] = bgCol;

//     pfCol += 3;
//     bkCol += 3;

//     bgCharLine += 3;
//     if (bgCharLine >= PIECE_DEPTH) {
//       bgCharLine = 0;
//       rollColour[1] = rollColour[4] = bgPalette[++pfCharLine];
//     }

//     i += 3;
//   }

//   const unsigned char *cl = showLava ? &lbg[0] : &wbg[0];
//   const unsigned char *clava = showLava ? &lavaColour[0] : &waterColour[0];

//   while (i < _SCANLINES) {

//     unsigned char lbgCol = cl[(lavab >> 2)];

//     if (lavab < (10 << 2))
//       lavab += 3;

//     pfCol[0] = clava[roll];
//     pfCol[1] = clava[roll + 1];
//     pfCol[2] = clava[roll + 2];

//     bkCol[0] = lbgCol;
//     bkCol[1] = lbgCol;
//     bkCol[2] = lbgCol;

//     pfCol += 3;
//     bkCol += 3;

//     i += 3;
//   }
// }

// EOF