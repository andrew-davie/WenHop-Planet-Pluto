#pragma once

void draw6Bitmap(unsigned int grpOffset, unsigned int colup0Offset, const unsigned char bitmap6[][6], int height, int y,
                 int colour);

void initAsciiStringDraw(int fontNumber, int colour, int buffer, int colbuf, const char *string, int x, int y);
bool drawNextChar();

// EOF