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
#include "scroll.h"

extern int roller;

const unsigned char *img[9];
static const unsigned char *corner[9];
const unsigned char *p;

static unsigned char revectorChar[256];

void initCharVector() {

    for (int i = 0; i < 256; i++)
        revectorChar[i] = i;
}


const unsigned char *const roundedCorner[16] = {

    CH(CHAR_MAP_0_0),     // 00
    CH(CHAR_MAP_0_0),     // 01 U
    CH(CHAR_MAP_0_0),     // 02 R
    CH(CHAR_MAP_0_14),    // 03 RU
    CH(CHAR_MAP_0_0),     // 04 D
    CH(CHAR_MAP_0_0),     // 05 DU
    CH(CHAR_MAP_1_14),    // 06 RD
    CH(CHAR_MAP_2_14),    // 07 URD
    CH(CHAR_MAP_0_0),     // 08 L
    CH(CHAR_MAP_3_14),    // 09 LU
    CH(CHAR_MAP_0_0),     // 10 LR
    CH(CHAR_MAP_4_14),    // 11 LUR
    CH(CHAR_MAP_5_14),    // 12 LD
    CH(CHAR_MAP_6_14),    // 13 LDU
    CH(CHAR_MAP_7_14),    // 14 LRD
    CH(CHAR_MAP_8_14),    // 15 LURD
};

const unsigned char *const conglomerate[] = {

    CH(CHAR_MAP_0_7),     // 0
    CH(CHAR_MAP_1_7),     // 1
    CH(CHAR_MAP_2_7),     // 2
    CH(CHAR_MAP_5_7),     // 3
    CH(CHAR_MAP_3_7),     // 4
    CH(CHAR_MAP_6_7),     // 5
    CH(CHAR_MAP_8_7),     // 6
    CH(CHAR_MAP_11_7),    // 7
    CH(CHAR_MAP_4_7),     // 8
    CH(CHAR_MAP_7_7),     // 9
    CH(CHAR_MAP_9_7),     // 10
    CH(CHAR_MAP_12_7),    // 11
    CH(CHAR_MAP_10_7),    // 12
    CH(CHAR_MAP_13_7),    // 13
    CH(CHAR_MAP_14_7),    // 14
    CH(CHAR_MAP_15_7),    // 15
};


void grabCharacters(const unsigned char *const cset[], int count) {

    enum ChName p2;
    enum ObjectType type;
    unsigned char udlr;

    for (int col = 0; col < count; col++) {

        p2 = GET(p[col]);
        type = CharToType[p2];

        if (Animate[type])
            img[col] = cset[*Animate[type]];    // cset[revectorChar[*Animate[type]]];

        else {

            if (Attribute[type] & ATT_GEODOGE) {

                udlr = ((Attribute[CharToType[GET(p[col - _BOARD_COLS])]] & ATT_GEODOGE) >> POS_GEODOGE |
                        ((Attribute[CharToType[GET(p[col + 1])]] & ATT_GEODOGE) >> (POS_GEODOGE - 1)) |
                        ((Attribute[CharToType[GET(p[col + _BOARD_COLS])]] & ATT_GEODOGE) >> (POS_GEODOGE - 2)) |
                        ((Attribute[CharToType[GET(p[col - 1])]] & ATT_GEODOGE) >> (POS_GEODOGE - 3)));


                img[col] = conglomerate[udlr];
            }

            else
                img[col] = cset[p2];    // charSet[revectorChar[p2]];
        }


        if (Attribute[type] & ATT_PAD) {

            udlr = ((Attribute[CharToType[GET(p[col - _BOARD_COLS])]] & ATT_CORNER) >> POS_CORNER |
                    ((Attribute[CharToType[GET(p[col + 1])]] & ATT_CORNER) >> (POS_CORNER - 1)) |
                    ((Attribute[CharToType[GET(p[col + _BOARD_COLS])]] & ATT_CORNER) >> (POS_CORNER - 2)) |
                    ((Attribute[CharToType[GET(p[col - 1])]] & ATT_CORNER) >> (POS_CORNER - 3)));

            corner[col] = roundedCorner[udlr];
        }

        else
            corner[col] = roundedCorner[0];
    }

    p += count - 1;
}


