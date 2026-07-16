#pragma once

#include "defines_dasm.h"
#include <stdbool.h>

extern bool onOff[_BOARD_COLS];
extern bool lastOnOff[_BOARD_COLS];

void setupBoardScanner();
void processBoardSquares();
void initBoard();
void explode(unsigned char *where, unsigned char explosionShape);
// void surroundingConglomerate(int col, int row);

// EOF
