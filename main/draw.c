#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"    // <- contains references from defines_dasm.h
#include "colour.h"
#include "draw.h"
#include "random.h"
#include "score.h"
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

    if (y + height >= _SCANLINES)
        height = _SCANLINES - y - 1;

    // else if (y < 0) {
    //     bm += -y * 6;
    //     height += y;
    //     p += y;
    //     q += y;
    // }

    for (int line = 0; line < height; line++) {
        if (y++ >= 0) {
            q[line] = colour;
            for (int c = 0; c < 6; c++)
                p[c * _BUFFER_SIZE + line] = *bm++;
        }

        else {
            bm += 6;
        }
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

const struct FONT {
    int lineHeight;
    const unsigned char **asciiTable;
    const unsigned char *charWidths;
} fonts[] = {
    {ALPHANUMERIC_FONT_HEIGHT, alphanumeric_asciiTable, alphanumeric_charWidths},
    {FONTCOMPACT_FONT_HEIGHT, fontcompact_asciiTable, fontcompact_charWidths},
    {FONTLARGE_FONT_HEIGHT, fontlarge_asciiTable, fontlarge_charWidths},
};


void initAsciiStringDraw(int fontNumber, int c, int delay, int buffer, int colbuf, const char *string, int x, int y) {

    // x: 0..47 pixel pos in GRP array
    // y: 1.._SCANLINES-1 line  (0 doesn't work because of colour)
    // string zero-terminated

    font = fontNumber;
    colour = c;

    ps = string;
    buf = RAM + buffer + y;
    colrx = RAM + colbuf + y - 1;

    speedDelay = delay;
    current = 0;

    cx = x;
    cy = y;
}


int wcol = 8;


int getWordWidth(const char *str) {

    int width = 0;
    while (*str && *str != '}' && *str != '|') {
        width += fonts[font].charWidths[*str - ' '] + 1;
        str++;
    }
    return width ? width - 1 : 0;
}


bool drawNextChar() {

    // false = complete
    // we have a lovely ASCII character set to work with, so this should be easy!

    if (++current > speedDelay && ps && *ps) {

        current = 0;

        int ch = *ps - ' ';

        if (ch == '|' - ' ') {
            cx = 0;
            cy += fonts[font].lineHeight + 2;

            wcol = (rangeRandom(16) << 4) | 8;
        }

        else if (ch == '}' - ' ') {
            cx = 0;
            cy += fonts[font].lineHeight + 2;
            wcol = (rangeRandom(16) << 4) | 8;
        }

        else if (ch == '>' - ' ') {
            cx = 47 - getWordWidth(ps + 1);
        }

        else if (ch == '=' - ' ') {
            cx = 24 - (getWordWidth(ps + 1) >> 1);
        }


        else if (fonts[font].charWidths[ch]) {

            ADDAUDIO(ch ? SFX_KEYPRESS : SFX_SPACE);

            if (ch == ',' - ' ')
                cx--;

            int column = cx >> 3;
            int bit = 7 - (cx & 7);

            unsigned char *p = RAM + _BUF_GLOBE_GRP;    // buf;
            // unsigned char *q = colrx - 1;

            const unsigned char *charShape = fonts[font].asciiTable[ch];

            if (charShape)
                for (int i = 0; i < fonts[font].lineHeight; i++) {

                    int vertPos = cy + i;
                    if (vertPos >= 0) {    // && vertPos < _SCANLINES) {

                        int shape = charShape[i] << (8 - fonts[font].charWidths[ch]);

                        int bs = bit + 1;
                        int band = column;

                        unsigned char *qq = RAM + _BUF_GLOBE_COLUP0 - 1;
                        qq[vertPos] = colour;

                        while (shape && (band < 6)) {

                            p[vertPos + band * _BUFFER_SIZE] |= (shape << bs) >> 8;

                            shape = (shape << bs) & 0xFF;
                            bs = 8;
                            band++;
                        }
                    }
                }

            cx += fonts[font].charWidths[ch] + 1;
        }

        ps++;
    }

    return ps && *ps;
}


// EOF
