#include "defines_dasm.h"

#include "cdfjplus.h"
#include "main.h"
#include "sound.h"

#include "colour.h"
#include "draw.h"
#include "grid6.h"
#include "state.h"

#define CHAMP_VOL 100

// clang-format off

const unsigned char trackChamp1[] = {

    HALFNOTE c5 c5
    HALFNOTE d5
    FULLNOTE c5
    HALFNOTE g4 g5
    FULLNOTE e5
    TRACK_END
};

const unsigned char trackChamp2[] = {

    FULLNOTE
    c4 g3 c4 g3
    TRACK_VOLUME, CHAMP_VOL*2, 
    HALFNOTE e3 g3 e3 g3
    FULLNOTE c4
    TRACK_END
};

// clang-format on
unsigned char presentsColour;

void initialise_GS_Copyright() {

    // sets the menu sprites position
    RAM[_P0_X] = 56;
    RAM[_P1_X] = 64;

    sound_volume = VOLUME_NONPLAYING;
    loadTrack(20, trackChamp1, CHAMP_VOL + 80, 0x54, 0);
    loadTrack(10, trackChamp2, CHAMP_VOL, 0x54, 1);

    myMemsetInt((unsigned int *)(RAM + _BUF_COPYRIGHT_GRP), 0, 6 * _SCANLINES / 4);

    presentsColour = 0;

    frame = 0;
}


void VB_GS_Copyright() {

    setPointer(DSJMP1PTR, _BUF_COPYRIGHT_JUMP);

    setPointer(_DS_CP_GRP0A, _BUF_COPYRIGHT_GRP + 0 * _SCANLINES);
    setPointer(_DS_CP_GRP1A, _BUF_COPYRIGHT_GRP + 1 * _SCANLINES);
    setPointer(_DS_CP_GRP0B, _BUF_COPYRIGHT_GRP + 2 * _SCANLINES);
    setPointer(_DS_CP_GRP1B, _BUF_COPYRIGHT_GRP + 3 * _SCANLINES);
    setPointer(_DS_CP_GRP0C, _BUF_COPYRIGHT_GRP + 4 * _SCANLINES);
    setPointer(_DS_CP_GRP1C, _BUF_COPYRIGHT_GRP + 5 * _SCANLINES);

    setPointer(_DS_CP_PF, _BUF_COPYRIGHT_PF);

    setPointer(_DS_CP_COLUPF, _BUF_COPYRIGHT_COLUPF);
    setPointer(_DS_CP_COLUP0, _BUF_COPYRIGHT_COLUP0);

    if (frame < 250) {

#define CGSPACER 4
#define TOP (_SCANLINES / 2 - BAND - 15)
#define CGP (gfx_grid_champgames_champ_gif_HEIGHT)
#define BAND (CGP + CGSPACER * 2)


        unsigned char *l = RAM + _BUF_COPYRIGHT_PF;
        unsigned char *r = l + _SCANLINES;
        unsigned char *c = RAM + _BUF_COPYRIGHT_COLUPF;
        unsigned char *spc = RAM + _BUF_COPYRIGHT_COLUP0;


        for (int sl = 0; sl < TOP; sl++) {
            *l++ = *c++ = *r++ = *spc++ = 0;
        }

        for (int sl = TOP; sl < TOP + 2 * BAND; sl++) {

            *l++ = 255;
            *r++ = 255;
            *spc++ = 0x6;

            if (tvSystem == _TV_SYSTEM_NTSC)
                *c++ = convertColour(sl < TOP + BAND ? 0x90 : 0x40);
            else
                *c++ = convertColour(sl < TOP + BAND ? 0x92 : 0x42);
        }

        for (int sl = TOP + 2 * BAND; sl < _SCANLINES; sl++) {
            *l++ = *r++ = *c++ = *spc++ = 0;
        }


        draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_champgames_champ_gif,
                    gfx_grid_champgames_champ_gif_HEIGHT, TOP + CGSPACER + 1, 8);
        draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_champgames_games_gif,
                    gfx_grid_champgames_games_gif_HEIGHT, TOP + BAND + CGSPACER + 1, 8);


        if (frame > 60)
            if (presentsColour < (8 << 2))
                presentsColour++;

        draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_champgames_presents_gif,
                    gfx_grid_champgames_presents_gif_HEIGHT, TOP + 2 * BAND + 10, presentsColour >> 2);

        if ((RAM[_SK_ID] == _WENHOP_SK_ID) && frame > 100) {

            int col = convertColour(frame & 16 ? 0x16 : 0x12);

            draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_savekey_gif, gfx_grid_savekey_gif_HEIGHT,
                        _SCANLINES - 11 - gfx_grid_savekey_gif_HEIGHT, col);

            if (RAM[_SK_RESET]) {
                draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_savekey_reset_gif,
                            gfx_grid_savekey_reset_gif_HEIGHT,
                            _SCANLINES - 16 - gfx_grid_savekey_gif_HEIGHT - gfx_grid_savekey_reset_gif_HEIGHT, 6);

                if (!(frame & 15))
                    ADDAUDIO(SFX_SELECTION);
            }
        }
    }

    else
        setGameState(GS_RAINBOW);    // GS_COUCH_COMPLIANT);
}

void OS_GS_Copyright() {
    playAudio();
}

// EOF
