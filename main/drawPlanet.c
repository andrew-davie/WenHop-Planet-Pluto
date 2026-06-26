
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "drawplanet.h"
#include "main.h"
#include "reverseBits.h"
#include "scroll.h"

#include "spinningGlobe/blood2.h"
#include "spinningGlobe/bloodworld.h"
#include "spinningGlobe/earth.h"
#include "spinningGlobe/green1.h"
#include "spinningGlobe/lava.h"
#include "spinningGlobe/moon.h"
#include "spinningGlobe/neptune.h"
#include "spinningGlobe/pangea.h"
#include "spinningGlobe/titan.h"

#define DSP -200

extern void initStars();

struct GLOBE {
    int retrograde;
    const unsigned char *map;
    const unsigned char *const *charSet;
    const unsigned char *palette;
};


const unsigned char neptune_ntsc_palette_override[3] = {
    0x92, /* palette[1] = (28,56,144) */
    0x94, /* palette[2] = (56,84,168) */
    0x96, /* palette[4] = (80,116,188) */
};

const unsigned char jupiter_ntsc_palette_override[3] = {
    0x4C, /* palette[1] = (108,108,108) */
    0x18, /* palette[2] = (144,144,144) */
    0x2A, /* palette[4] = (176,176,176) */
};

const unsigned char earth_ntsc_palette_override[3] = {
    0x94, /* palette[1] = (28,76,120) */
    0xD6, /* palette[2] = (104,112,52) */
    0x0A, /* palette[4] = (212,188,248) */
};

const unsigned char moon_ntsc_palette_override[3] = {
    0x14, /* palette[1] = (108,108,108) */
    0xE4, /* palette[2] = (144,144,144) */
    0xE2, /* palette[4] = (176,176,176) */
};


const unsigned char bloodworld_ntsc_palette_override[3] = {
    0x40, /* palette[1] = (68,40,0) */
    0x42, /* palette[2] = (100,72,24) */
    0xC6, /* palette[4] = (144,144,144) */
};

const unsigned char pangea_ntsc_palette_override[3] = {
    0x84, /* palette[1] = (0,44,92) */
    0xD4, /* palette[2] = (64,64,64) */
    0x32, /* palette[4] = (176,176,176) */
};

const unsigned char blood2_ntsc_palette_override[3] = {
    0xF6, /* palette[1] = (68,40,0) */
    0x94, /* palette[2] = (132,104,48) */
    0x64, /* palette[4] = (176,176,176) */
};

const unsigned char green1_ntsc_palette_override[3] = {
    0xB2, /* palette[1] = (0,60,44) */
    0xD6, /* palette[2] = (32,92,32) */
    0xC4, /* palette[4] = (108,108,108) */
};

const unsigned char ridged_ntsc_palette_override[3] = {
    0x12, /* palette[1] = (0,44,92) */
    0xDA, /* palette[2] = (64,64,64) */
    0xCA, /* palette[4] = (144,144,144) */
};

const unsigned char lava_ntsc_palette_override[3] = {
    0xE2, /* palette[1] = (44,48,0) */
    0x30, /* palette[2] = (132,24,0) */
    0x34, /* palette[4] = (172,80,48) */
};

// TODO: run spinningGlobe/make.sh to re-gen the planet data
//       run python3 spinningGlobe/planet-gen.py to create new planet images

// To add a planet

//  a) put planet texture map in spinningGlobes/textures director (e.g., newplanet.jpg)
//  b) run cset.py from spinningGlobes directory
//     suggested params:
//     --no-dither
//     --trixel-height 10 --trixel-width 5
//      --adaptive-palette --black-threshold 20
//      --brightness 1.0
//     --max-chars 128 textures/titan.png 20 4
//  c) optionally add planet to spinningGlobes/make.sh
//
//  Note: a reconstructed image is placed in spinningGlobes (newplanet_recon.png)

//  1) enter filename in makefile SRCS list (e.g., spinningGlobes/newplanet.c)
//  2) enter map, charset, palette entries in planets[] table below
//     note: the palette can be replaced (copy from newplanet.c to above, and append _override)
//  3) "#include spinningGlobe/newplanet.h" at the top of displayPlanet.c

