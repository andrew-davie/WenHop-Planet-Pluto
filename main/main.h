#pragma once

#include "state.h"

#define _WENHOP_SK_ID _SK_GAME_ID


extern unsigned int frame;
extern int tvSystem;
extern unsigned int rand;
extern unsigned char colubk;

extern void ClearChannel(void *ptr);
extern void MemCopy32(void *ptr1, void *ptr2, unsigned int count);
extern void Random(unsigned int count);

void setJumpVectors(unsigned int buffer, short int startAddress, short int endAddress, int length);

unsigned int rangeRandom(short int range);

void setGameState(enum GAME_STATE state);


// EOF