const unsigned char rollDirect[3][CHAR_Y] = {


    {2, 0, 1, 5, 3, 4, 8, 6, 7, 11, 9, 10, 14, 12, 13, 17, 15, 16, 20, 18, 19, 23, 21, 22, 26, 24, 25, 29, 27, 28},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
    {1, 2, 0, 4, 5, 3, 7, 8, 6, 10, 11, 9, 13, 14, 12, 16, 17, 15, 19, 20, 18, 22, 23, 21, 25, 26, 24, 28, 29, 27},
};


const unsigned short arenas[] = {

    _BUF_GAME_PF0_LEFT,
    _BUF_GAME_PF0_RIGHT,
};

/*
 * drawScreen() -- single-pass version.
 *
 * Originally this fetched 5 characters (25 bits) per half and ran the loop
 * twice (arenas[0], arenas[1]) to build the 40px asymmetric strip. Same
 * technique as drawScreenMirror(): one wider fetch instead of two glued
 * halves. 40 output px + 5 bits of CHAR_TRIX_X shift slack = 45 bits needed,
 * which is exactly 9 characters x 5 bits -- zero slack wasted (tighter than
 * drawScreenMirror()'s 8-char/3-bit-waste, since every plane here is either
 * a full byte or a byte-aligned nibble extraction, not a truncated one).
 *
 * The 40-bit meaningful window (post-shift) is laid out exactly like the
 * old per-half computation, just concatenated instead of computed twice:
 *
 *   bits[39:36] PF0_LEFT nibble   bits[19:16] PF0_RIGHT nibble
 *   bits[35:28] PF1_LEFT byte     bits[15:8]  PF1_RIGHT byte
 *   bits[27:20] PF2_LEFT byte     bits[7:0]   PF2_RIGHT byte
 *
 * Each nibble is pulled via the same "extract the byte starting at the
 * nibble's low bit, reverseBits, mask 0xF0" trick the original code used --
 * the top 4 bits of that extracted byte are discarded by the mask, so it
 * doesn't matter that they can be zero-as-invalid past bit 39 at low shift
 * values (same reasoning that made the original safe with only 1 bit of
 * headroom, just verified fresh for this wider window rather than assumed).
 */