const struct GLOBE planets[10] = {
    {1, earth_map, earth_charset, earth_ntsc_palette_override},
    {-1, lava_map, lava_charset, lava_ntsc_palette_override},
    {1, neptune_map, neptune_charset, neptune_ntsc_palette_override},
    {-1, green1_map, green1_charset, green1_ntsc_palette_override},
    {-1, pangea_map, pangea_charset, pangea_ntsc_palette_override},
    {1, bloodworld_map, bloodworld_charset, bloodworld_ntsc_palette_override},
    {1, blood2_map, blood2_charset, blood2_ntsc_palette_override},
    {1, titan_map, titan_charset, titan_ntsc_palette},
    {-1, moon_map, moon_charset, moon_ntsc_palette_override},
    {-1, moon_map, moon_charset, moon_ntsc_palette_override},    // TODO: dup
};


const int pix85[] = {
    0b000000000000000000000000000000,    // 00  0
    0b000000000000100001000000010000,    // 01  3
    0b000000000001001001000100000100,    // 02  6
    0b000000000001010010010010000100,    // 03  9
    0b000000000001010010010010000100,    // 04  12
    0b000000000001010101001001000010,    // 05  15
    0b000000000010101010010100100010,    // 06  18
    0b000000000010101010010100100010,    // 07  21
    0b000000000010101011001010010010,    // 08  24
    0b000000000010101011001010010010,    // 09  27
    0b000000000010101011001010010010,    // 10  30
    0b000000000010110101101010010001,    // 11  33
    0b000000000010110101101010010001,    // 12  36
    0b000000000010110101101010010001,    // 13  39
    0b000000000010111011010101010001,    // 14  42
    0b000000000010111011010101010001,    // 15  45
    0b000000000010111011010101010001,    // 16  48
    0b000000000010111011010101010001,    // 17  51
    0b000000000010111011010101010001,    // 18  54
    0b000000000011011101101011001001,    // 19  57
    0b000000000011011101101011001001,    // 20  60
    0b000000000011011101101011001001,    // 21  63
    0b000000000011011101101011001001,    // 22  66
    0b000000000011011101101011001001,    // 23  69
    0b000000000011011101101011001001,    // 24  72
    0b000000000011011101101011001001,    // 25  75
    0b000000000011011101101011001001,    // 26  78
    0b000000000011011101101011001001,    // 27  81
    0b000000000011011101101011001001,    // 28  84
    0b000000000011011101101011001001,    // 29  87
    0b000000000011011101101011001001,    // 30  90
    0b000000000011011101101011001001,    // 31  93
    0b000000000011011101101011001001,    // 32  96
    0b000000000011011101101011001001,    // 33  99
    0b000000000011011101101011001001,    // 34  102
    0b000000000011011101101011001001,    // 35  105
    0b000000000011011101101011001001,    // 36  108
    0b000000000011011101101011001001,    // 37  111
    0b000000000011011101101011001001,    // 38  114
    0b000000000010111011010101010001,    // 39  117
    0b000000000010111011010101010001,    // 40  120
    0b000000000010111011010101010001,    // 41  123
    0b000000000010111011010101010001,    // 42  126
    0b000000000010111011010101010001,    // 43  129
    0b000000000010110101101010010001,    // 44  132
    0b000000000010110101101010010001,    // 45  135
    0b000000000010110101101010010001,    // 46  138
    0b000000000010101011001010010010,    // 47  141
    0b000000000010101011001010010010,    // 48  144
    0b000000000010101010010100100010,    // 49  147
    0b000000000010101010010100100010,    // 50  150
    0b000000000001010101001001000010,    // 51  153
    0b000000000001010101001001000010,    // 52  156
    0b000000000001010010010010000100,    // 53  159
    0b000000000001001001000100000100,    // 54  162
    0b000000000001000100001000001000,    // 55  165
    0b000000000000100001000000010000,    // 56  168
};


// Table maps line index to line number in texture
// Generated by spinningGlobe/scale.py
// texture height = chars * trix/char * 3 - fudge (which makes the bottom align)
// sub-position is always 3 (CC)


// Number of table entries (57 for original): 57
// Texture height in pixels:                  120
// Character cell height in scanlines:        30
// Sub-position step [3]:
// Array name [line85]:
// Mapping - (s)pherical or (l)inear [s]:

