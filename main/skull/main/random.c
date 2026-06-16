#include "random.h"

unsigned int prng_b;
unsigned int prng_a;

void initRandom() {

  prng_a = 0x40001FB4; //*(unsigned int *)(0x40001FB4);
  prng_b = prng_a ^ 0xBABEACED;
}

unsigned int getRandom32() {
  prng_b = 36969 * (prng_b & 65535) + (prng_b >> 16);
  prng_a = 18000 * (prng_a & 65535) + (prng_a >> 16);
  return (prng_b << 16) + prng_a;
}

unsigned int rangeRandom(int range) {
  return ((getRandom32() >> 16) * range) >> 16;
}

// EOF