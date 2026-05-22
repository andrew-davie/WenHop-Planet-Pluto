#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"    // <- contains references from defines_dasm.h
#include "colour.h"
#include "draw.h"


void draw6Bitmap(unsigned int grpOffset, unsigned int colup0Offset,    //
                 const unsigned char bitmap6[][6],                     //
                 int height, int y, int colour) {

    // Draw a 6-sprite wide bitmap

    unsigned char *p = RAM + grpOffset + y;           //_BUF_COPYRIGHT_GRP0A + y;
    unsigned char *q = RAM + colup0Offset + y - 1;    //_BUF_COPYRIGHT_COLUP0 + y - 1;
    const unsigned char *bm = &bitmap6[0][0];

    colour = convertColour(colour);

    if (y + height >= _SCANLINES)
        height = _SCANLINES - y;

    for (int line = 0; line < height; line++) {
        q[line] = colour;
        for (int c = 0; c < 6; c++) {
            p[c * _SCANLINES + line] = *bm++;
        }
    }
}
