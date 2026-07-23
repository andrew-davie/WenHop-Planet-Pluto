#pragma once


#define FLASH(colour, time) pulseBackgroundColour(colour, time);

extern int roller;
extern int luminance;
extern int lumTarget;

void fadeBackgroundColour();
void pulseBackgroundColour(unsigned char colour, int time);

void interleaveChronoColour(int *r);
unsigned char convertColour(unsigned char colour);
void setPFColours(const unsigned char *palette, unsigned char *colourBuffer);
void setPalette(int buf);
void loadPalette();
void adjustLuminance(int speed);


// EOF