#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "draw.h"
#include "gameState.h"
#include "grid6.h"
#include "kernels.h"
#include "main.h"


#define DURATION_COUCH 400


// Re-using COPYRIGHT buffers, but alias just for readability
#define _BUF_CC_JUMP _BUF_COPYRIGHT_JUMP
#define _BUF_CC_COLUPF _BUF_COPYRIGHT_COLUPF
#define _BUF_CC_COLUP0 _BUF_COPYRIGHT_COLUP0
#define _BUF_CC_COLUBK _BUF_COPYRIGHT_COLUBK
#define _BUF_CC_GRP _BUF_COPYRIGHT_GRP
#define _BUF_CC_PF _BUF_COPYRIGHT_PF

static int colx;


void initKernel_CouchCompliant() {
    initKernel_Copyright();
}


void initGameState_CouchCompliant() {

    myMemsetInt((unsigned int *)(RAM + _BUF_CC_COLUP0), 0, _BUFFER_SIZE / 4);
    myMemsetInt((unsigned int *)(RAM + _BUF_CC_COLUBK), 0, _BUFFER_SIZE / 4);
    myMemsetInt((unsigned int *)(RAM + _BUF_CC_COLUPF), 0, _BUFFER_SIZE / 4);

    // luminance = -15;
    lumTarget = 0;
    colx = 0;
    frame = 0;
}

#define COUCH_BASE 50
#define COUCH_SHADOW_WIDTH 40
#define COUCH_TEXT_OFFSET 30

#define COLOUR_SHADOW 0x60
#define COLOUR_COUCH 0x50
#define COLOUR_TEXT 0x90

#define SHADOW_FADE_START_FRAME 50
#define TEXT_REVEAL_FRAME 80

#define LUM_TARGET_EXIT -15

void VB_CouchCompliant() {
    initDataStreams_Copyright();    // re-use Copyright kernel

    if (frame < DURATION_COUCH) {
        if (luminance == lumTarget && (frame & 1) && (colx < COUCH_SHADOW_WIDTH))
            colx++;

        unsigned char *colours = (unsigned char *)(RAM + _BUF_CC_COLUP0);
        unsigned char *shadow_px = &colours[COUCH_BASE + colx - 1];
        *shadow_px = (*shadow_px & 0xF) | convertColour(COLOUR_SHADOW);
    }

    int couch_colour = (colx >> 2) | COLOUR_COUCH;
    draw6Bitmap(_BUF_CC_GRP, _BUF_CC_COLUP0, gfx_grid_couch_compliant_gif, gfx_grid_couch_compliant_gif_HEIGHT,
                COUCH_BASE + 1 + colx, couch_colour);

    if (frame > TEXT_REVEAL_FRAME) {
        int text_colour = convertColour((COLOUR_TEXT | ((frame >> 3) & 7)) + 4);
        draw6Bitmap(_BUF_CC_GRP, _BUF_CC_COLUP0, gfx_grid_compliant_gif, gfx_grid_compliant_gif_HEIGHT,
                    COUCH_BASE + COUCH_TEXT_OFFSET + colx, text_colour);
    }

    // fade out shadow
    if (frame > SHADOW_FADE_START_FRAME && !(frame & 7)) {
        unsigned char *colours = RAM + _BUF_CC_COLUP0;
        for (int i = COUCH_BASE; i < COUCH_BASE + COUCH_SHADOW_WIDTH; i++) {
            if ((--colours[i] & 0xF) == 0xF)
                colours[i] = 0;
        }
    }

    if (frame >= DURATION_COUCH) {
        lumTarget = LUM_TARGET_EXIT;
        if (luminance == lumTarget && gameState == nextGameState)
            setGameState(GS_RASTER_BLEED);
    }
}


void OS_CouchCompliant() {
    adjustLuminance(1);
}

// EOF
