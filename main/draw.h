#pragma once

#include <stdbool.h>


enum fontsize {
    FONT_STANDARD,
    FONT_COMPACT,
    FONT_LARGE,
};


void draw6Bitmap(unsigned int grpOffset, unsigned int colup0Offset, const unsigned char bitmap6[][6], int height, int y,
                 int colour);

void initAsciiStringDraw(int fontNumber, int colour, int delay, int buffer, int colbuf, const char *string, int x,
                         int y);
bool drawNextChar();

// EOF