#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "draw.h"
#include "grid6.h"
#include "main.h"
#include "sound.h"
#include "state.h"

void initialise_GS_CouchCompliant() {

    frame = 0;

    // zeroBuffer((int *)(RAM + _BUF_MENU_GRP0A), (_ARENA_SCANLINES * 6) >> 2);
    // zeroBuffer((int *)(RAM + _BUF_MENU_COLUPF), _ARENA_SCANLINES >> 2);

    // ADDAUDIO(SFX_MAGIC);
}

void VB_GS_CouchCompliant() {

    // rainbow...
    static unsigned int col;
    col++;

    for (int i = 0; i < _SCANLINES; i++)
        RAM[_BUF_COUCH_COMPLIANT_COLUP0 + i] = convertColour((col + ((_SCANLINES - 1 - i) >> 2)) & 0xFF);

    setPointer(DS0PTR, _BUF_COUCH_COMPLIANT_COLUP0);

    // couch
    if (frame < 375) {

#define COUCH_BASE 50

        static int colx = -10;
        if (++colx >= 0) {

            if (colx < 40) {

                int clr = colx >> 2;
                if (clr > 8)
                    clr = 8;
                clr = clr & 0xFE;

                draw6Bitmap(_BUF_COUCH_COMPLIANT_GRP0A, _BUF_COUCH_COMPLIANT_COLUP0, gfx_grid_couch_compliant_gif, 2,
                            COUCH_BASE + colx, (clr) ? 0x8E + clr : 0);

                draw6Bitmap(_BUF_COUCH_COMPLIANT_GRP0A, _BUF_COUCH_COMPLIANT_COLUP0, gfx_grid_couch_compliant_gif,
                            gfx_grid_couch_compliant_gif_HEIGHT, COUCH_BASE + 1 + colx, clr);

                draw6Bitmap(_BUF_COUCH_COMPLIANT_GRP0A, _BUF_COUCH_COMPLIANT_COLUP0, gfx_grid_compliant_gif,
                            gfx_grid_compliant_gif_HEIGHT, COUCH_BASE + 30 + colx, clr ? 0xCE + clr : 0);
            }

            if (frame > 300 && !(frame & 3)) {
                unsigned char *c = RAM + _BUF_COUCH_COMPLIANT_COLUP0;
                for (int i = COUCH_BASE; i < COUCH_BASE + 39; i++) {
                    if (c[i] && (--c[i] & 0xF) == 0xF)
                        c[i] = 0;
                }
            }
        }
    }

    else
        setGameState(GS_RAINBOW);
}

void OS_GS_CouchCompliant() {

    playAudio();
}

// EOF
