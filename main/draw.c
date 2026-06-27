#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"    // <- contains references from defines_dasm.h
#include "colour.h"
#include "draw.h"
#include "sound.h"

#include "../gfx/alphanumeric.h"
#include "../gfx/fontcompact.h"
#include "../gfx/fontlarge.h"

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
static int colour;
static int speedDelay;
static int current;

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


void initAsciiStringDraw(int fontNumber, int c, int delay, int buffer, int colbuf, const char *string, int x, int y) {

    // x: 0..47 pixel pos in GRP array
    // y: 1.._SCANLINES-1 line  (0 doesn't work because of colour)
    // string zero-terminated

    font = fontNumber;
    colour = c;

    ps = string;
    buf = RAM + buffer;          // + y;
    colrx = RAM + colbuf - 1;    // + y - 1;

    speedDelay = delay;
    current = 0;

    justify = JUSTIFY_NONE;

    cx = x;
    cy = y;
}


const unsigned char justifyChar[] = {

    0b00, /* ' ' ASCII 32 */
    0b00, /* '!' ASCII 33 */
    0b10, /* '0x22' ASCII 34 */
    0b00, /* '#' ASCII 35 */
    0b00, /* '$' ASCII 36 */
    0b00, /* '%' ASCII 37 */
    0b00, /* '&' ASCII 38 */
    0b00, /* ''' ASCII 39 */
    0b01, /* '(' ASCII 40 */
    0b00, /* ')' ASCII 41 */
    0b10, /* '*' ASCII 42 */
    0b10, /* '+' ASCII 43 */
    0b10, /* ',' ASCII 44 */
    0b00, /* '-' ASCII 45 */
    0b00, /* '.' ASCII 46 */
    0b00, /* '/' ASCII 47 */
    0b00, /* '0' ASCII 48 */
    0b00, /* '1' ASCII 49 */
    0b00, /* '2' ASCII 50 */
    0b00, /* '3' ASCII 51 */
    0b00, /* '4' ASCII 52 */
    0b00, /* '5' ASCII 53 */
    0b00, /* '6' ASCII 54 */
    0b00, /* '7' ASCII 55 */
    0b00, /* '8' ASCII 56 */
    0b00, /* '9' ASCII 57 */
    0b00, /* ':' ASCII 58 */
    0b00, /* ';' ASCII 59 */
    0b00, /* '<' ASCII 60 */
    0b00, /* '=' ASCII 61 */
    0b00, /* '>' ASCII 62 */
    0b00, /* '?' ASCII 63 */
    0b00, /* '@' ASCII 64 */
    0b00, /* 'A' ASCII 65 */
    0b00, /* 'B' ASCII 66 */
    0b00, /* 'C' ASCII 67 */
    0b00, /* 'D' ASCII 68 */
    0b00, /* 'E' ASCII 69 */
    0b00, /* 'F' ASCII 70 */
    0b00, /* 'G' ASCII 71 */
    0b00, /* 'H' ASCII 72 */
    0b00, /* 'I' ASCII 73 */
    0b00, /* 'J' ASCII 74 */
    0b00, /* 'K' ASCII 75 */
    0b00, /* 'L' ASCII 76 */
    0b00, /* 'M' ASCII 77 */
    0b00, /* 'N' ASCII 78 */
    0b00, /* 'O' ASCII 79 */
    0b00, /* 'P' ASCII 80 */
    0b00, /* 'Q' ASCII 81 */
    0b00, /* 'R' ASCII 82 */
    0b00, /* 'S' ASCII 83 */
    0b11, /* 'T' ASCII 84 */
    0b00, /* 'U' ASCII 85 */
    0b00, /* 'V' ASCII 86 */
    0b00, /* 'W' ASCII 87 */
    0b00, /* 'X' ASCII 88 */
    0b00, /* 'Y' ASCII 89 */
    0b00, /* 'Z' ASCII 90 */
    0b00, /* '[' ASCII 91 */
    0b00, /* '0x5C' ASCII 92 */
    0b00, /* ']' ASCII 93 */
    0b10, /* '^' ASCII 94 */
    0b00, /* '_' ASCII 95 */
    0b00, /* '`' ASCII 96 */
    0b00, /* 'a' ASCII 97 */
    0b00, /* 'b' ASCII 98 */
    0b01, /* 'c' ASCII 99 */
    0b01, /* 'd' ASCII 100 */
    0b01, /* 'e' ASCII 101 */
    0b11, /* 'f' ASCII 102 */
    0b00, /* 'g' ASCII 103 */
    0b00, /* 'h' ASCII 104 */
    0b00, /* 'i' ASCII 105 */
    0b00, /* 'j' ASCII 106 */
    0b00, /* 'k' ASCII 107 */
    0b00, /* 'l' ASCII 108 */
    0b00, /* 'm' ASCII 109 */
    0b00, /* 'n' ASCII 110 */
    0b00, /* 'o' ASCII 111 */
    0b00, /* 'p' ASCII 112 */
    0b00, /* 'q' ASCII 113 */
    0b01, /* 'r' ASCII 114 */
    0b00, /* 's' ASCII 115 */
    0b10, /* 't' ASCII 116 */
    0b00, /* 'u' ASCII 117 */
    0b00, /* 'v' ASCII 118 */
    0b00, /* 'w' ASCII 119 */
    0b00, /* 'x' ASCII 120 */
    0b00, /* 'y' ASCII 121 */
    0b00, /* 'z' ASCII 122 */
    0b00, /* '{' ASCII 123 */
    0b00, /* '|' ASCII 124 */
    0b00, /* '}' ASCII 125 */
    0b00, /* '~' ASCII 126 */
};


