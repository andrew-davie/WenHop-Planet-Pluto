#include "defines_dasm.h"

#include "cdfjplus.h"
#include "main.h"
#include "sound.h"

#include "colour.h"
#include "draw.h"
#include "gameState.h"
#include "grid6.h"


#define CHAMP_VOL 100
#define DURATION_COPYRIGHT 250
#define PRESENTS_LUM 8
#define FADE_SPEED 17000
#define FADE_SHIFT 16

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
static unsigned int presentsColour;


void initDataStreams_Copyright() {

    static const struct dataStreams streams[] = {

        {DSJMP1PTR, _BUF_COPYRIGHT_JUMP},

        {_DS_CP_GRP0A + 0, _BUF_COPYRIGHT_GRP + 0 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 1, _BUF_COPYRIGHT_GRP + 1 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 2, _BUF_COPYRIGHT_GRP + 2 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 3, _BUF_COPYRIGHT_GRP + 3 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 4, _BUF_COPYRIGHT_GRP + 4 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 5, _BUF_COPYRIGHT_GRP + 5 * _BUFFER_SIZE},

        {_DS_CP_PF, _BUF_COPYRIGHT_PF},

        {_DS_CP_COLUPF, _BUF_COPYRIGHT_COLUPF},
        {_DS_CP_COLUP0, _BUF_COPYRIGHT_COLUP0},
        {_DS_CP_COLUBK, _BUF_COPYRIGHT_COLUBK},
    };

    initDataStreams(streams, sizeof(streams) / sizeof(struct dataStreams));
}

void initKernel_Copyright() {

    // Note: kernel shared with GS_COUCH_COMPLIANT
    // Runs AFTER initGameState_Copyright

    setJumpVectors(_BUF_COPYRIGHT_JUMP, _copyrightLoop, _copyrightExit, _SCANLINES);
    initDataStreams_Copyright();
}


void initGameState_Copyright() {

    sound_volume = VOLUME_NONPLAYING;
    loadTrack(20, trackChamp1, CHAMP_VOL + 80, 0x54, 0);
    loadTrack(10, trackChamp2, CHAMP_VOL, 0x54, 1);

    myMemsetInt((unsigned int *)(RAM + _BUF_COPYRIGHT_GRP), 0, _BUFFER_SIZE * 6 / 4);
    myMemsetInt((unsigned int *)(RAM + _BUF_COPYRIGHT_COLUBK), 0, _BUFFER_SIZE / 4);

    presentsColour = 2 << FADE_SHIFT;

    luminance = -15;
    lumTarget = 0;
    frame = 0;
}


void VB_Copyright() {

    initDataStreams_Copyright();


    if (frame < DURATION_COPYRIGHT) {

#define CGSPACER 8
#define TOP (_SCANLINES / 2 - BAND - 15)
#define CGP (gfx_grid_champgames_champ_gif_HEIGHT)
#define BAND (CGP + CGSPACER * 2)


        unsigned char *l = RAM + _BUF_COPYRIGHT_PF;
        unsigned char *r = l + _SCANLINES;
        unsigned char *c = RAM + _BUF_COPYRIGHT_COLUPF;
        unsigned char *spc = RAM + _BUF_COPYRIGHT_COLUP0;
        unsigned char *bk = RAM + _BUF_COPYRIGHT_COLUBK;

        for (int i = 0; i < _SCANLINES; i++)
            c[i] = bk[i] = colubk;

        l += TOP;
        r += TOP;
        c += TOP;

        for (int sl = TOP; sl < TOP + 2 * BAND; sl++) {

            *l++ = 255;
            *r++ = 255;
            *spc++ = 0x6;

            // NOTE: both branches of this used to compute the exact same value
            // (tvSystem was never actually used to pick a different colour).
            // Collapsed to a single statement; if PAL/SECAM was meant to use
            // different hex constants here, those need to be supplied.
            *c++ = convertColour(sl < TOP + BAND ? 0xC4 : 0x16);
        }

        drawString(FONT_LARGE, 0xC2, 10, _BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, "=Grumpy", 66);
        while (drawNextChar())
            ;


        drawString(FONT_LARGE, 0x14, 10, _BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, "=Wizard", 82);
        while (drawNextChar())
            ;

        if (frame > 60) {
            if (presentsColour < (PRESENTS_LUM << FADE_SHIFT))
                presentsColour += FADE_SPEED;


            drawString(FONT_COMPACT, 0x14, 10, _BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, "=PRODUCT", 95);
            while (drawNextChar())
                ;
        }

        if ((RAM[_SK_ID] == _WENHOP_SK_ID) && frame > 70) {
            int col = convertColour(frame & 16 ? 0x16 : 0x12);

            draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_savekey_gif, gfx_grid_savekey_gif_HEIGHT,
                        _SCANLINES - 11 - gfx_grid_savekey_gif_HEIGHT, col);

            if (RAM[_SK_RESET]) {
                draw6Bitmap(_BUF_COPYRIGHT_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_savekey_reset_gif,
                            gfx_grid_savekey_reset_gif_HEIGHT,
                            _SCANLINES - 16 - gfx_grid_savekey_gif_HEIGHT - gfx_grid_savekey_reset_gif_HEIGHT, 6);
            }
        }
    }

    else {

        lumTarget = -15;

        if (luminance == lumTarget)
            setGameState(GS_COUCH_COMPLIANT);

        RAM[_SK_RESET] = 0;    // superfluous when singleton
    }
}

void OS_Copyright() {
    adjustLuminance(1);
}

// EOF
