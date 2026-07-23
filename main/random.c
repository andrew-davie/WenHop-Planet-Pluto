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

    // SK_ODOMETER only varies boot-to-boot if a real SaveKey/AtariVox with
    // prior play history is plugged in -- without one (no device, or the
    // very first-ever boot), the two lines above are identical every single
    // power-up, so the whole PRNG stream from here on is bit-for-bit
    // deterministic -- including randomizeStarAngle()'s draw for the very
    // first cave (see swipe.c/schedule.c), which is what actually exposed
    // this. RAM[_INTIM] is the RIOT's free-running countdown timer; by the
    // time this runs the 6502 has already been executing for at least a
    // frame to request this ARM call, so its value carries real per-boot
    // timing jitter that exists with or without a peripheral connected.
    // Spread the single noisy byte across the whole word before mixing in,
    // rather than XOR-ing it straight into the low byte only.
    prng_a ^= RAM[_INTIM] * 2654435761u;    // Knuth's multiplicative hash constant
    prng_b ^= RAM[_INTIM] * 40503u;         // a different odd multiplier -- keeps a/b decorrelated
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