#pragma once

#include <stdbool.h>


enum fontsize {
    FONT_STANDARD,
    FONT_COMPACT,
    FONT_LARGE,
};


void draw6Bitmap(unsigned int grpOffset, unsigned int colup0Offset, const unsigned char bitmap6[][6], int height, int y,
                 int colour);

void initString();
void drawString(int fontNumber, int colour, int delay, int buffer, int colbuf, const char *string, int y);
bool drawNextChar();

void drawAttachedChar(int ch);
void blitShape(int ch, int trixX, int y, int height, int buffer);

// EOF