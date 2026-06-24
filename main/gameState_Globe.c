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

#include "../gfx/fontcompact.h"

#define CHAMP_VOL 100
#define DURATION_GLOBE 50
#define PRESENTS_LUM 8
#define FADE_SPEED 17000
#define FADE_SHIFT 16

static int id_y, ystep;
const unsigned char *thePalette;
int infoPhase;
int wait;
int planet;

struct STARS {

    unsigned int x;
    unsigned int y;
    unsigned char colour;
} stars[10];


void initStars() {

    for (unsigned int i = 0; i < sizeof(stars) / sizeof(stars[0]); i++) {
        stars[i].x = rangeRandom(40) << 8;
        stars[i].y = rangeRandom(66) << 8;
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

    planet = 0;
    initPlanet(planet);
    id_y = _SCANLINES << 3;
    ystep = -26;

    infoPhase = 0;
    wait = 100;

    sound_max_volume = VOLUME_MAX;
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


const char *planetInfo[10] = {
    // clang-format off

    "=\"Squabbling|=rock whose|=dominant|=species spent|=millennia|=figuring out|=how to leave|=and, mostly,|=didn't bother#",
    "=\">The name was|=chosen by|=committee,|=and somehow|=it was the|=kindest|=option on|=the table#",                         // 8
    "=\"Dim flatulent|=gas giant that|=smells like|=regret and|=exists solely|=to remind|=near systems|=what failure|=looks like#",    // 6
    "=\"Permanently|=shrouded in|=a haze that|=scientists|=politely call|='organic|=particulate'|=and everyone|=else calls|=filth#",    // 0
    "=\"Named after|=the first|=explorer to|=land there,|=who...|=immediately...|=wished he|=hadn't#",    // 1
    "=\"A liquid world,|=and trust us,|=you do not|=want to know|=what the|=liquid is#",        // 2
    "=\"Hot and|=bothered#",                                               // 4
    "=\"Tribble|=trouble#",                                                // 5
    "=\"Wet grey lump|=that even its|=own moons|=try to stay|=away from#",    // 7
    "=\"Mostly|=harmless#",     // 3

    // clang-format on
};


const char *review[] = {
    ">+++++",    // 0
    ">*++++",    // 1
    ">**+++",    // 2
    ">***++",    // 3
    ">****+",    // 4
    ">*****",    // 5
};

const char *planetInfoName[10] = {
    ">EARTH",       // 0
    ">SPUTUM",      // 1
    ">NEPTUNE",     // 2
    ">BRIMSTON",    // 3
    ">SKUMVEIL",    // 4
    ">GRUNTHOS",    // 5
    ">SWILL",       // 6
    ">LICHONI",     // 7
    ">MUCKSPON",    // 8
    ">TODO",        // 9
};

unsigned char planetNameColour[10] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

unsigned char pic;

unsigned char planetInfoColour[10] = {
    0x18,    // 0
    0x28,    // 1
    0x38,    // 2
    0x48,    // 3
    0x58,    // 4
    0x68,    // 5
    0x78,    // 6
    0x88,    // 7
    0x88,    // 8
    0x88,    // 9
};

const char *planetPhysics[10] = {
    ">6900 km}>9.81 m/s^",      // 3
    ">1500 km}>4.8 m/s^",       // 0
    ">9874 km}>1.5 m/s^",       // 1
    ">14566 km}>4.3 m/s^",      // 2
    ">42000 km}>222.4 m/s^",    // 4
    ">89000 km}>11.15 m/s^",    // 5
    ">6800 km}>2.3 m/s^",       // 6
    ">12400 km}>16.8 m/s^",     // 7
    ">4522 km}>22.2 m/s^",      // 8
    ">4522 km}>22.2 m/s^",      // 9
};


void initGameState_Globe() {

    myMemsetInt((unsigned int *)(RAM + _GLOBE_BUFFERS_START), 0, _GLOBE_BUFFERS_SIZE / 4);
    myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_COLUP0), 0x58585858, _BUFFER_SIZE / 4);
    frame = 0;
}


