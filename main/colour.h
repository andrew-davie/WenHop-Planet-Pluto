#pragma once

enum CC {

    // ChronoColour modes.
    // Explicit values to prevent idiots rearranging the order.

    CC_NONE = 0,
    CC_PCC = 1,
    CC_ICC = 2,
};


#define FLASH(colour, time) pulseBackgroundColour(colour, time);

extern int roller;

void fadeBackgroundColour();
void pulseBackgroundColour(unsigned char colour, int time);

void interleaveChronoColour(int *r);
unsigned char convertColour(unsigned char colour);
void setPFColours(unsigned char *colours);
void setPalette();
void loadPalette();

// EOF