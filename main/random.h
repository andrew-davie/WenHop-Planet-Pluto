#pragma once

extern unsigned int prng_a, prng_b;

void initRandom();
unsigned int getRandom32();
unsigned int rangeRandom(int range);

// EOF