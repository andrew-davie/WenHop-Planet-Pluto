#include <stdbool.h>

#include "defines_dasm.h"

#include "animations.h"
#include "attribute.h"
#include "bitPatterns.h"
#include "cdfjplus.h"    // <- contains references from defines_dasm.h
#include "characterset.h"
#include "colour.h"
#include "draw.h"
#include "drawScreen.h"
#include "mellon.h"
#include "playerAnimation.h"
#include "reverseBits.h"
#include "scroll.h"
#include "sound.h"

#include "../gfx/alphanumeric.h"
#include "../gfx/fontcompact.h"
#include "../gfx/fontlarge.h"


int dramaticPause;


void draw6Bitmap(unsigned int grpOffset, unsigned int colup0Offset,    //
                 const unsigned char bitmap6[][6],                     //
                 int height, int y, int colour) {

    // Draw a 6-sprite wide bitmap

    unsigned char *p = RAM + grpOffset + y;
    unsigned char *q = RAM + colup0Offset + y - 1;
    const unsigned char *bm = &bitmap6[0][0];

    colour = convertColour(colour);

    if (y < 0) {
        bm -= y * 6;
        height += y;
        p -= y;
        q -= y;
    }

    if (y + height >= _SCANLINES)
        height = _SCANLINES - y;


    for (int line = 0; line < height; line++) {
        // if (y++ >= 0) {
        q[line] = colour;
        for (int c = 0; c < 6; c++)
            p[c * _BUFFER_SIZE + line] = *bm++;
        // }

        // else {
        //     bm += 6;
        // }
    }
}


static const char *ps = 0;
static unsigned char *buf;
static unsigned char *colrx;
static int cx;
static int cy;
static int font;
static int stringColour;
static int speedDelay;
static int current;
static int underline;
static int escape;

enum justify {
    JUSTIFY_NONE,
    JUSTIFY_LEFT,
    JUSTIFY_RIGHT,
    JUSTIFY_CENTER,
};


static enum justify justify;

const struct FONT {
    enum fontsize size;
    int lineHeight;
    const unsigned char **asciiTable;
    const unsigned char *charWidths;
} fonts[] = {
    {FONT_STANDARD, ALPHANUMERIC_FONT_HEIGHT, alphanumeric_asciiTable, alphanumeric_charWidths},
    {FONT_COMPACT, FONTCOMPACT_FONT_HEIGHT, fontcompact_asciiTable, fontcompact_charWidths},
    {FONT_LARGE, FONTLARGE_FONT_HEIGHT, fontlarge_asciiTable, fontlarge_charWidths},
};


void drawString(int fontNumber, int c, int delay, int buffer, int colbuf, const char *string, int y) {

    // x: 0..47 pixel pos in GRP array
    // y: 1.._SCANLINES-1 line  (0 doesn't work because of colour)
    // string zero-terminated

    font = fontNumber;
    stringColour = c;
    underline = false;
    escape = false;

    dramaticPause = 0;

    ps = string;
    buf = RAM + buffer;          // + y;
    colrx = RAM + colbuf - 1;    // + y - 1;

    speedDelay = delay;
    current = 0;

    justify = JUSTIFY_NONE;

    cx = 0;
    cy = y;
}

// clang-format off

#define J_ON        0b10000000  /* visible */
#define J_PARAGRAPH 0b00001000  /* new paragraph if followed by EOL */
#define J_UNDER     0b00000100  /* underline */
#define J_LEFT      0b00000010  /* left-justify */
#define J_RIGHT     0b00000001  /* right-justify */

