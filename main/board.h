#pragma once

void setupBoardScanner();
void processBoardSquares();
void initBoard();
void explode(unsigned char *where, unsigned char explosionShape);
void surroundingConglomerate(int col, int row);


// EOF
