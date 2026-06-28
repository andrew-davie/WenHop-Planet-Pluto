#pragma once


#define SCALE_FAR 0x20000
#define SCALE_NEAR 0x0C000

extern int scalex;
extern int planetDir;
extern int rotationAccel;

void drawPlanet(int half);
void initPlanet(int planet);
int nextPlanet();

// EOF
