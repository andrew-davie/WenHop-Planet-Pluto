#pragma once

#define LIFE_SIZE  10
#define LIFE_CELLS (LIFE_SIZE * LIFE_SIZE)

// 0 = dead, 1-7 = alive with that colour
extern unsigned char wcol[LIFE_CELLS];

void initLife();
void life();

// EOF
