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


bool isIn(char ch, const char *str) {

    if (font == FONT_COMPACT)
        while (*str)
            if (ch == *str++)
                return true;
    return false;
}

#define LTR(s) (s - ' ')
#define KERN_LEFT "*+,\"^tTf"
#define KERN_RIGHT "(cderTf"


int getLineWidth(const char *str) {

    int width = 0;
    while (*str && *str != '}' && *str != '|') {
        width += fonts[font].charWidths[LTR(*str)] + 1;

        if (font == FONT_COMPACT) {
            if (isIn(*str, KERN_LEFT))    // LHS
                width--;
            if (isIn(*str, KERN_RIGHT))    // RHS
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

    while (++current > speedDelay && ps && *ps) {

        current = 0;
        if (*ps == '.' && *(ps - 1) == '.' && *(ps - 2) == '.')
            current = -100;

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

            ADDAUDIO(ci ? SFX_KEYPRESS : SFX_SPACE);

            if (isIn(ch, KERN_LEFT))
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

            if (isIn(ch, KERN_RIGHT))
                cx--;
        }

        ps++;
    }

    return ps && *ps;
}


// EOF
