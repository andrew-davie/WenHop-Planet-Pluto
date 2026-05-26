#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "draw.h"
#include "gameState.h"
#include "grid6.h"
#include "kernels.h"
#include "main.h"


#define DURATION_COUCH 250


// Re-using COPYRIGHT buffers, but alias just for readability
#define _BUF_CC_JUMP _BUF_COPYRIGHT_JUMP
#define _BUF_CC_COLUPF _BUF_COPYRIGHT_COLUPF
#define _BUF_CC_COLUP0 _BUF_COPYRIGHT_COLUP0
#define _BUF_CC_GRP _BUF_COPYRIGHT_GRP
#define _BUF_CC_PF _BUF_COPYRIGHT_PF

int colx;


void initKernel_CouchCompliant() {
    initKernel_Copyright();
}


void initGameState_CouchCompliant() {

    myMemset((unsigned char *)(RAM + _BUF_CC_COLUPF), 0, _SCANLINES);
    myMemset((unsigned char *)(RAM + _BUF_CC_COLUP0), 0, _SCANLINES);

    colx = 0;
    frame = 0;
}


void VB_CouchCompliant() {

    initDataStreams_Copyright();    // re-use Copyright kernel


    if (frame < DURATION_COUCH) {

#define COUCH_BASE 50

#define COLOUR_SHADOW 0x60
#define COLOUR_COUCH 0x50
#define COLOUR_TEXT 0x90

        if (colx < 40)
            colx++;

        int clr = colx >> 2;

        unsigned char *p = (unsigned char *)(RAM + _BUF_CC_COLUP0);
        p[COUCH_BASE + colx - 1] = p[COUCH_BASE + colx - 1] & 0xF | convertColour(COLOUR_SHADOW);

        draw6Bitmap(_BUF_CC_GRP, _BUF_CC_COLUP0, gfx_grid_couch_compliant_gif, gfx_grid_couch_compliant_gif_HEIGHT,
                    COUCH_BASE + 1 + colx, clr | COLOUR_COUCH);

        if (frame > 80)
            draw6Bitmap(_BUF_CC_GRP, _BUF_CC_COLUP0, gfx_grid_compliant_gif, gfx_grid_compliant_gif_HEIGHT,
                        COUCH_BASE + 30 + colx, convertColour((COLOUR_TEXT | (frame >> 2) & 7) + 4));


        // fade out shadow
        if (frame > 50 && !(frame & 3)) {
            unsigned char *c = RAM + _BUF_CC_COLUP0;
            for (int i = COUCH_BASE; i < COUCH_BASE + 40; i++) {
                if ((--c[i] & 0xF) == 0xF)
                    c[i] = 0;
            }
        }
    }

    else
        //        setGameState(GS_RAINBOW);
        setGameState(GS_MENU);
}

void OS_CouchCompliant() {
}

// EOF
