#pragma once

#include "state.h"

extern unsigned int frame;

unsigned int rangeRandom(short int range);
void setNextGameState(enum GAME_STATE state);

// EOF