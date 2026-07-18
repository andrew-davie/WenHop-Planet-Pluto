#include <stdbool.h>

#include "drawScreen.h"

#include "defines_dasm.h"


#include "cdfjplus.h"

#include "animations.h"
#include "attribute.h"
#include "caveData.h"
#include "characterset.h"
#include "charset.h"
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

unsigned char revectorChar[256];

void initCharVector() {

    for (int i = 0; i < 256; i++)
        revectorChar[i] = i;
}

// // To create emoji comment...
// // python3 tools/reformat_arrays.py main/score.c

// const unsigned char _CHAR_INNER_CORNER_03[] = {

//     0b00011, 0b00000, 0b00000,    // 0 |◼️◼️◼️🟫🟫|
//     0b00001, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️🟫|
//     0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 9 |◼️◼️◼️◼️◼️|
// };

// const unsigned char _CHAR_INNER_CORNER_06[] = {

//     0b00000, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
//     0b00011, 0b00000, 0b00000,    // 9 |◼️◼️◼️🟫🟫|
// };

// const unsigned char _CHAR_INNER_CORNER_07[] = {

//     0b00001, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️🟫|
//     0b00001, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️🟫|
//     0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
//     0b00011, 0b00000, 0b00000,    // 9 |◼️◼️◼️🟫🟫|
// };

// const unsigned char _CHAR_INNER_CORNER_09[] = {

//     0b11000, 0b00000, 0b00000,    // 0 |🟫🟫◼️◼️◼️|
//     0b10000, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️◼️|
//     0b10000, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 9 |◼️◼️◼️◼️◼️|
// };

// const unsigned char _CHAR_INNER_CORNER_11[] = {

//     0b11001, 0b00000, 0b00000,    // 0 |🟫🟫◼️◼️🟫|
//     0b10001, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️🟫|
//     0b10000, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 9 |◼️◼️◼️◼️◼️|
// };

// const unsigned char _CHAR_INNER_CORNER_12[] = {

//     0b00000, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
//     0b10000, 0b00000, 0b00000,    // 9 |🟫◼️◼️◼️◼️|
// };

// const unsigned char _CHAR_INNER_CORNER_13[] = {

//     0b11000, 0b00000, 0b00000,    // 0 |🟫🟫◼️◼️◼️|
//     0b10000, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️◼️|
//     0b10000, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️◼️|
//     0b10000, 0b00000, 0b00000,    // 9 |🟫◼️◼️◼️◼️|
// };

// const unsigned char _CHAR_INNER_CORNER_14[] = {

//     0b00000, 0b00000, 0b00000,    // 0 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 1 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 2 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
//     0b10011, 0b00000, 0b00000,    // 9 |🟫◼️◼️🟫🟫|
// };

// const unsigned char _CHAR_INNER_CORNER_15[] = {

//     0b11011, 0b00000, 0b00000,    // 0 |🟫🟫◼️🟫🟫|
//     0b10001, 0b00000, 0b00000,    // 1 |🟫◼️◼️◼️🟫|
//     0b10001, 0b00000, 0b00000,    // 2 |🟫◼️◼️◼️🟫|
//     0b00000, 0b00000, 0b00000,    // 3 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 4 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 5 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 6 |◼️◼️◼️◼️◼️|
//     0b00000, 0b00000, 0b00000,    // 7 |◼️◼️◼️◼️◼️|
//     0b00001, 0b00000, 0b00000,    // 8 |◼️◼️◼️◼️🟫|
//     0b10011, 0b00000, 0b00000,    // 9 |🟫◼️◼️🟫🟫|
// };


const unsigned char *const roundedCorner[16] = {

    charset_auto[CHAR_MAP_0_0].data,     // 00
    charset_auto[CHAR_MAP_0_0].data,     // 01 U
    charset_auto[CHAR_MAP_0_0].data,     // 02 R
    charset_auto[CHAR_MAP_0_14].data,    // 03 RU
    charset_auto[CHAR_MAP_0_0].data,     // 04 D
    charset_auto[CHAR_MAP_0_0].data,     // 05 DU
    charset_auto[CHAR_MAP_1_14].data,    // 06 RD
    charset_auto[CHAR_MAP_2_14].data,    // 07 URD
    charset_auto[CHAR_MAP_0_0].data,     // 08 L
    charset_auto[CHAR_MAP_3_14].data,    // 09 LU
    charset_auto[CHAR_MAP_0_0].data,     // 10 LR
    charset_auto[CHAR_MAP_4_14].data,    // 11 LUR
    charset_auto[CHAR_MAP_5_14].data,    // 12 LD
    charset_auto[CHAR_MAP_6_14].data,    // 13 LDU
    charset_auto[CHAR_MAP_7_14].data,    // 14 LRD
    charset_auto[CHAR_MAP_8_14].data,    // 15 LURD
};