void drawScreen() {

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

    int startRow = (-lcount * (0x10000 / CHAR_Y)) >> 16;
    lcount += startRow * CHAR_Y;

#define DIV(a, b) (((a) * (0x10004 / (b))) >> 16)

    int sXpix = sX >> 16;
    int characterX = DIV(sXpix, CHAR_TRIX_X);
    int shift = CHAR_TRIX_X - (sXpix - characterX * CHAR_TRIX_X);

    unsigned char leftMaskLeft = theCave->flags & CAVEDEF_MASK_LEFT_PIXEL ? 0b11100000 : 0b11110000;

    int scanline = 0;
    unsigned char *pfL = RAM + arenas[0] + scanline;
    unsigned char *pfR = RAM + arenas[1] + scanline;


    for (int row = startRow; scanline < _SCANLINES; row++) {

        const int height = _SCANLINES - scanline < CHAR_Y ? _SCANLINES - scanline : CHAR_Y;

        p = RAM + _BOARD + row * _BOARD_COLS + characterX;

        grabCharacters(charSet, 9);


        for (int y = -lcount; y < height; y++) {

            int lineColour = rollDirect[roller][y];

            // 9 characters x 5 bits, column 0 (leftmost) at the top of the
            // field -- same convention as drawScreen()/drawScreenMirror()
            // have always used.
            unsigned long long packed =
                ((unsigned long long)(img[0][lineColour] | corner[0][lineColour]) & 0x1F) << 40 |
                ((unsigned long long)(img[1][lineColour] | corner[1][lineColour]) & 0x1F) << 35 |
                ((unsigned long long)(img[2][lineColour] | corner[2][lineColour]) & 0x1F) << 30 |
                ((unsigned long long)(img[3][lineColour] | corner[3][lineColour]) & 0x1F) << 25 |
                ((unsigned long long)(img[4][lineColour] | corner[4][lineColour]) & 0x1F) << 20 |
                ((unsigned long long)(img[5][lineColour] | corner[5][lineColour]) & 0x1F) << 15 |
                ((unsigned long long)(img[6][lineColour] | corner[6][lineColour]) & 0x1F) << 10 |
                ((unsigned long long)(img[7][lineColour] | corner[7][lineColour]) & 0x1F) << 5 |
                ((unsigned long long)(img[8][lineColour] | corner[8][lineColour]) & 0x1F);

            // shift is loop-invariant (fixed for this whole call) but its
            // VALUE isn't known until runtime, so a plain "packed >>= shift"
            // forces gcc to emit the generic 64-bit variable-shift sequence
            // (separate runtime-computed lo/hi combine) on every one of these
            // ~66 iterations. shift only ever takes 5 values (1..CHAR_TRIX_X,
            // see its computation above), so switching on it once per
            // iteration turns each shift back into a compile-time-constant
            // immediate shift -- one barrel-shifter instruction per 32-bit
            // half instead of a general-purpose routine.
            //
            // Measured on hardware: hoisting this switch out to wrap the
            // whole row loop (5 full copies of the loop body, one dispatch
            // per drawScreen() call instead of one per iteration) was tried
            // and was SLOWER on real hardware (70K vs 67K) despite being
            // "obviously" fewer dispatches -- the 5x code-size growth costs
            // more in instruction-fetch than the per-iteration branch saves.
            // Keep the dispatch here, inside the loop, at this size.
            // switch (shift) {
            // case 1: packed >>= 1; break;
            // case 2: packed >>= 2; break;
            // case 3: packed >>= 3; break;
            // case 4: packed >>= 4; break;
            // default: packed >>= 5; break;
            // }


            packed >>= shift;

            *(pfL + (_BUFFER_SIZE << 1)) = reverseBits[(unsigned char)(packed >> 20)];    // PF2_LEFT
            *(pfL + _BUFFER_SIZE) = (unsigned char)(packed >> 28);                        // PF1_LEFT
            *pfL++ = reverseBits[(unsigned char)(packed >> 36)] & leftMaskLeft;           // PF0_LEFT

            *(pfR + (_BUFFER_SIZE << 1)) = reverseBits[(unsigned char)packed];    // PF2_RIGHT
            *(pfR + _BUFFER_SIZE) = (unsigned char)(packed >> 8);                 // PF1_RIGHT
            *pfR++ = reverseBits[(unsigned char)(packed >> 16)];                  // PF0_RIGHT
        }

        scanline += height + lcount;
        lcount = 0;
    }
}


/*
 * "Mirrored-register" playfield variant of drawScreen().
 *
 * NOT a left/right content mirror - the left and right 16 pixels are
 * independent, contiguous board content (a proper 32px-wide scrolling
 * bitmap), same as drawScreen()'s 40px asymmetric strip. "Mirrored" here
 * refers only to the TIA register layout, which uses just PF1/PF2 (no
 * PF0) laid out left to right as:
 *
 *     PF1_LEFT | PF2_LEFT | PF2_RIGHT | PF1_RIGHT
 *
 * This is the same 4-plane arrangement already used by _BUF_SKULL_PF and
 * _BUF_MENU_PF (see defines_dasm.h and gameState_Skull.c / gameState_Menu.c
 * for the matching data stream setup), and the direct/reverseBits[] pattern
 * per plane below is lifted from gameState_Skull.c:doDrawBitmap(), which
 * slices one contiguous pixel row into this same 4-plane layout.
 *
 * Unlike drawScreen() (which fetches 5 characters per half and glues two
 * 20px halves together), this fetches ONE 8-character window per scanline
 * and slices it straight into all 4 planes. Gluing two 5-character/20px
 * halves together doesn't divide evenly into 2x16px - dropping each half's
 * leftover 4 bits to fit PF1+PF2 only left a 4-column gap at the seam
 * (visible as the right side looking shifted/wrong). Using one wider fetch
 * avoids that split entirely: 32 output bits need up to 5 bits of
 * sub-pixel shift slack (CHAR_TRIX_X), i.e. >=37 raw bits, so 8 characters
 * (40 bits) are fetched into a 64-bit accumulator (5-char/32-bit packing
 * doesn't have room for that, hence the wider type).
 *
 * `buffer` is the RAM offset of the 4-plane target; each plane is
 * _BUFFER_SIZE bytes, laid out as:
 *
 *     buffer + 0 * _BUFFER_SIZE  ->  PF1_LEFT
 *     buffer + 1 * _BUFFER_SIZE  ->  PF2_LEFT
 *     buffer + 2 * _BUFFER_SIZE  ->  PF2_RIGHT
 *     buffer + 3 * _BUFFER_SIZE  ->  PF1_RIGHT
 *
 * Caller is responsible for wiring those 4 offsets up to the
 * corresponding _DS_..._PF1_LEFT/PF2_LEFT/PF2_RIGHT/PF1_RIGHT data
 * streams (initDataStreams_Skull()/initDataStreams_Menu() are examples).
 */
