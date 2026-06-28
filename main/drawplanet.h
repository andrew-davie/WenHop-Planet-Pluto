#pragma once

#define MAX_PLANET (sizeof(planets)/sizeof(planets[0]))

extern int planetDir;

void drawPlanet(int half);
void initPlanet(int planet);
int nextPlanet();

// EOF
