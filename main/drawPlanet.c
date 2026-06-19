#include "drawplanet.h"

#include "defines_dasm.h"

#include "cdfjplus.h"
#include "colour.h"

#include "animations.h"
#include "attribute.h"
#include "reverseBits.h"

#include "main.h"
// #include "defines_from_dasm_for_c.h"
#include "characterset.h"
#include "scroll.h"
// #include "defines.h"

// duplicate from defines_cdfj.h
// Raw queue pointers
extern void *DDR;
#ifndef RAM
#define RAM ((unsigned char *)DDR)
#endif


const int pix85[] = {
    0b000000000000000000000000000000,    // 0
    0b000000000000100001000000010000,    // 3
    0b000000000001001001000100000100,    // 6
    0b000000000001010010010010000100,    // 9
    0b000000000001010010010010000100,    // 12
    0b000000000001010101001001000010,    // 15
    0b000000000010101010010100100010,    // 18
    0b000000000010101010010100100010,    // 21
    0b000000000010101011001010010010,    // 24
    0b000000000010101011001010010010,    // 27
    0b000000000010101011001010010010,    // 30
    0b000000000010110101101010010001,    // 33
    0b000000000010110101101010010001,    // 36
    0b000000000010110101101010010001,    // 39
    0b000000000010111011010101010001,    // 42
    0b000000000010111011010101010001,    // 45
    0b000000000010111011010101010001,    // 48
    0b000000000010111011010101010001,    // 51
    0b000000000010111011010101010001,    // 54
    0b000000000011011101101011001001,    // 57
    0b000000000011011101101011001001,    // 60
    0b000000000011011101101011001001,    // 63
    0b000000000011011101101011001001,    // 66
    0b000000000011011101101011001001,    // 69
    0b000000000011011101101011001001,    // 72
    0b000000000011011101101011001001,    // 75
    0b000000000011011101101011001001,    // 78
    0b000000000011011101101011001001,    // 81
    0b000000000011011101101011001001,    // 84
    0b000000000011011101101011001001,    // 87
    0b000000000011011101101011001001,    // 90
    0b000000000011011101101011001001,    // 93
    0b000000000011011101101011001001,    // 96
    0b000000000011011101101011001001,    // 99
    0b000000000011011101101011001001,    // 102
    0b000000000011011101101011001001,    // 105
    0b000000000011011101101011001001,    // 108
    0b000000000011011101101011001001,    // 111
    0b000000000011011101101011001001,    // 114
    0b000000000010111011010101010001,    // 117
    0b000000000010111011010101010001,    // 120
    0b000000000010111011010101010001,    // 123
    0b000000000010111011010101010001,    // 126
    0b000000000010111011010101010001,    // 129
    0b000000000010110101101010010001,    // 132
    0b000000000010110101101010010001,    // 135
    0b000000000010110101101010010001,    // 138
    0b000000000010101011001010010010,    // 141
    0b000000000010101011001010010010,    // 144
    0b000000000010101010010100100010,    // 147
    0b000000000010101010010100100010,    // 150
    0b000000000001010101001001000010,    // 153
    0b000000000001010101001001000010,    // 156
    0b000000000001010010010010000100,    // 159
    0b000000000001001001000100000100,    // 162
    0b000000000001000100001000001000,    // 165
    0b000000000000100001000000010000,    // 168
};

