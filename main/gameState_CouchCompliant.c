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
    initKernel_Copyright();    // re-use
}

void initGameState_CouchCompliant() {

    unsigned char *p = (unsigned char *)(RAM + _BUF_CC_COLUPF);
    unsigned char *q = (unsigned char *)(RAM + _BUF_CC_COLUP0);
    for (int i = 0; i < _SCANLINES; i++) {
        *p++ = 0;
        *q++ = 0;
    }

    colx = 0;

    frame = 0;
}

void VB_CouchCompliant() {

    setPointer(DSJMP1PTR, _BUF_CC_JUMP);

    for (int i = 0; i < 6; i++)
        setPointer(_DS_CP_GRP0A + i, _BUF_CC_GRP + i * _SCANLINES);

    setPointer(_DS_CP_PF, _BUF_CC_PF);

    setPointer(_DS_CP_COLUPF, _BUF_CC_COLUPF);
    setPointer(_DS_CP_COLUP0, _BUF_CC_COLUP0);


    if (frame < DURATION_COUCH) {

#define COUCH_BASE 50

        if (colx < 40)
            colx++;

        int clr = colx >> 2;

        unsigned char *p = (unsigned char *)(RAM + _BUF_CC_COLUP0);
        p[COUCH_BASE + colx - 1] |= convertColour(0x90);

        draw6Bitmap(_BUF_CC_GRP, _BUF_CC_COLUP0, gfx_grid_couch_compliant_gif, gfx_grid_couch_compliant_gif_HEIGHT,
                    COUCH_BASE + 1 + colx, clr);

        if (frame > 80)
            draw6Bitmap(_BUF_CC_GRP, _BUF_CC_COLUP0, gfx_grid_compliant_gif, gfx_grid_compliant_gif_HEIGHT,
                        COUCH_BASE + 30 + colx, (frame & 16) ? 0x26 : 0x2A);


        // fade out shadow
        if (frame > 50 && !(frame & 3)) {
            unsigned char *c = RAM + _BUF_CC_COLUP0;
            for (int i = COUCH_BASE; i < COUCH_BASE + 40; i++) {
                if (c[i] && (--c[i] & 0xF) == 0xF)
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
