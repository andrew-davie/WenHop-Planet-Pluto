#pragma once

#include "defines_dasm.h"

#define FLASH(colour, time) pulseBackgroundColour(colour, time);

extern int roller;

void fadeBackgroundColour();
void pulseBackgroundColour(unsigned char colour, int time);

void interleaveChronoColour(int *r);
unsigned char convertColour(unsigned char colour);

// EOF