void drawScreenMirror(int buffer) {

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

    int startRow = (-lcount * (0x10000 / CHAR_Y)) >> 16;
    lcount += startRow * CHAR_Y;

#define DIV(a, b) (((a) * (0x10004 / (b))) >> 16)

    int sXpix = sX >> 16;
    int characterX = DIV(sXpix, CHAR_TRIX_X);
    int shift = CHAR_TRIX_X - (sXpix - characterX * CHAR_TRIX_X);

    int scanline = 0;
    for (int row = startRow; scanline < _SCANLINES; row++) {

        const int height = _SCANLINES - scanline < CHAR_Y ? _SCANLINES - scanline : CHAR_Y;

        p = RAM + _BOARD + row * _BOARD_COLS + characterX;

        grabCharacters(charSet, 8);

        unsigned char *pf = RAM + buffer + scanline;

        for (int y = -lcount; y < height; y++) {

            int lineColour = rollDirect[roller][y];

            // 8 characters x 5 bits, column 0 (leftmost) at the top of the
            // field, matching drawScreen()'s own column-ordering convention.
            unsigned long long packed =
                ((unsigned long long)(img[0][lineColour] | corner[0][lineColour]) & 0x1F) << 35 |
                ((unsigned long long)(img[1][lineColour] | corner[1][lineColour]) & 0x1F) << 30 |
                ((unsigned long long)(img[2][lineColour] | corner[2][lineColour]) & 0x1F) << 25 |
                ((unsigned long long)(img[3][lineColour] | corner[3][lineColour]) & 0x1F) << 20 |
                ((unsigned long long)(img[4][lineColour] | corner[4][lineColour]) & 0x1F) << 15 |
                ((unsigned long long)(img[5][lineColour] | corner[5][lineColour]) & 0x1F) << 10 |
                ((unsigned long long)(img[6][lineColour] | corner[6][lineColour]) & 0x1F) << 5 |
                ((unsigned long long)(img[7][lineColour] | corner[7][lineColour]) & 0x1F);

            unsigned int px32 = (unsigned int)(packed >> shift);

            pf[0] = px32 >> 24;                                             // PF1_LEFT  (direct)
            pf[_BUFFER_SIZE] = reverseBits[(unsigned char)(px32 >> 16)];    // PF2_LEFT  (reversed)
            pf[2 * _BUFFER_SIZE] = px32 >> 8;                               // PF2_RIGHT (direct)
            pf[3 * _BUFFER_SIZE] = reverseBits[(unsigned char)px32];        // PF1_RIGHT (reversed)
            pf++;
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

    static const unsigned char sh[] = {4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7};

    int shift = sh[col];
    int mask = ~(1 << shift);

    base[0] = (base[0] & mask) | ((colour & 1) << shift);
    base[1] = (base[1] & mask) | (((colour >> 1) & 1) << shift);
    base[2] = (base[2] & mask) | (((colour >> 2) & 1) << shift);

    return true;
}

// EOF
