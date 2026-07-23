#include "defines_dasm.h"

#include "cdfjplus.h"

#include "random.h"


unsigned int prng_b;
unsigned int prng_a;


void initRandom() {

    prng_a = 0xFACE1234;
    prng_b = 0xBABEACED;

    // getRandom32() combines the two generators as (prng_b << 16) + prng_a,
    // so its top 16 bits are almost entirely prng_b -- and rangeRandom()
    // only ever reads those top bits (see "getRandom32() >> 16" below).
    // Odometer increments by 1 per play, so RAM[_SK_ODOMETER + 1] (its high
    // byte, the only thing that used to feed prng_b) only changes once every
    // 256 plays: any single rangeRandom() draw taken early in a session --
    // e.g. randomizeStarAngle()'s star-rotation pick -- came out the same
    // for ~256 consecutive plays even though prng_a (fed by the low byte,
    // changing every play) was varying fine underneath. Verified against a
    // standalone model of this exact generator before landing this fix.
    //
    // Fix: feed BOTH odometer bytes into BOTH generators (byte-swapped for
    // prng_b), so an ordinary single-play odometer increment perturbs
    // prng_b's seed too, not just prng_a's.
    unsigned char odoLo = RAM[_SK_ODOMETER];
    unsigned char odoHi = RAM[_SK_ODOMETER + 1];
    prng_a ^= odoLo | (odoHi << 8);
    prng_b ^= odoHi | (odoLo << 8);
}

unsigned int getRandom32() {
    prng_b = 36969 * (prng_b & 65535) + (prng_b >> 16);
    prng_a = 18000 * (prng_a & 65535) + (prng_a >> 16);
    return (prng_b << 16) + prng_a;
}

unsigned int rangeRandom(int range) {

    // getRandom32() is (prng_b << 16) + prng_a -- reading ">> 16" alone
    // means every draw depends almost entirely on prng_b, and barely at all
    // on prng_a (only via the rare add-carry crossing bit 16). Confirmed on
    // hardware: prng_a's half visibly varies across resets, prng_b's half
    // doesn't. XOR-folding the low half into the high half before reading it
    // means a draw varies whenever EITHER half varies, not just prng_b's.
    unsigned int v = getRandom32();
    v ^= v << 16;
    return ((v >> 16) * range) >> 16;
}

// EOF