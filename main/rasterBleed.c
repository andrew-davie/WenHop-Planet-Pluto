#include <assert.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "main.h"
#include "random.h"
#include "rasterBleed.h"


typedef struct {
    const ScanLine *const *frames;
    int count;
} ScanFrame;

const ScanFrame frame_set[MAX_RASTERBLEED_FRAME] = {
    // [LENA_FRAME] = {lena_screen_frames, LENA_NUM_FRAMES},    //
    [PIRATE_FRAME] = {pirate_screen_frames, PIRATE_NUM_FRAMES},    //

    // [PARROT_FRAME] = {parrot_screen_frames, PARROT_NUM_FRAMES},    //
    [SKULL_FRAME] = {skull_screen_frames, SKULL_NUM_FRAMES},    //
    //[ZPH_FRAME] = {zph_screen_frames, ZPH_NUM_FRAMES},          //
    //[SAM_FRAME] = {sam_screen_frames, SAM_NUM_FRAMES},          //
    //[DH_FRAME] = {dh_screen_frames, DH_NUM_FRAMES},                      //
    //[TOYSTORY_FRAME] = {toystory_screen_frames, TOYSTORY_NUM_FRAMES},    //
};

// static_assert(sizeof(frame_set) / sizeof(frame_set[0]) == MAX_RASTERBLEED_FRAME, "frame_set/enum mismatch");


void rasterBleed(int image, int y) {

    if (image < 0 || image >= MAX_RASTERBLEED_FRAME || frame_set[image].count == 0    //
        || y < -127 || y >= _SCANLINES)
        return;

    static int f[MAX_RASTERBLEED_FRAME] = {0};


    if (++f[image] >= frame_set[image].count)
        f[image] = 0;

    const ScanLine *frm = frame_set[image].frames[f[image]];

    int height = 128;

    if (y < 0) {
        height += y;
        frm -= y;
        y = 0;
    }

    if (y + height >= _SCANLINES) {
        height = _SCANLINES - 1 - y;
    }


    unsigned char *dest = (unsigned char *)(RAM + _BUF_RASTER_BLEED_GRP) + y;
    unsigned char *colr = (unsigned char *)(RAM + _BUF_RASTER_BLEED_COLUP0) + y - 2;

    for (int i = 0; i < height; i++) {
        for (int col = 0; col < 6; col++)
            *(dest + col * _BUFFER_SIZE) = frm[i].bits[col];
        dest++;
        *colr++ = convertColour(frm[i].colour);
    }
}

// EOF
