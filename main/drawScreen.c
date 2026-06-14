#include <stdbool.h>

#include "cavedata.h"
#include "defines_dasm.h"


#include "cdfjplus.h"

#include "animations.h"
#include "attribute.h"
#include "caveData.h"
#include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "main.h"
#include "reverseBits.h"
#include "score.h"
#include "scroll.h"

extern int roller;

const unsigned char *img[5];
const unsigned char *corner[5];
const unsigned char *p;

unsigned char revectorChar[128];

void initCharVector() {

    for (int i = 0; i < 128; i++)
        revectorChar[i] = i;
}

// To create emoji comment...
// python3 tools/reformat_arrays.py main/score.c

const unsigned char _CHAR_INNER_CORNER_03[] = {

    0b00011, 0b00000, 0b00000,    // 0 |◼️◼️◼️🟫🟫|
    0b00001, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️🟫|
    0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 9 |◼️◼️◼️◼️◼️|
};

const unsigned char _CHAR_INNER_CORNER_06[] = {

    0b00000, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
    0b00011, 0b00000, 0b00000,    // 9 |◼️◼️◼️🟫🟫|
};

const unsigned char _CHAR_INNER_CORNER_07[] = {

    0b00001, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️🟫|
    0b00001, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️🟫|
    0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
    0b00011, 0b00000, 0b00000,    // 9 |◼️◼️◼️🟫🟫|
};

const unsigned char _CHAR_INNER_CORNER_09[] = {

    0b11000, 0b00000, 0b00000,    // 0 |🟫🟫◼️◼️◼️|
    0b10000, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️◼️|
    0b10000, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 9 |◼️◼️◼️◼️◼️|
};

const unsigned char _CHAR_INNER_CORNER_11[] = {

    0b11001, 0b00000, 0b00000,    // 0 |🟫🟫◼️◼️🟫|
    0b10001, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️🟫|
    0b10000, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 9 |◼️◼️◼️◼️◼️|
};

const unsigned char _CHAR_INNER_CORNER_12[] = {

    0b00000, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
    0b10000, 0b00000, 0b00000,    // 9 |🟫◼️◼️◼️◼️|
};

const unsigned char _CHAR_INNER_CORNER_13[] = {

    0b11000, 0b00000, 0b00000,    // 0 |🟫🟫◼️◼️◼️|
    0b10000, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️◼️|
    0b10000, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
    0b10000, 0b00000, 0b00000,    // 9 |🟫◼️◼️◼️◼️|
};

const unsigned char _CHAR_INNER_CORNER_14[] = {

    0b00000, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
    0b10011, 0b00000, 0b00000,    // 9 |🟫◼️◼️🟫🟫|
};

const unsigned char _CHAR_INNER_CORNER_15[] = {

    0b11011, 0b00000, 0b00000,    // 0 |🟫🟫◼️🟫🟫|
    0b10001, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️🟫|
    0b10001, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️🟫|
    0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
    0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
    0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
    0b10011, 0b00000, 0b00000,    // 9 |🟫◼️◼️🟫🟫|
};


const unsigned char *const roundedCorner[16] = {

    _CHAR_BLANK,              // 00
    _CHAR_BLANK,              // 01 U
    _CHAR_BLANK,              // 02 R
    _CHAR_INNER_CORNER_03,    // 03 RU
    _CHAR_BLANK,              // 04 D
    _CHAR_BLANK,              // 05 DU
    _CHAR_INNER_CORNER_06,    // 06 RD
    _CHAR_INNER_CORNER_07,    // 07 URD
    _CHAR_BLANK,              // 08 L
    _CHAR_INNER_CORNER_09,    // 09 LU
    _CHAR_BLANK,              // 10 LR
    _CHAR_INNER_CORNER_11,    // 11 LUR
    _CHAR_INNER_CORNER_12,    // 12 LD
    _CHAR_INNER_CORNER_13,    // 13 LDU
    _CHAR_INNER_CORNER_14,    // 14 LRD
    _CHAR_INNER_CORNER_15,    // 15 LURD
};


void grabCharacters() {

    enum ChName p2;
    enum ObjectType type;
    unsigned char udlr;

    for (int col = 0; col < 5; col++) {

        p2 = GET(p[col]);
        type = CharToType[p2];

        if (Animate[type])
            img[col] = charSet[revectorChar[*Animate[type]]];
        else
            img[col] = charSet[revectorChar[p2]];


        if (Attribute[type] & ATT_PAD) {

            udlr = ((Attribute[CharToType[GET(p[col - _1ROW])]] & ATT_CORNER) >> POS_CORNER |
                    ((Attribute[CharToType[GET(p[col + 1])]] & ATT_CORNER) >> (POS_CORNER - 1)) |
                    ((Attribute[CharToType[GET(p[col + _1ROW])]] & ATT_CORNER) >> (POS_CORNER - 2)) |
                    ((Attribute[CharToType[GET(p[col - 1])]] & ATT_CORNER) >> (POS_CORNER - 3)));

            corner[col] = roundedCorner[udlr];
        }

        else
            corner[col] = _CHAR_BLANK;
    }

    p += 4;
}

