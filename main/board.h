#pragma once

#include "defines_dasm.h"
#include <stdbool.h>

extern bool onOff[_BOARD_COLS];
extern bool lastOnOff[_BOARD_COLS];
extern bool onOffHoriz[_BOARD_ROWS];
extern bool lastOnOffHoriz[_BOARD_ROWS];


// The board scanner's current cell, threaded explicitly through the
// per-cell handler chain instead of relying on implicit globals. me/row/col
// only need to be valid for the duration of one cell's processing; the
// actual persistent-across-frames scan position lives in boardRow/boardCol
// (see main.h), which only setupBoardScanner() and processBoardSquares()
// touch directly.
typedef struct {
    unsigned char *me;
    int row;
    int col;
} BoardCursor;

void setupBoardScanner();
void processBoardSquares();
void initBoard();
void explode(unsigned char *where, unsigned char explosionShape);

// EOF
