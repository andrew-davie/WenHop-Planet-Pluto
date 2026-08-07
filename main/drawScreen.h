#pragma once

#include <stdbool.h>

// Neighbour-dependent geodoge cluster glyphs, indexed by a 4-bit
// up/down/left/right ATT_GEODOGE bitmask -- see grabCharacters() (the real
// on-screen selection logic) and particle.c's PT_RAIN case, which mirrors it
// for per-pixel rain collision against geodoge.
extern const unsigned char *const conglomerate[];

void drawScreen();
void drawScreenMirror(int buffer);
void initCharVector();
bool drawBit(int x, int y, unsigned char colour);

// EOF
