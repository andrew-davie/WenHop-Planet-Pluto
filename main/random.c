#include "defines_dasm.h"

#include "cdfjplus.h"

#include "random.h"


unsigned int prng_b;
unsigned int prng_a;


void initRandom() {

    prng_a = 0xFACE1234;
    prng_b = 0xBABEACED;

    prng_a ^= RAM[_SK_ODOMETER];
    prng_b ^= RAM[_SK_ODOMETER + 1];
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