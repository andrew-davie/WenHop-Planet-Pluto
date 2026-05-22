#include "defines_dasm.h"

#include "cdfjplus.h"

#include "random.h"


unsigned int prng_b;
unsigned int prng_a;

unsigned int cave_random_a, cave_random_b;


// unsigned int Room_random_a, Room_random_b;

// unsigned int rndX;

void initRandom() {

    prng_a = 0xFACE1234;
    prng_b = 0xBABEACED;

    prng_a ^= RAM[_SK_ODOMETER];
    prng_b ^= RAM[_SK_ODOMETER + 1];

    // for (int i = 0; i < SAVEKEY_SIZE; i++) {
    // 	prng_a ^= saveKeySolved[i] ^ saveKeyPerfect[i];
    // 	prng_b ^= saveKeyUnlocked[i];
    // }
}

unsigned int getRandom32() {
    prng_b = 36969 * (prng_b & 65535) + (prng_b >> 16);
    prng_a = 18000 * (prng_a & 65535) + (prng_a >> 16);
    return (prng_b << 16) + prng_a;
}

unsigned int rangeRandom(int range) {
    return ((getRandom32() >> 16) * range) >> 16;
}

unsigned int getCaveRandom32() {
    cave_random_b = 36969 * (cave_random_b & 65535) + (cave_random_b >> 16);
    cave_random_a = 18000 * (cave_random_a & 65535) + (cave_random_a >> 16);
    return (cave_random_b << 16) + cave_random_a;
}

// EOF