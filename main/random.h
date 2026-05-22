#pragma once

// extern unsigned int rndX;
extern unsigned int prng_a, prng_b;
extern unsigned int Room_random_a, Room_random_b;
extern unsigned int cave_random_a, cave_random_b;

void initRandom();
unsigned int getRandom32();
unsigned int rangeRandom(int range);
unsigned int getCaveRandom32();

// EOF