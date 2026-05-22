#include "reverseBits.h"

// COMPILE-TIME REVERSE BITS IN BYTE
#define RVS(a)                                                                                                         \
    (((((a) >> 0) & 1) << 7) | ((((a) >> 1) & 1) << 6) | ((((a) >> 2) & 1) << 5) | ((((a) >> 3) & 1) << 4) |           \
     ((((a) >> 4) & 1) << 3) | ((((a) >> 5) & 1) << 2) | ((((a) >> 6) & 1) << 1) | ((((a) >> 7) & 1) << 0))

#define P0(a) RVS(a)
#define P1(a) P0(a), P0(a + 1)
#define P2(a) P1(a), P1(a + 2)
#define P3(a) P2(a), P2(a + 4)
#define P4(a) P3(a), P3(a + 8)
#define P5(a) P4(a), P4(a + 16)
#define P6(a) P5(a), P5(a + 32)
#define P7(a) P6(a), P6(a + 64)
#define P8(a) P7(a), P7(a + 128)

// Want to call RVS(n) for 0-255 values. The weird #defines above allows a
// single-call It's effectively a recursive power-of-two call of the base RVS
// macro

const unsigned char reverseBits[256] = {
    P8(0),
};