const unsigned char *const geoDoge[] = {

    charset_auto[CHAR_MAP_0_7].data,     // 0
    charset_auto[CHAR_MAP_1_7].data,     // 1
    charset_auto[CHAR_MAP_2_7].data,     // 2
    charset_auto[CHAR_MAP_5_7].data,     // 3
    charset_auto[CHAR_MAP_3_7].data,     // 4
    charset_auto[CHAR_MAP_6_7].data,     // 5
    charset_auto[CHAR_MAP_8_7].data,     // 6
    charset_auto[CHAR_MAP_11_7].data,    // 7
    charset_auto[CHAR_MAP_4_7].data,     // 8
    charset_auto[CHAR_MAP_7_7].data,     // 9
    charset_auto[CHAR_MAP_9_7].data,     // 10
    charset_auto[CHAR_MAP_12_7].data,    // 11
    charset_auto[CHAR_MAP_10_7].data,    // 12
    charset_auto[CHAR_MAP_13_7].data,    // 13
    charset_auto[CHAR_MAP_14_7].data,    // 14
    charset_auto[CHAR_MAP_15_7].data,    // 15
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

        else {

            if (Attribute[type] & ATT_GEODOGE) {

                udlr = ((Attribute[CharToType[GET(p[col - _1ROW])]] & ATT_GEODOGE) >> POS_GEODOGE |
                        ((Attribute[CharToType[GET(p[col + 1])]] & ATT_GEODOGE) >> (POS_GEODOGE - 1)) |
                        ((Attribute[CharToType[GET(p[col + _1ROW])]] & ATT_GEODOGE) >> (POS_GEODOGE - 2)) |
                        ((Attribute[CharToType[GET(p[col - 1])]] & ATT_GEODOGE) >> (POS_GEODOGE - 3)));


                img[col] = geoDoge[udlr];
            }

            else
                img[col] = charSet[revectorChar[p2]];
        }


        if (Attribute[type] & ATT_PAD) {

            udlr = ((Attribute[CharToType[GET(p[col - _1ROW])]] & ATT_CORNER) >> POS_CORNER |
                    ((Attribute[CharToType[GET(p[col + 1])]] & ATT_CORNER) >> (POS_CORNER - 1)) |
                    ((Attribute[CharToType[GET(p[col + _1ROW])]] & ATT_CORNER) >> (POS_CORNER - 2)) |
                    ((Attribute[CharToType[GET(p[col - 1])]] & ATT_CORNER) >> (POS_CORNER - 3)));

            corner[col] = roundedCorner[udlr];
        }

        else
            corner[col] = roundedCorner[0];
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

            unsigned char leftMask = !half && theCave->flags & CAVEDEF_MASK_LEFT_PIXEL ? 0b11100000 : 0b11110000;

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

    int line = (y - (scrollY >> 16)) * 3;
    if (line < 0 || line >= _SCANLINES - 2)
        return false;

    int col = x - (scrollX >> 16);
    if (col < 0 || col > SCREEN_TRIX_X - 1)
        return false;

    colour |= colour << 3;
    colour >>= roller;

    unsigned char *base = _BUF_GAME_PF0_LEFT + RAM + line;

    if (col >= 20) {
        col -= 20;
        base += 3 * _BUFFER_SIZE;
    }

    base += ((col + 4) >> 3) * _BUFFER_SIZE;

    static const unsigned int sh[] = {4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7};

    int shift = sh[col];
    int mask = ~(1 << shift);

    base[0] = (base[0] & mask) | ((colour & 1) << shift);
    base[1] = (base[1] & mask) | (((colour >> 1) & 1) << shift);
    base[2] = (base[2] & mask) | (((colour >> 2) & 1) << shift);

    return true;
}

// EOF
