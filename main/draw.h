#pragma once

#include <stdbool.h>


enum fontsize {
    FONT_STANDARD,
    FONT_COMPACT,
    FONT_LARGE,
};


void draw6Bitmap(unsigned int grpOffset, unsigned int colup0Offset, const unsigned char bitmap6[][6], int height, int y,
                 int colour);

void drawString(int fontNumber, int colour, int delay, int buffer, int colbuf, const char *string, int y);
bool drawNextChar();

void drawBitmapChar(int ch, int charX, int charY);
void drawAttachedChar(int ch);

// EOF