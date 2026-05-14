#pragma once

#include "defines_dasm.h"
#include <stdbool.h>

#define RANDOM_CAR_COLOUR 0

// When 0 --> chooses colour from 'carColours[]'
// When 1 (debugginXg only), play a puzzle and hit F1 to choose car colours
// If you like the colour, break to the debugger and view randomCarColour[3]
// variable Copy the 3 values into carColours[] array ALWAYS have
// RANDOM_CAR_COLOUR == 0 for releases

extern int flashTime;
extern int openSlot;

#define FLASH(colour, time) pulseBackgroundColour(colour, time);

#if RANDOM_CAR_COLOUR
extern unsigned char bgPalette[__BOARD_DEPTH];
extern unsigned char fgPalette[2];
#endif

extern int roller;
extern int luminance;
// extern int lumTarget;
extern bool ignoreSelect;
extern int carColour;

extern const unsigned char carColours[][3];

void fadeBackgroundColour();
void setGamePalette();
// void setBackgroundPalette(const unsigned char *c);
void initColours();
void pulseBackgroundColour(unsigned char colour, int time);

void interleaveColour(int *r);
unsigned char convertColour(unsigned char colour);
// void chooseColourScheme();
void chooseBackgroundPalette();
unsigned char adjustBrightness(unsigned char colour);
// void setBrightness();
void adjustLuminance(int rateMask);

// EOF