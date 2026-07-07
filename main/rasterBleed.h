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

// Re-using COPYRIGHT buffers, but alias just for readability
#define _BUF_RASTER_BLEED_JUMP _BUF_COPYRIGHT_JUMP
#define _BUF_RASTER_BLEED_COLUPF _BUF_COPYRIGHT_COLUPF
#define _BUF_RASTER_BLEED_COLUP0 _BUF_COPYRIGHT_COLUP0
#define _BUF_RASTER_BLEED_COLUBK _BUF_COPYRIGHT_COLUBK
#define _BUF_RASTER_BLEED_GRP _BUF_COPYRIGHT_GRP
#define _BUF_RASTER_BLEED_PF _BUF_COPYRIGHT_PF


enum {
    SKULL_FRAME,
    // ZPH_FRAME,
    // SAM_FRAME,
    // DH_FRAME,
    // TOYSTORY_FRAME,

    MAX_RASTERBLEED_FRAME,
};


typedef struct {
    unsigned char r, g, b;
} RGB;

extern const int FRAME_SETS_MAX;
extern const ScanLine *const *const frame_sets[];

unsigned char blendAtariColour(unsigned char a, unsigned char b, int xPercent);
void rasterBleed(int image, int y);

// EOF