const short int line85[] = {
    0,      // 0
    9,      // 1
    15,     // 2
    18,     // 3
    21,     // 4
    24,     // 5
    24,     // 6
    27,     // 7
    27,     // 8
    32,     // 9
    35,     // 10
    38,     // 11
    38,     // 12
    41,     // 13
    41,     // 14
    44,     // 15
    44,     // 16
    47,     // 17
    47,     // 18
    50,     // 19
    50,     // 20
    53,     // 21
    53,     // 22
    56,     // 23
    56,     // 24
    56,     // 25
    59,     // 26
    59,     // 27
    64,     // 28
    64,     // 29
    67,     // 30
    67,     // 31
    70,     // 32
    70,     // 33
    73,     // 34
    73,     // 35
    76,     // 36
    76,     // 37
    76,     // 38
    79,     // 39
    79,     // 40
    82,     // 41
    85,     // 42
    85,     // 43
    88,     // 44
    88,     // 45
    91,     // 46
    91,     // 47
    96,     // 48
    99,     // 49
    99,     // 50
    102,    // 51
    105,    // 52
    108,    // 53
    111,    // 54
    114,    // 55
    128,    // 56
    -1,  -1, -1, -1, -1, -1, -1,
};

#define PSS 0

int pscrollSpeed = PSS;    // 0x8800;    // C80;    // 0xA00;


unsigned int stepSize = 0x100;
unsigned int stepSize2 = 0x100;


unsigned int mult = (int)(1.25 * 0x100);
unsigned int div = (int)(0x100 / 1.25);

int scalex;
int planetDir, dirTarget;
int body;


#define SCALE_FAR (0x140 << 8)                     // 262144
#define SCALE_NEAR (0xC8 << 8)                     // 216
#define MIDPOINT ((SCALE_FAR + SCALE_NEAR) / 2)    // ~131180
#define DIVISOR 12000

#define MINSCALE SCALE_NEAR
#define MAXSCALE SCALE_FAR


void initPlanet(int planet) {

    scalex = MAXSCALE;
    planetDir = 0;
    pscrollSpeed = PSS;

    body = planet;

    initStars();

    extern const unsigned char *thePalette;
    thePalette = planets[body].palette;
}


int nextPlanet() {

    if (++body >= (int)(sizeof(planets) / sizeof(planets[0])))
        body = 0;

    initPlanet(body);
    return body;
}


void drawPlanet(int half) {

    const unsigned char *map = planets[body].map;
    const unsigned char *const *charset = planets[body].charSet;
    // const unsigned char *palette = planets[body].palette;

    planetDir += ((MIDPOINT - scalex) * (0x10000 / DIVISOR)) >> 16;
    scalex += planetDir;


    if (pscrollSpeed < 0x1100)
        pscrollSpeed += 30;


    scrollY = 0;


    // if (!(swcha & (0x40 << 4)))
    //     pscrollSpeed += 100;
    // if (!(swcha & (0x80 << 4)))
    //     pscrollSpeed -= 100;

    // if (!(swcha & 0x20)) {
    //     if (scalex < 0x300)
    //         scalex += 5;
    // }

    // if (!(swcha & 0x10)) {
    //     if (scalex > 0x15)
    //         scalex -= 5;
    // }

    stepSize = (0x100 * ((scalex * mult) >> 8)) >> 16;
    stepSize2 = (0x100 * ((scalex * div) >> 8)) >> 16;


#define TEX 20

    scrollX += planets[body].retrograde * pscrollSpeed;
    if (scrollX >= (TEX << 16)) {
        scrollX -= (TEX << 16);
    }
    if (scrollX < 0) {
        scrollX += TEX << 16;    //+= (20 << 16);
    }


    int shift = 4 - ((scrollX >> 14) & 3);
    int frac = scrollX >> 16;
    const unsigned char *xchar;
    const unsigned char *image[6];
    // int lastBottom = _SCANLINES;


    // int yz = (_SCANLINES >> 1);


    unsigned char *pf0 = RAM + _BUF_GLOBE_PF + (half ? 0 : (3 * _BUFFER_SIZE));
    unsigned char *pf1 = pf0 + _BUFFER_SIZE;
    unsigned char *pf2 = pf1 + _BUFFER_SIZE;

    int equivalentLine = 0;
    int equiv = 0;
    int absScanline = 0;
    unsigned char piece;
    // unsigned int type;

    int startScanline = 0;

    for (absScanline = startScanline; absScanline < _SCANLINES - 3 && line85[equiv] >= 0;) {

        if (equiv > 56)
            equiv = 56;    // tmp

        xchar = map + (half + frac) + map[0] * (line85[equiv] >> 5) + 2;

#define GRAB(i)                                                                                                        \
    piece = *xchar++;                                                                                                  \
    image[i] = charset[piece] + (line85[equiv] & 31);

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
                //                *pf0++ = p3;
            }

            else {


                //              *pf0++ = reverseBits[p3 >> 16];
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
}

// EOF
