#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "draw.h"
#include "grid6.h"
#include "main.h"
#include "state.h"


int colx;

void initialise_GS_CouchCompliant() {

    unsigned char *p = (unsigned char *)(RAM + _BUF_COPYRIGHT_COLUPF);
    unsigned char *q = (unsigned char *)(RAM + _BUF_COPYRIGHT_COLUP0);
    for (int i = 0; i < _SCANLINES; i++) {
        *p++ = 0;
        *q++ = 0;
    }

    colx = 0;

    frame = 0;
}

void VB_GS_CouchCompliant() {

    setPointer(DSJMP1PTR, _BUF_COPYRIGHT_JUMP);

    for (int i = 0; i < 6; i++)
        setPointer(_DS_CP_GRP0A + i, _BUF_COPYRIGHT_GRP + i * _SCANLINES);

    setPointer(_DS_CP_PF, _BUF_COPYRIGHT_PF);

    setPointer(_DS_CP_COLUPF, _BUF_COPYRIGHT_COLUPF);
    setPointer(_DS_CP_COLUP0, _BUF_COPYRIGHT_COLUP0);


    if (frame < 180) {

#define COUCH_BASE 50

        if (++colx < 40) {

            int clr = colx >> 2;
            if (clr > 8)
                clr = 8;
            //            clr = clr & 0xFE;

            // The blue 'shadow'
            draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_couch_compliant_gif, 1, COUCH_BASE + colx,
                        (clr) ? 0x90 + clr : 0);

            draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_couch_compliant_gif,
                        gfx_grid_couch_compliant_gif_HEIGHT, COUCH_BASE + 1 + colx, clr);

            draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_compliant_gif,
                        gfx_grid_compliant_gif_HEIGHT, COUCH_BASE + 30 + colx, clr ? 0xD0 + (clr >> 1) : 0);
        }

        // fade out shadow
        if (frame > 70 && !(frame & 3)) {
            unsigned char *c = RAM + _BUF_COPYRIGHT_COLUP0;
            for (int i = COUCH_BASE; i < COUCH_BASE + 39; i++) {
                if (c[i] && (--c[i] & 0xF) == 0xF)
                    c[i] = 0;
            }
        }
    }

    else
        setGameState(GS_RAINBOW);
}

void OS_GS_CouchCompliant() {
}

// EOF
