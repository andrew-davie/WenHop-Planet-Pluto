#pragma once

#include <stdbool.h>

void drawScreen();
void drawIconScreen(int startRow, int endRow);
void initCharVector();
bool drawBit(int x, int y, unsigned char colour);

// EOF
