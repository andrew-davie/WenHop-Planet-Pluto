#pragma once

#include <stdbool.h>

void drawScreen(int half);
void drawIconScreen(int startRow, int endRow);
void initCharVector();
bool drawBit(int x, int y, unsigned char colour);

extern unsigned char revectorChar[128];

// EOF
