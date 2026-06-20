#include "defines_dasm.h"

#include "cdfjplus.h"

#include "caveData.h"
#include "colour.h"
#include "draw.h"
#include "grid6.h"
#include "main.h"
#include "menuCharacterSet.h"
#include "random.h"
#include "reverseBits.h"
#include "savekey.h"
// #include "globe/main/main.h"
#include "drawPlanet.h"
#include "drawplanet.h"
#include "sound.h"

#define CHAMP_VOL 100
#define DURATION_GLOBE 50
#define PRESENTS_LUM 8
#define FADE_SPEED 17000
#define FADE_SHIFT 16

static int base = _SCANLINES;
const unsigned char *thePalette;


struct STARS {

    unsigned char x;
    unsigned char y;
    unsigned char colour;
} stars[20];

bool drawBit2(int x, int y, unsigned char colour);


void initStars() {

    for (unsigned int i = 0; i < sizeof(stars) / sizeof(stars[0]); i++) {
        stars[i].x = rangeRandom(40);
        stars[i].y = rangeRandom(66);
        stars[i].colour = rangeRandom(7) + 1;
    }
}


void initDataStreams_Globe() {

    static const struct dataStreams streams[] = {

        {_DS_GLOBE_COLUBK, _BUF_GLOBE_COLUBK},

        {_DS_GLOBE_PF0_LEFT, _BUF_GLOBE_PF},
        {_DS_GLOBE_PF1_LEFT, _BUF_GLOBE_PF + 1 * _BUFFER_SIZE},
        {_DS_GLOBE_PF2_LEFT, _BUF_GLOBE_PF + 2 * _BUFFER_SIZE},

        {_DS_GLOBE_PF0_RIGHT, _BUF_GLOBE_PF + 3 * _BUFFER_SIZE},
        {_DS_GLOBE_PF1_RIGHT, _BUF_GLOBE_PF + 4 * _BUFFER_SIZE},
        {_DS_GLOBE_PF2_RIGHT, _BUF_GLOBE_PF + 5 * _BUFFER_SIZE},

        {_DS_GLOBE_AUDV0, _BUF_AUDV},
        {_DS_GLOBE_AUDC0, _BUF_AUDC},
        {_DS_GLOBE_AUDF0, _BUF_AUDF},

        {_DS_GLOBE_COLUPF, _BUF_GLOBE_COLUPF},

        {_DS_GLOBE_COLUP0, _BUF_GLOBE_COLUP0},
        {_DS_GLOBE_COLUP1, _BUF_GLOBE_COLUP0 + _BUFFER_SIZE},

        {_DS_GLOBE_GRP0A, _BUF_GLOBE_GRP + 0 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP1A, _BUF_GLOBE_GRP + 1 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP0B, _BUF_GLOBE_GRP + 2 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP1B, _BUF_GLOBE_GRP + 3 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP0C, _BUF_GLOBE_GRP + 4 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP1C, _BUF_GLOBE_GRP + 5 * _BUFFER_SIZE},

        {DSJMP1PTR, _BUF_GLOBE_JUMP},

    };

    initDataStreams(streams, sizeof(streams) / sizeof(struct dataStreams));
}


unsigned int buf[12];


void initKernel_Globe() {

    setJumpVectors(_BUF_GLOBE_JUMP, _globeLoop, _globeExit, _SCANLINES);
    initDataStreams_Globe();


    killRepeatingAudio();

    thePalette = initPlanet(0);
    base = _SCANLINES;

    sound_max_volume = VOLUME_MAX;

    myMemsetInt((unsigned int *)(RAM + _GLOBE_BUFFERS_START), 0, _GLOBE_BUFFERS_SIZE / 4);
}


void drawPaletteGlobe(const unsigned char *palette) {

    unsigned char *p = RAM + _BUF_GLOBE_COLUPF;

    unsigned char pal[3];

    int baseRoller = roller;
    for (int i = 0; i < 3; i++) {
        pal[i] = convertColour(palette[baseRoller]);
        if (++baseRoller > 2)
            baseRoller = 0;
    }

    for (int i = 0; i < _SCANLINES; i += 3) {
        p[i] = pal[1];
        p[i + 1] = pal[2];
        p[i + 2] = pal[0];
    }
}


void initGameState_Globe() {

    myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_PF), 0, _BUFFER_SIZE * 10 / 4);

    draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_menu_planetx_gif, gfx_grid_menu_planetx_gif_HEIGHT, 170,
                0xC6);

    frame = 0;
}


void VB_Globe() {

    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    if (frame > DURATION_GLOBE && (RAM[_SWCHB] != 0x3F))
        setGameState(GS_MENU);

    drawPlanet(5);

    for (unsigned int i = 0; i < sizeof(stars) / sizeof(stars[0]); i++)
        drawBit2(stars[i].x, stars[i].y, stars[i].colour);


    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_couch_compliant_gif, gfx_grid_couch_compliant_gif_HEIGHT,
    //             80, 0x28);


    // if (base > 80)
    //     base--;

    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_menu_planetx_gif, gfx_grid_menu_planetx_gif_HEIGHT, 170,
    //             0xC6);

    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem1_gif, gfx_grid_lorem1_gif_HEIGHT, base + 30, 0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem2_gif, gfx_grid_lorem2_gif_HEIGHT, base + 42, 0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem3_gif, gfx_grid_lorem3_gif_HEIGHT, base + 54, 0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem1_gif, gfx_grid_lorem1_gif_HEIGHT, base + 36 + 30,
    // 0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem2_gif, gfx_grid_lorem2_gif_HEIGHT, base
    // + 36 + 42,
    //             0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem3_gif, gfx_grid_lorem3_gif_HEIGHT, base
    // + 36 + 54,
    //             0x8);
}


bool drawBit2(int x, int y, unsigned char colour) {

    colour |= colour << 3;
    colour >>= roller;

    int scrollY = 0;
    int scrollX = 0;

    int line = (y - ((scrollY) >> 16)) * 3;
    if (line < 0 || line >= _SCANLINES - 3)
        return false;

    int col = x - ((scrollX /** CHAR_TRIX_X*/) >> 16);
    if (col < 0 || col > SCREEN_TRIX_X - 1)
        return false;

    unsigned char *base = _BUF_GLOBE_PF + RAM + line;

    if (col >= 20) {
        col -= 20;
        base += 3 * _BUFFER_SIZE;
    }

    base += ((col + 4) >> 3) * _BUFFER_SIZE;

    int shift;
    if (col < 4)
        shift = col + 4;

    else if (col < 12)
        shift = 11 - col;

    else
        shift = (col - 12);

    int bit = 1 << shift;

    unsigned char mask0 = (colour) << shift;
    unsigned char mask1 = (colour >> 1) << shift;
    unsigned char mask2 = (colour >> 2) << shift;


    if (!((base[0] | base[1] | base[2]) & bit)) {


        base[0] = (base[0] & ~bit) | (bit & mask0);
        base[1] = (base[1] & ~bit) | (bit & mask1);
        base[2] = (base[2] & ~bit) | (bit & mask2);
    }

    return true;
}


void OS_Globe() {

    interleaveChronoColour(&roller);
    setPFColours((unsigned char *)(RAM + _BUF_GLOBE_COLUPF));

    // unsigned int *p = (unsigned int *)(RAM + _BUF_GLOBE_PF);
    // for (int i = 0; i < 6 * _BUFFER_SIZE / 4; i++)
    //     *p++ = 0;

    drawPlanet(0);
}

// EOF
