#pragma once

#define LIFE_SIZE  10
#define LIFE_CELLS (LIFE_SIZE * LIFE_SIZE)

// 0 = dead, 1-7 = alive with that colour
extern unsigned char wcol[LIFE_CELLS];

void initLife();

// Advances up to 'rows' rows of the next generation per call, so the cost of
// a full generation can be spread across multiple frames. Call it repeatedly
// (e.g. once per frame with rows=1) to keep the simulation running.
void life(int rows);

// EOF