void drawBit2(int x, int y, unsigned char colour) {

    colour |= colour << 3;
    colour >>= roller;

    int line = y * 3;
    if (line < 0 || line >= _SCANLINES - 3)
        return;

    int col = x;
    if (col < 0 || col > SCREEN_TRIX_X - 1)
        return;

    unsigned char *basex = _BUF_GLOBE_PF + RAM + line;

    if (col >= 20) {
        col = 39 - col;
        basex += 3 * _BUFFER_SIZE;
    }

    basex += ((col + 4) >> 3) * _BUFFER_SIZE;

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


    if (!((basex[0] | basex[1] | basex[2]) & bit)) {

        basex[0] = (basex[0] & ~bit) | (bit & mask0);
        basex[1] = (basex[1] & ~bit) | (bit & mask1);
        basex[2] = (basex[2] & ~bit) | (bit & mask2);
    }
}

extern int planetDir;
int lastpd;
int rev;

void VB_Globe() {

    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    if (frame > DURATION_GLOBE && (RAM[_SWCHB] != 0x3F))
        setGameState(GS_MENU);

    drawPlanet(5);

    for (unsigned int i = 0; i < sizeof(stars) / sizeof(stars[0]); i++) {
        drawBit2(stars[i].x >> 8, stars[i].y >> 8, stars[i].colour);
    }

    if (!drawNextChar() && !--wait) {
        switch (infoPhase++) {

        case 0:
            initAsciiStringDraw(2, planetNameColour[planet] + 2, 8, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                                planetInfoName[planet], 0, 160);
            wait = 10;
            break;

        case 1:
            initAsciiStringDraw(1, planetNameColour[planet] - 2, 1, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                                planetPhysics[planet], 0, 176);
            wait = 50;
            pic = ((rangeRandom(15) + 1) << 4) | 8;
            break;

        case 2: {

            int lines = 1;
            unsigned char *p = planetInfo[planet];
            while (*p) {
                if (*p == '|' || *p == '}')
                    lines++;
                p++;
            }

            initAsciiStringDraw(1, 0xD8 /*planetInfoColour[planet]*/, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                                planetInfo[planet], 0, (142 - (lines * FONTCOMPACT_FONT_HEIGHT)) >> 1);
            wait = 50;
            rev = rangeRandom(6);

            break;
        }

        case 3:
            initAsciiStringDraw(1, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[rev], 0, 142);
            wait = 250;
            break;


        case 4:
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            wait = 150;
            lastpd = planetDir < 0 ? -1 : 1;
            break;

        case 5:

            if (lastpd != (planetDir < 0 ? -1 : 1) && planetDir < 0) {
                planet = nextPlanet();
                wait = 200;
                infoPhase = 0;
            } else {
                lastpd = planetDir < 0 ? -1 : 1;
                infoPhase--;
                wait++;
            }
            break;
        }
    }
}


void OS_Globe() {

    interleaveChronoColour(&roller);
    myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_PF), 0, 6 * _BUFFER_SIZE / 4);

    drawPlanet(0);

    // if (frame > 6000)


    id_y += ystep;


    if ((ystep < 0 && id_y < (60 << 3)) || (ystep > 0 && id_y > (65 << 3)))
        ystep = -(ystep >> 2);


    // int b22 = id_y >> 3;
    // if (b22 < 78)
    //     b22 = 78;


    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_menu_planetx_gif, gfx_grid_menu_planetx_gif_HEIGHT, b22,
    //             0xC6);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem1_gif, gfx_grid_lorem1_gif_HEIGHT, b22 + 30, 0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem2_gif, gfx_grid_lorem2_gif_HEIGHT, b22 + 42, 0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem3_gif, gfx_grid_lorem3_gif_HEIGHT, b22 + 54, 0x8);
    // draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem1_gif, gfx_grid_lorem1_gif_HEIGHT, b22 + 36 + 30,
    // 0x8); draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem2_gif, gfx_grid_lorem2_gif_HEIGHT, b22 + 36 +
    // 42, 0x8); draw6Bitmap(_BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, gfx_grid_lorem3_gif, gfx_grid_lorem3_gif_HEIGHT, b22 +
    // 36 + 54, 0x8);
}

// EOF
