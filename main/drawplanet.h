#pragma once


#define SCALE_FAR 0x20000
#define SCALE_NEAR 0x0C000
#define MIDPOINT ((SCALE_FAR + SCALE_NEAR) / 2)    // ~131180

extern int scalex;
extern int planetDir;

void drawPlanet(int half);
void initPlanet(int planet);
int nextPlanet();
void drawStars();


// EOF