#define LTR(s) (s - ' ')

int getLineWidth(const char *str) {

    int width = 0;
    while (*str && *str != '}' && *str != '|') {
        width += fonts[font].charWidths[LTR(*str)] + 1;

        if (font == FONT_COMPACT) {
            if (justifyChar[LTR(*str)] & 0b10)    // LHS
                width--;
            if (justifyChar[LTR(*str)] & 0b01)    // RHS
                width--;
        }

        str++;
    }
    return width ? width - 1 : 0;
}

void setJustifyX(const char *str) {

    if (justify == JUSTIFY_CENTER)
        cx = 24 - (getLineWidth(str) >> 1);

    else if (justify == JUSTIFY_RIGHT)
        cx = 48 - getLineWidth(str);

    else    // JUSTIFY_NONE and JUSTIFY_LEFT
        cx = 0;
}


bool drawNextChar() {

    // false = complete

    if (ps && *ps && ++current > speedDelay) {

        current = 0;
        // if (*ps == '.' && *(ps - 1) == '.' && *(ps - 2) == '.')
        //     current = -100;

        int ch = *ps;
        int ci = LTR(ch);

        if (ch == '|') {
            setJustifyX(ps + 1);
            cy += fonts[font].lineHeight;
        }

        else if (ch == '>') {
            justify = JUSTIFY_RIGHT;
            setJustifyX(ps + 1);
        }

        else if (ch == '<') {
            justify = JUSTIFY_LEFT;
            setJustifyX(ps + 1);
        }

        else if (ch == '=') {
            justify = JUSTIFY_CENTER;
            setJustifyX(ps + 1);
        }

        else if (fonts[font].charWidths[ci]) {

            if (ch != '+' && ch != ' ')
                ADDAUDIO(ch == '*' ? SFX_DOGE : SFX_KEYPRESS);

            if (font == FONT_COMPACT && justifyChar[ci] & 0b10)
                cx--;

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

            cx += fonts[font].charWidths[ci] + 1;

            if (font == FONT_COMPACT && justifyChar[ci] & 0b01)
                cx--;
        }

        ps++;
    }

    return ps && *ps;
}


// EOF