const short int line85[] = {
    0,      // 0
    47,     // 3
    73,     // 6
    85,     // 9
    105,    // 12
    114,    // 15
    131,    // 18
    140,    // 21
    149,    // 24
    163,    // 27
    172,    // 30
    178,    // 33
    192,    // 36
    198,    // 39
    204,    // 42
    213,    // 45
    227,    // 48
    233,    // 51
    236,    // 54
    242,    // 57
    256,    // 60
    262,    // 63
    268,    // 66
    274,    // 69
    288,    // 72
    291,    // 75
    297,    // 78
    303,    // 81
    309,    // 84
    323,    // 87
    326,    // 90
    332,    // 93
    338,    // 96
    352,    // 99
    358,    // 102
    364,    // 105
    367,    // 108
    373,    // 111
    387,    // 114
    393,    // 117
    399,    // 120
    405,    // 123
    419,    // 126
    425,    // 129
    431,    // 132
    448,    // 135
    454,    // 138
    460,    // 141
    469,    // 144
    483,    // 147
    492,    // 150
    501,    // 153
    518,    // 156
    527,    // 159
    547,    // 162
    562,    // 165
    588,    // 168
    -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


int pscrollSpeed = 0x1240;


unsigned int stepSize = 0x100;
unsigned int stepSize2 = 0x100;


unsigned int mult = (int)(1.25 * 0x100);
unsigned int div = (int)(0x100 / 1.25);

int scalex = 0x60;    // 3D0;
int dir = 0;

void initPlanet() {

    scalex = 0xb0;
    dir = 0;
}


void drawPlanet(int half) {

    scalex += dir;
    if (dir < 0 && scalex < 0x40) {
        dir = -dir;
    }

    if (dir > 0 && scalex > 0x200)
        dir = -dir;

    // scalex--;
    // half: 0 for left, 5 for right

    scrollY = 0;


    if (!(swcha & (0x40 << 4)))
        pscrollSpeed += 100;
    if (!(swcha & (0x80 << 4)))
        pscrollSpeed -= 100;

    if (!(swcha & 0x20)) {
        if (scalex < 0x300)
            scalex += 5;
    }

    if (!(swcha & 0x10)) {
        if (scalex > 0x15)
            scalex -= 5;
    }

    stepSize = (0x100 * ((scalex * mult) >> 8)) >> 8;
    stepSize2 = (0x100 * ((scalex * div) >> 8)) >> 8;

    // if (sizerDelay > 1 && (SWCHA & 0x20)) {
    //     sizerDelay = 0;
    //     if (stepSize > 0x1000)
    //         stepSize -= 10;
    // }

    scrollX += pscrollSpeed;
    if (scrollX >= (30 << 16)) {
        scrollX -= (30 << 16);
    }
    if (scrollX < 0) {
        scrollX += 30 << 16;    //+= (20 << 16);
    }


    // if (sizerDelay++ > 2) {
    //     sizer+= sizerDirection;
    //     if (sizer > 10 || sizer < 0) {
    //         sizer -= sizerDirection;
    //         sizerDirection = -sizerDirection;
    //     }
    //     sizerDelay = 0;
    // }


    int shift = 4 - ((scrollX >> 14) & 3);
    int frac = scrollX >> 16;
    unsigned char *xchar;
    const unsigned char *image[6];
    int lastBottom = _SCANLINES;


    // int base = half ? _BUF_GLOBE_PF : (_BUF_GLOBE_PF + 3 * _BUFFER_SIZE);

    unsigned char *pf0 = RAM + _BUF_GLOBE_PF + (half ? 0 : (3 * _BUFFER_SIZE));
    unsigned char *pf1 = pf0 + _BUFFER_SIZE;
    unsigned char *pf2 = pf1 + _BUFFER_SIZE;

    int equivalentLine = 0;
    int equiv = 0;
    int absScanline = 0;
    unsigned char piece;
    unsigned int type;

    int startScanline = 0;


    for (absScanline = startScanline; absScanline < _SCANLINES && line85[equiv] >= 0;) {

        xchar = RAM + _BOARD + (half + frac) + _BOARD_COLS * (line85[equiv] >> 5);

#if 0
#define GRAB(i)                                                                                                        \
    piece = *xchar++;                                                                                                  \
    type = CharToType[piece];                                                                                          \
    if (Animate[type])                                                                                                 \
        piece = (*Animate[type])[AnimIdx[type].index];                                                                 \
    image[i] = *charSet[piece] + (line85[equiv] & 31);
#endif

#define GRAB(i)                                                                                                        \
    piece = *xchar++;                                                                                                  \
    image[i] = charSet[piece] + (line85[equiv] & 31);

        GRAB(0);
        GRAB(1);
        GRAB(2);
        GRAB(3);
        GRAB(4);
        GRAB(5);


        int mask = pix85[equiv];
        int roll = roller;

        for (int icc = 0; icc < 3; icc++) {


            absScanline++;

            // int p = (*image[0]++ << 20 | *image[1]++ << 16 | *image[2]++ << 12 | *image[3]++ << 8 | *image[4]++ << 4
            // |
            //          *image[5]++) >>
            //         shift;


            int p = (*(image[0] + roll) << 20       //
                     | *(image[1] + roll) << 16     //
                     | *(image[2] + roll) << 12     //
                     | *(image[3] + roll) << 8      //
                     | *(image[4] + roll) << 4 |    //
                     *(image[5] + roll)) >>
                    shift;

            int p2 = 0;
            int p3 = 0;
            int bitOffset = 128;    // 1/2 pix


            if (!half) {

#define BLK(i)                                                                                                         \
    if (mask & (1 << i))                                                                                               \
        p2 = (p2 << 1) | ((p >> (19 - i)) & 1);

                BLK(0)
                BLK(1)
                BLK(2)
                BLK(3)
                BLK(4)
                BLK(5)
                BLK(6)
                BLK(7)
                BLK(8)
                BLK(9)

                BLK(10)
                BLK(11)
                BLK(12)
                BLK(13)
                BLK(14)
                BLK(15)
                BLK(16)
                BLK(17)
                BLK(18)
                BLK(19)

#define PUT(i)                                                                                                         \
    if (p2 & (1 << (bitOffset >> 8)))                                                                                  \
        p3 |= 1 << i;                                                                                                  \
    bitOffset += stepSize;

                PUT(0)
                PUT(1)
                PUT(2)
                PUT(3)
                PUT(4)
                PUT(5)
                PUT(6)
                PUT(7)
                PUT(8)
                PUT(9)

                PUT(10)
                PUT(11)
                PUT(12)
                PUT(13)
                PUT(14)
                PUT(15)
                PUT(16)
                PUT(17)
                PUT(18)
                // PUT(19)

                if (p2 & (1 << (bitOffset >> 8)))
                    p3 |= 0b1000000000000000000;

            }

            else {

#define BLK2(i)                                                                                                        \
    if (mask & (1 << i))                                                                                               \
        p2 = (p2 >> 1) | (((p >> i) & 1) << 19);

                BLK2(0)
                BLK2(1)
                BLK2(2)
                BLK2(3)
                BLK2(4)
                BLK2(5)
                BLK2(6)
                BLK2(7)
                BLK2(8)
                BLK2(9)

                BLK2(10)
                BLK2(11)
                BLK2(12)
                BLK2(13)
                BLK2(14)
                BLK2(15)
                BLK2(16)
                BLK2(17)
                BLK2(18)
                BLK2(19)


#define PUT2(i)                                                                                                        \
    if (p2 & (1 << (19 - (bitOffset >> 8))))                                                                           \
        p3 |= 1 << i;                                                                                                  \
    bitOffset += stepSize;


                PUT2(19)
                PUT2(18)
                PUT2(17)
                PUT2(16)
                PUT2(15)
                PUT2(14)
                PUT2(13)
                PUT2(12)
                PUT2(11)
                PUT2(10)

                PUT2(9)
                PUT2(8)
                PUT2(7)
                PUT2(6)
                PUT2(5)
                PUT2(4)
                PUT2(3)
                PUT2(2)
                PUT2(1)
                // PUT2(0)

                if (p2 & (1 << (19 - (bitOffset >> 8))))
                    p3 |= 1;
            }

            if (half) {

                p3 <<= 4;

                *pf2++ = p3 >> 16;
                *pf1++ = reverseBits[(p3 >> 8) & 0xFF];
                *pf0++ = p3;
            }

            else {


                *pf0++ = reverseBits[p3 >> 16];
                *pf1++ = p3 >> 8;
                *pf2++ = reverseBits[p3 & 0xFF];
            }


            if (++roll > 2)
                roll = 0;
        }

        for (int i = 0; i < 6; i++)
            image[i] += 3;

        equivalentLine += stepSize;
        equiv = equivalentLine >> 8;
    }

    int thisBottom = absScanline;

    while (absScanline < lastBottom && absScanline < _SCANLINES) {
        *pf0++ = 0;
        *pf1++ = 0;
        *pf2++ = 0;
        absScanline++;
    }

    lastBottom = thisBottom;
}