const unsigned char rollDirect[3][CHAR_Y] = {


    {2, 0, 1, 5, 3, 4, 8, 6, 7, 11, 9, 10, 14, 12, 13, 17, 15, 16, 20, 18, 19, 23, 21, 22, 26, 24, 25, 29, 27, 28},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
    {1, 2, 0, 4, 5, 3, 7, 8, 6, 10, 11, 9, 13, 14, 12, 16, 17, 15, 19, 20, 18, 22, 23, 21, 25, 26, 24, 28, 29, 27},
};


const unsigned int arenas[] = {

    _BUF_GAME_PF0_LEFT,
    _BUF_GAME_PF0_RIGHT,
};

void drawScreen() {    // --> cycles 62870 (@20230616)

    if (gameTick < 2)
        return;    // allow geodoge to coalesce


    int sY = scrollY;
    int sX = scrollX;

#if ENABLE_SHAKE

    sX += shakeX;
    if (sX < SCROLL_MIN_X)
        sX = SCROLL_MIN_X;
    if (sX > SCROLL_MAX_X)
        sX = SCROLL_MAX_X;

    sY += shakeY;
    if (sY < SCROLL_MIN_Y)
        sY = SCROLL_MIN_Y;
    if (sY > SCROLL_MAX_Y)
        sY = SCROLL_MAX_Y;

#endif

    int lcount = -(sY >> 16) * 3;
    // int shift = CHAR_TRIX_X - (((sX & 0xFFFF) * CHAR_TRIX_X) >> 16);


    int startRow = (-lcount * (0x10000 / CHAR_Y)) >> 16;
    lcount += startRow * CHAR_Y;

#define DIV(a, b) (((a) * (0x10004 / (b))) >> 16)

    int sXpix = sX >> 16;
    int characterX = DIV(sXpix, CHAR_TRIX_X);
    int shift = CHAR_TRIX_X - (sXpix - characterX * CHAR_TRIX_X);

    int scanline = 0;
    for (int row = startRow; scanline < _SCANLINES; row++) {

        const int height = _SCANLINES - scanline < CHAR_Y ? _SCANLINES - scanline : CHAR_Y;

        p = RAM + _BOARD + row * _1ROW + characterX;

        for (int half = 0; half < 2; half++) {

            unsigned char leftMask = !half && theCave->flags & CAVEDEF_LOCK_X ? 0b11100000 : 0b11110000;

            grabCharacters();

            unsigned char *pf0 = RAM + arenas[half] + scanline;

            for (int y = -lcount; y < height; y++) {

                int lineColour = rollDirect[roller][y];

                int px = ((unsigned int)(img[0][lineColour] | corner[0][lineColour]) << 27 >> 7) |
                         ((unsigned int)(img[1][lineColour] | corner[1][lineColour]) << 27 >> 12) |
                         ((unsigned int)(img[2][lineColour] | corner[2][lineColour]) << 27 >> 17) |
                         ((unsigned int)(img[3][lineColour] | corner[3][lineColour]) << 27 >> 22) |
                         ((unsigned int)(img[4][lineColour] | corner[4][lineColour]) << 27 >> 27);

                px >>= shift;

                *(pf0 + (_BUFFER_SIZE << 1)) = reverseBits[(unsigned char)px];
                *(pf0 + _BUFFER_SIZE) = px >> 8;
                *pf0++ = reverseBits[px >> 16] & leftMask;
            }
        }

        scanline += height + lcount;
        lcount = 0;
    }
}

bool drawBit(int x, int y, unsigned char colour) {

    colour |= colour << 3;
    colour >>= roller;

    int line = (y - ((scrollY) >> 16)) * 3;
    if (line < 0 || line >= _SCANLINES - 3)
        return false;

    int col = x - ((scrollX /** CHAR_TRIX_X*/) >> 16);
    if (col < 0 || col > SCREEN_TRIX_X - 1)
        return false;

    unsigned char *base = _BUF_GAME_PF0_LEFT + RAM + line;

    if (col >= 20) {
        col -= 20;
        base += 3 * _BUFFER_SIZE;
    }

    base += ((col + 4) >> 3) * _BUFFER_SIZE;

    int shift;
    if (col < 4)
        shift = col + 4;

    else if (col < 12)
        shift = 11 - col;

    else
        shift = (col - 12);

    int bit = 1 << shift;

    unsigned char mask0 = (colour) << shift;
    unsigned char mask1 = (colour >> 1) << shift;
    unsigned char mask2 = (colour >> 2) << shift;


    base[0] = (base[0] & ~bit) | (bit & mask0);
    base[1] = (base[1] & ~bit) | (bit & mask1);
    base[2] = (base[2] & ~bit) | (bit & mask2);

    return true;
}

// EOF
