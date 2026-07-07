#pragma once

// #define SCREEN_BYTES_PER_ROW 6

// typedef struct {
//     unsigned char colour;
//     unsigned char bits[SCREEN_BYTES_PER_ROW];
// } ScanLine;


#include "../tools/buzz.h"
#include "../tools/dh.h"
#include "../tools/donald.h"
#include "../tools/lena.h"
#include "../tools/obama.h"
#include "../tools/playboy.h"
#include "../tools/sam.h"
#include "../tools/skull.h"
#include "../tools/toystory.h"
#include "../tools/zph.h"

enum {
    SKULL_FRAME,
    ZPH_FRAME,
    SAM_FRAME,
    DH_FRAME,
    TOYSTORY_FRAME,

    MAX_FRAME,
};


typedef struct {
    unsigned char r, g, b;
} RGB;

extern const int FRAME_SETS_MAX;
extern const ScanLine *const *const frame_sets[];

unsigned char blendAtariColour(unsigned char a, unsigned char b, int xPercent);
void rasterBleed(int image, int y);

// EOF
