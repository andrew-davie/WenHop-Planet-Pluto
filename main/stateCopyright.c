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
unsigned int presentsColour;


void initKernel_Copyright() {

    // Note: kernel shared with GS_COUCH_COMPLIANT

    setJumpVectors(_BUF_COPYRIGHT_JUMP, _kernelCopyright, _copyrightExit, _SCANLINES);
    setPointer(DSJMP1PTR, _BUF_COPYRIGHT_JUMP);
}


void initialise_GS_Copyright() {

    sound_volume = VOLUME_NONPLAYING;
    loadTrack(20, trackChamp1, CHAMP_VOL + 80, 0x54, 0);
    loadTrack(10, trackChamp2, CHAMP_VOL, 0x54, 1);

    unsigned char *p = (unsigned char *)(RAM + _BUF_COPYRIGHT_GRP);
    for (int i = 0; i < _SCANLINES * 6; i++)
        *p++ = 0;

    presentsColour = 0;

    frame = 0;
}


void VB_GS_Copyright() {

    setPointer(DSJMP1PTR, _BUF_COPYRIGHT_JUMP);

    for (int i = 0; i < 6; i++)
        setPointer(_DS_CP_GRP0A + i, _BUF_COPYRIGHT_GRP + i * _SCANLINES);

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

        for (int i = 0; i < _SCANLINES; i++)
            c[i] = 0;

        l += TOP;
        r += TOP;
        c += TOP;

        for (int sl = TOP; sl < TOP + 2 * BAND; sl++) {

            *l++ = 255;
            *r++ = 255;
            *spc++ = 0x6;

            if (tvSystem == _TV_SYSTEM_NTSC)
                *c++ = convertColour(sl < TOP + BAND ? 0x90 : 0x40);
            else
                *c++ = convertColour(sl < TOP + BAND ? 0x92 : 0x42);
        }

        // for (int sl = TOP + 2 * BAND; sl < _SCANLINES; sl++) {
        //     *l++ = *r++ = *c++ = *spc++ = 0;
        // }


        draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_champgames_champ_gif,
                    gfx_grid_champgames_champ_gif_HEIGHT, TOP + CGSPACER + 1, 8);
        draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_champgames_games_gif,
                    gfx_grid_champgames_games_gif_HEIGHT, TOP + BAND + CGSPACER + 1, 8);


#define PRESENTS_LUM 8
#define FADE_SPEED 17000
#define FADE_SHIFT 16

        if (frame > 60)
            if (presentsColour < (PRESENTS_LUM << FADE_SHIFT))
                presentsColour += FADE_SPEED;

        draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_champgames_presents_gif,
                    gfx_grid_champgames_presents_gif_HEIGHT, TOP + 2 * BAND + 10, (presentsColour >> FADE_SHIFT));

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

    else {
        setGameState(GS_COUCH_COMPLIANT);

        RAM[_SK_RESET] = 0;    // superfluous when singleton
    }
}

void OS_GS_Copyright() {
}

// EOF
