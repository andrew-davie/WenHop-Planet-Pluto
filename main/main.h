#pragma once

#include "state.h"

extern unsigned int frame;
extern int tvSystem;
extern unsigned int rand;
extern unsigned char colubk;

extern void ClearChannel(void *ptr);
extern void MemCopy32(void *ptr1, void *ptr2, unsigned int count);
extern void Random(unsigned int count);


unsigned int rangeRandom(short int range);
void setNextGameState(enum GAME_STATE state);


// EOF