const unsigned char justifyChar['~' - ' ' + 1] = {

//   🟨 = visible
//  ⎧       🟨 = new paragraph if followed by | 
//  ⎮      ⎧  🟨 = underline
//  ⎮      ⎮ ⎧ 🟨 = left-justify
//  ⎮      ⎮ ⎮⎧  🟨 = right-justify
//  ⎮      ⎮ ⎮⎮ ⎮
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   ' '         32
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '!'         33
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '"'         34
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '#'         35
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '$'         36       replacement '.' without a delay (for numbers0
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '%'         37
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '&'         38
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   '''         39 
    🟨🟦🟦🟦🟦🟦🟦🟨 ,    //   '('         40
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   ')'         41
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '*'         42
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '+'         43
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   ','         44       + short pause
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '-'         45
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '.'         46
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '/'         47
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '0'         48
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '1'         49
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '2'         50
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '3'         51
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '4'         52
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '5'         53
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '6'         54
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '7'         55
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '8'         56
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '9'         57
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ':'         58
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ';'         59
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '<'         60       left justify string
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '='         61       center justify string
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '>'         62       right justify string
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '?'         63
    🟨🟦🟦🟦🟦🟨🟨🟦 ,    //   '@'         64
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'A'         65
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'B'         66
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'C'         67
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'D'         68
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'E'         69
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'F'         70
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'G'         71
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'H'         72
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'I'         73
    🟨🟦🟦🟦🟦🟨🟨🟦 ,    //   'J'         74
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'K'         75
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'L'         76
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'M'         77
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'N'         78
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'O'         79
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'P'         80
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Q'         81
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'R'         82
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'S'         83
    🟨🟦🟦🟦🟦🟨🟨🟨 ,    //   'T'         84
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'U'         85
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'V'         86
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'W'         87
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'X'         88
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Y'         89
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Z'         90
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '['         91
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   'backslash  92
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ']'         93
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '^'         94
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '_'         95      toggle underline
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '`'         96
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'a'         97
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'b'         98
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'c'         99
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'd'         100
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'e'         101
    🟨🟦🟦🟦🟦🟨🟨🟨 ,    //   'f'         102
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'g'         103
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'h'         104
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'i'         105
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'j'         106
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'k'         107
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'l'         108
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'm'         109
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'n'         110
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'o'         111
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'p'         112
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'q'         113
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'r'         114
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   's'         115
    🟨🟦🟦🟦🟦🟨🟨🟦 ,    //   't'         116
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'u'         117
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'v'         118
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'w'         119
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'x'         120
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'y'         121
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'z'         122
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '{'         123
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '|'         124      new line
    🟦🟦🟦🟦🟨🟦🟦🟦 ,    //   '}'         125      new paragraph
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '~'         126      pause
};

// clang-format on


#define LTR(s) (s - ' ')
#define EOL '|'
#define EOP '}'

enum {
    CTL_EOL = '|',
    CTL_PAR = '}',
    CTL_LEFT = '<',
    CTL_RIGHT = '>',
    CTL_CENTER = '=',
    CTL_UNDERLINE = '_',
    CTL_PAUSE = '~',
};


int getLineWidth(const char *str) {

    bool esc = false;

    int width = 0;
    while (*str && *str != EOL && *str != EOP) {

        int ch = *str;
        int ci = LTR(ch);

        str++;

        if (ch == '\\') {
            esc = true;
            continue;
        }

        if (esc || (justifyChar[ci] & J_ON)) {

            if (ch != '#' && ch != '\"')
                width += fonts[font].charWidths[ci] + 1;

            if (justifyChar[ci] & J_LEFT)
                width--;
            if (justifyChar[ci] & J_RIGHT)
                width--;

            esc = false;
        }
    }

    return width ? width - 1 : 0;
}


void setJustifyX(const char *str) {

    switch (justify) {

    case JUSTIFY_CENTER:
        cx = (48 - getLineWidth(str)) >> 1;
        break;

    case JUSTIFY_RIGHT:
        cx = 48 - getLineWidth(str);
        break;

    default:
        cx = 0;
        break;
    }
}


void doLetter(int ci, int cx, int cy, char colour) {

    int column = cx >> 3;
    int bit = 7 - (cx & 7);

    const unsigned char *charShape = fonts[font].asciiTable[ci];
    if (charShape) {

        for (int i = 0; i < fonts[font].lineHeight; i++) {

            int vertPos = cy + i;
            if (vertPos >= 0 && vertPos < _SCANLINES) {

                int shape = charShape[i] << (8 - fonts[font].charWidths[ci]);

                int bs = bit + 1;
                int band = column;

                if (shape)
                    colrx[vertPos] = colour;

                while (shape && (band < 6)) {

                    buf[vertPos + band * _BUFFER_SIZE] |= (shape << bs) >> 8;

                    shape = (shape << bs) & 0xFF;
                    bs = 8;
                    band++;
                }
            }
        }
    }
}


bool drawNextChar() {

    if (dramaticPause)
        dramaticPause--;

    while (!dramaticPause && ps && *ps && ++current > speedDelay) {

        int ch = *ps;
        int ci = LTR(ch);

        ps++;


        if (!escape) {

            switch (ch) {

            case '\\':
                escape = true;
                current = 0;
                continue;

            case CTL_PAR:

                cy += fonts[font].lineHeight >> 1;
                dramaticPause = 20;
                __attribute__((fallthrough));

            case CTL_EOL: {

                setJustifyX(ps);
                cy += fonts[font].lineHeight;
                continue;
            }

            case CTL_RIGHT:
                justify = JUSTIFY_RIGHT;
                setJustifyX(ps);
                continue;

            case CTL_LEFT:
                justify = JUSTIFY_LEFT;
                setJustifyX(ps);
                continue;

            case CTL_CENTER:
                justify = JUSTIFY_CENTER;
                setJustifyX(ps);
                continue;

            case CTL_UNDERLINE:
                underline = !underline;
                continue;

            case CTL_PAUSE:
                dramaticPause = 50;
                continue;
            }
        }

        if ((escape || (justifyChar[ci] & J_ON)) && fonts[font].charWidths[ci]) {

            current = 0;

            if (ch == '.' && *ps != '#' && *ps != '.')    // pause sentence except at end
                dramaticPause = 50;

            else if (ch == ',')
                dramaticPause = 10;

            if (ch != ' ' && ch != '+')                           // review empty star, space
                ADDAUDIO(ch == '*' ? SFX_DOGE : SFX_KEYPRESS);    // review-star = "*"

            if (ch == '\"')
                cx -= fonts[font].charWidths[LTR('\"')];

            if (justifyChar[ci] & J_LEFT)
                cx--;

            doLetter(ci, cx, cy, convertColour(stringColour));
            if ((justifyChar[ci] & J_UNDER) && underline) {

                int uwide = fonts[font].charWidths[ci] - fonts[font].charWidths[LTR('_')] + 1;
                for (int i = 0; i <= uwide; i++)
                    doLetter(LTR('_'), cx + i, cy + 1, convertColour(0x44));
            }

            cx += fonts[font].charWidths[ci] + 1;

            if (justifyChar[ci] & J_RIGHT)
                cx--;

            escape = false;
        }
    }

    return ps && *ps;
}


// const unsigned char _CHAR_WYRM_2[CHAR_Y] = {

//     0b01111, 0b00000, 0b01111,  // 00 |◼️🟪🟪🟪🟪|
//     0b01111, 0b00000, 0b01111,  // 01 |◼️🟪🟪🟪🟪|
//     0b01111, 0b00000, 0b01111,  // 02 |◼️🟪🟪🟪🟪|
//     0b01111, 0b00100, 0b01111,  // 03 |◼️🟪⬜️🟪🟪|
//     0b00001, 0b01110, 0b01111,  // 04 |◼️🟥🟥🟥🟪|
//     0b01011, 0b00100, 0b01111,  // 05 |◼️🟪🟥🟪🟪|
//     0b01111, 0b00100, 0b01111,  // 06 |◼️🟪⬜️🟪🟪|
//     0b01111, 0b00000, 0b01111,  // 07 |◼️🟪🟪🟪🟪|
//     0b01111, 0b00000, 0b01111,  // 08 |◼️🟪🟪🟪🟪|
//     0b01111, 0b00000, 0b01111,  // 09 |◼️🟪🟪🟪🟪|
// };

/*


void doDrawBitmap(const unsigned char *shape, int x, int y) {

    unsigned char *pf_FL = RAM + _BUF_SKULL_PF + y;
    unsigned char *pf_CL = pf_FL + _SCANLINES;
    unsigned char *pf_CR = pf_CL + _SCANLINES;
    unsigned char *pf_FR = pf_CR + _SCANLINES;

    int size = shape[0];
    const unsigned char *bf = shape + 1;

    int baseRoll = roller;

    union g {
        int bGraphic;
        unsigned char g[4];
    } gfx;

    union masker {
        int mask;
        unsigned char mask2[4];
    } mask;

    for (int i = 0; i < size; i += 3) {

        mask.mask = ((bf[0] | bf[1] | bf[2]) << x) ^ 0xFFFFFFFF;

        for (int line = 0; line < 3; line++) {

            gfx.bGraphic = bf[baseRoll] << x;


            *pf_FL = (*pf_FL & mask.mask2[3]) | gfx.g[3];                              // far left
            *pf_CL = (*pf_CL & reverseBits[mask.mask2[2]]) | reverseBits[gfx.g[2]];    // center left
            *pf_CR = (*pf_CR & mask.mask2[1]) | gfx.g[1];                              // center right
            *pf_FR = (*pf_FR & reverseBits[mask.mask2[0]]) | reverseBits[gfx.g[0]];    // far right

            if (++baseRoll > 2)
                baseRoll = 0;

            pf_FL++;
            pf_CL++;
            pf_FR++;
            pf_CR++;
        }

        bf += 3;
    }
}
*/


// void drawCharacterPart(int ch, int x, int y, unsigned char *buffer) {

//     // draw to a 20-wide buffer (left or right side of screen)

//     if (x >= 20 || x <= -CHAR_X)
//         return;

//     const unsigned char *shape = charSet[ch];

//     for (int i = 0; i < CHAR_TRIX_Y; i++) {


//         int lineShape = *shape << (19 - CHAR_TRIX_X - x);

//         *buffer &= mask;
//         *buffer |= ();


//     }


// }


// void drawCharacter(int ch, int x, int y) {

//     if (ch >= CH_MAX)
//         return;

//     unsigned char *buffer;

//     buffer = (unsigned char *)(RAM +_BUF_GAME_PF0_LEFT);
//     drawCharacterPart(ch, x, y, buffer);

//     buffer = (unsigned char *)(RAM +_BUF_GAME_PF0_RIGHT);
//     drawCharacterPart(ch, x - 20, y, buffer);

// }


void blitShape(int ch, int shift, int y, int offset) {

    //            X, (also SHIFT! when we go "<<20>>shift")
    //        |  |0   |      1 |        |  |2         3
    //        |  |0123|45178901|23456789|  |01234517890123456789
    //
    // starting shape is        XXXXXXXX
    //
    // ending shapes....                                               X=
    // -------+  +----+--------+--------+  +--------------------+
    // XXXXXXX|  |X   |        |        |  |                    |*     -7 (shift <-- 19)
    //  XXXXXX|  |XX  |        |        |  |                    |      -6 (shift <-- 18)
    //   XXXXX|  |XXX |        |        |  |                    |      -5 (shift <-- 17)
    //    XXXX|  |XXXX|        |        |  |                    |      -4 (shift <-- 16)
    //     XXX|  |XXXX|X       |        |  |                    |      -3 (shift <-- 15)
    //      XX|  |XXXX|XX      |        |  |                    |      -2 (shift <-- 14)
    //       X|  |XXXX|XXX     |        |  |                    |      -1 (shift <-- 13)
    //        |  |XXXX|XXXX    |        |  |                    |-----  0 (shift <-- 12)
    //        |  | XXX|XXXXX   |        |  |                    |       1 (shift <-- 11)
    //        |  |  XX|XXXXXX  |        |  |                    |       2 (shift <-- 10)
    //        |  |   X|XXXXXXX |        |  |                    |       3 (shift <--  9)
    //        |  |    |XXXXXXXX|        |  |                    |       4 (shift <--  8)
    //        |  |    | XXXXXXX|X       |  |                    |       5 (shift <--  7)
    //        |  |    |  XXXXXX|XX      |  |                    |       6 (shift <--  6)
    //        |  |    |   XXXXX|XXX     |  |                    |       7 (shift <--  5)
    //        |  |    |    XXXX|XXXX    |  |                    |       8 (shift <--  4)
    //        |  |    |     XXX|XXXXX   |  |                    |       9 (shift <--  3)
    //        |  |    |      XX|XXXXXX  |  |                    |      10 (shift <--  2)
    //        |  |    |       X|XXXXXXX |  |                    |      11 (shift <--  1)
    //        |  |    |        |XXXXXXXX|  |                    |      12 (shift <--  0)
    //        |  |    |        | XXXXXXX|  |X                   |*     13 (shift      1 -->)
    //        |  |    |        |  XXXXXX|  |XX                  |      14 (shift      2 -->)
    //        |  |    |        |   XXXXX|  |XXX                 |      15 (shift      3 -->)
    //        |  |    |        |    XXXX|  |XXXX                |      16 (shift      4 -->)
    //        |  |    |        |     XXX|  |XXXXX               |      17 (shift      5 -->)
    //        |  |    |        |      XX|  |XXXXXX              |      18 (shift      6 -->)
    //        |  |    |        |       X|  |XXXXXXX             |------19 (shift      7 -->)
    //        |  |    |        |        |  |XXXXXXXX            |      20 (shift      8 -->)
    //        (not used)
    // -------+  +----+--------+--------+  +--------------------+

    int height = CHAR_Y;

    unsigned char *p1 = RAM + offset + y;
    unsigned char *p2 = p1 + _BUFFER_SIZE;
    unsigned char *p3 = p2 + _BUFFER_SIZE;

    int modifier = y + 1;

    //    if (y >= 0)
    while (modifier > 2)
        modifier -= 3;

    //  else
    while (modifier < 0)
        modifier += 3;

    // int r = 2;
    int bitsh = 19 - shift;

    const char nextRoller[] = {1, 2, 0, 1, 2, 0, 1, 2, 0};

    for (int trix = 0; trix < height && y < _SCANLINES; trix += 3) {

        if (y >= 0) {

            const unsigned char *fp = charSet[ch];

            unsigned int mask = fp[trix] | fp[trix + 1] | fp[trix + 2];
            mask <<= bitsh;
            mask = ~mask;

            unsigned char m1 = mask >> (16 + 7);
            m1 = reverseBits[m1];
            unsigned char m2 = mask >> (8 + 7);
            unsigned char m3 = mask >> 7;
            m3 = reverseBits[m3];

#define INNER(r)                                                                                                       \
    {                                                                                                                  \
        unsigned int shiftCh = fp[trix + nextRoller[roller + r + modifier]];                                           \
        shiftCh <<= bitsh;                                                                                             \
                                                                                                                       \
        *(p1 + r) = (*(p1 + r) & m1) | reverseBits[(unsigned char)(shiftCh >> (16 + 7))];                              \
        *(p2 + r) = (*(p2 + r) & m2) | shiftCh >> (8 + 7);                                                             \
        *(p3 + r) = (*(p3 + r) & m3) | reverseBits[(shiftCh >> 7) & 0xFF];                                             \
    }

            INNER(0)
            INNER(1)
            INNER(2)
        }

        p1 += 3;
        p2 += 3;
        p3 += 3;

        y += 3;
    }
}

void drawAttachedChar(int ch) {

    ch = GET(ch);
    int type = CharToType[ch];

    // if (Animate[type])
    //     ch = revectorChar[*Animate[type]];
    // else
    //     ch = revectorChar[ch];

    if (Animate[type])
        ch = *Animate[type];
    // else
    //     ch = revectorChar[ch];


    int ay = 0;
    int autoY = autoMoveY;
    while (autoY > 0) {
        autoY -= 3;
        ay++;
    }

    int offsetX = 0;
    int offsetY = attachment == CH_DOGE_00 ? -12 : -24;

    if (attachmentOffset) {

        offsetX = attachmentOffset->x;
        offsetY = attachmentOffset->y;


        // if (!offsetX && !offsetY)
        //     attachmentOffset = 0;

        // else

        if ((attachmentOffset + 1)->x || (attachmentOffset + 1)->y)
            attachmentOffset++;

        else
            attachmentOffset = 0;
        ;
    }


    int y = playerY * CHAR_Y - (scrollY >> 16) * 3 + autoMoveY /*- 20 + 1*/ - (shakeY >> 16) + offsetY;


    int trixX = ((playerX - 1) * CHAR_TRIX_X) + -(scrollX >> 16) + (faceDirection * (frameAdjustX + (autoMoveX >> 2))) +
                2 - (shakeX >> 16) - offsetX;

    // y = trixY;

    if (y + CHAR_Y >= 0 && y < _SCANLINES && trixX > -8 && trixX < 40) {
        if (trixX > -8 && trixX < 20)
            blitShape(ch, trixX, y, _BUF_GAME_PF0_LEFT);

        if (trixX > 12 && trixX < 40)
            blitShape(ch, trixX - 20, y, _BUF_GAME_PF0_RIGHT);
    }
}


// EOF
