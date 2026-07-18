#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "draw.h"
#include "drawGridPreview.h"
#include "drawplanet.h"
#include "joystick.h"
#include "life.h"
#include "main.h"
#include "planet.h"
#include "random.h"
#include "scroll.h"
#include "sound.h"

#include "../gfx/fontcompact.h"
#include "../gfx/fontlarge.h"


//------------------------------------------------------------------------------
// Notes on Huffman encoding of remarks

#define HUFFMAN

// Remarks are in planetInfo.c
// These remarks are used as-is if HUFFMAN is NOT defined.
// However, if HUFFMAN is defined, then the compressed version of these remarks is used.
// These are generated via tools/huffman.py
// Usage (from main)..
// python3 tools/huffman.py path/to/planetInfo.c  path/to/planetHuffman
// The generated planetHuffman files are used instead of planetInfo.c

// Beware out-of-date .o files -- do a make-clean and make if you change HUFFMAN

// Strings are formatted correctly by...
// python3 tools/reformat.py
// # or
// python3 reformat.py path/to/planetInfo.c

//------------------------------------------------------------------------------

static int pif;
static int lines;
static int pa_lines;
static int infoPhase;
int wait;
static int planet = -1;
static bool finished;

const unsigned char *thePalette;


enum info {

    INFO_FADEUP,
    INFO_NAME,
    INFO_PHYSICS,
    INFO_CLEAR1,
    INFO_PLANETADVISOR,
    INFO_INFO,
    INFO_RATING1,
    INFO_RATING2,
    INFO_CLEAR,
    INFO_NEXTPLANET,
    INFO_FADE_DOWN,
};


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

/* Track 1 — Melody */
const unsigned char track1[] = {HALFNOTE
                                    /* Intro: slow opening, rise and return */
                                    e5 g5 b5 a5 g5 e5 d5 e5 QUARTERNOTE
                                        /* Main theme A — ascending flow, graceful descent */
                                        e5 g5 a5 b5 a5 g5 f5_SHARP e5 d5 e5 g5 a5 b5 c6 b5 a5 g5 f5_SHARP e5 d5 e5 g5 a5
                                            b5 c6 b5 a5 g5 f5_SHARP e5 d5 e5 HALFNOTE
                                                /* Bridge: slower, lifting */
                                                f5_SHARP g5 a5 b5 a5 g5 QUARTERNOTE
                                                    /* Development — reach higher, return through the scale */
                                                    e5 f5_SHARP g5 a5 b5 c6 b5 a5 g5 a5 b5 c6 b5 a5 g5 f5_SHARP e5 d5 e5
                                                        g5 a5 g5 f5_SHARP e5 d5 e5 f5_SHARP g5 a5 b5 a5 g5 HALFNOTE
                                                            /* Cadence: winding down */
                                                            f5_SHARP e5 d5 e5 d5 e5 FULLNOTE
                                                                /* Resolution */
                                                                e5 TRACK_LOOP};

/* Track 2 — Bass (slow harmonic support, drops low for the void) */
const unsigned char track2[] = {
    HALFNOTE
        /* Under intro */
        e4 e4 g4 g4 a4 g4 e4 d4
            /* Under main theme */
            e4 g4 a4 g4 e4 d4 c4 d4 FULLNOTE e3 /* deep drop — the void */
                e4                              /* return */
                    HALFNOTE
                        /* Under bridge */
                        g4 a4 g4 e4 d4 e4 g4 a4
                            /* Under development */
                            e4 g4 a4 g4 e4 d4 e4 g4 FULLNOTE e3 /* second deep drop */
                                g3                              /* low third */
                                    HALFNOTE
                                        /* Final approach */
                                        a4 g4 e4 d4 c4 e4 FULLNOTE e3 /* resolve into the deep */
                                            e4 TRACK_LOOP};


/* Track 1 — Melody */
const unsigned char track1b[] = {HALFNOTE
                                     /* Opening dread — Phrygian minor 2nd crawl */
                                     d5 d5_SHARP d5 c6 a5 g5 f5 d5_SHARP
                                         /* Descent through the void */
                                         d5 c6 b5 a5 g5 f5 d5_SHARP d5 QUARTERNOTE
                                             /* Tension builds — chromatic crawl upward */
                                             d5 d5_SHARP f5 g5 a5 g5 f5 d5_SHARP d5 c6 b5 a5 g5 f5 d5_SHARP d5
                                                 /* Tritone sting at the peak */
                                                 d5_SHARP f5 g5 a5 b5 c6 b5 a5 g5 f5 d5_SHARP d5 c6 a5 g5 f5 HALFNOTE
                                                     /* Heavy, inevitable descent */
                                                     d5 d5_SHARP f5 g5 f5 d5_SHARP d5 c6 FULLNOTE d5 TRACK_LOOP};

/* Track 2 — Bass */
const unsigned char track2b[] = {FULLNOTE
                                     /* Low rumble — root and third */
                                     d4 f3 d4 HALFNOTE
                                         /* Chromatic crawl */
                                         f3 g3 f3 d4 c4 d4 c4 f3 FULLNOTE e3 /* abyss drop */
                                             g3 HALFNOTE
                                                 /* Development underpinning */
                                                 d4 c4 f3 g3 d4 c4 d4 f3 FULLNOTE e3 /* second drop, deeper dread */
                                                     d4 HALFNOTE f3 g3 c4 d4 TRACK_LOOP};


// clang-format off

/* Track 1 — Melody */
const unsigned char track1c[] = {
    HALFNOTE
    /* Gentle fall */
    a5 g5 f5 e5
    FULLNOTE
    d5
    HALFNOTE
    /* Rise, settle */
    e5 g5 a5 g5
    FULLNOTE
    f5
    HALFNOTE
    /* Searching */
    e5 d5 e5 g5
    FULLNOTE
    a5
    HALFNOTE
    /* Fall again, unexpected dip */
    g5 f5 e5 d5
    c5 d5
    FULLNOTE
    d5
    TRACK_LOOP
};

/* Track 2 — Sparse harmonic pillars (raised) */
const unsigned char track2c[] = {
    FULLNOTE
    d4
    g4
    a4
    d4
    f4
    g4
    d4
    a4
    g4
    d4
    TRACK_LOOP
};
/* Track 1 — 3 note loop: slow ascending crawl */
const unsigned char track1d[] = {FULLNOTE e3 f3 g3 TRACK_LOOP};

/* Track 2 — 5 note loop: oscillates across the same cluster differently */
const unsigned char track2d[] = {FULLNOTE g3 f3 e3 f3 g3_SHARP TRACK_LOOP};
// clang-format on


void initKernel_Globe() {

    setJumpVectors(_BUF_GLOBE_JUMP, _globeLoop, _globeExit, _SCANLINES);
    initDataStreams_Globe();

    planet++;
    initPlanet(planet);

    infoPhase = INFO_FADEUP;
    wait = 50;

    sound_volume = VOLUME_PLAYING;

    const int speed = 0x40;    // rangeRandom(80) + 16;

    loadTrack(10, track1b, 40, speed, 1);
    loadTrack(0, track2b, 25, speed, 2);
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


#ifdef HUFFMAN

// from tools:
// python3 huffman.py ../main/planetInfo.c  ../main/planetHuffman
// --> generates the compressed source 'planetHuffman'.c/.h
// uses the phrases stored in 'planetInfo.c'
// If you add a phrase/character then re-run encoder

#include "planetHuffman.h"

char huffBuff[PHRASE_BUF_SIZE];

#else /* unencoded */

#include "planetInfo.c"

#define PHRASE_COUNT (sizeof(planetInfo) / sizeof(planetInfo[0]))

#endif


bool seen[PHRASE_COUNT];


const char *review[] = {
    "=+++++",    // 0
    "=*++++",    // 1
    "=**+++",    // 2
    "=***++",    // 3
    "=****+",    // 4
    "=*****",    // 5
};


void initGameState_Globe() {

    myMemsetInt((unsigned int *)(RAM + _GLOBE_BUFFERS_START), 0, _GLOBE_BUFFERS_SIZE / 4);
    myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_COLUP0), 0x58585858, _BUFFER_SIZE / 4);

    luminance = -15;
    lumTarget = 0;
    frame = 0;

    for (unsigned int i = 0; i < PHRASE_COUNT; i++)
        seen[i] = false;

    finished = false;

    initLife();
}


void VB_Globe() {


    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    drawPlanet(5);
    drawStars();


    //    life(1);

    static int nsv = VOLUME_PLAYING;

    if (!rangeRandom(100))
        nsv = VOLUME_PLAYING + rangeRandom(VOLUME_MAX - VOLUME_PLAYING);

    sound_max_volume = approach(sound_max_volume, nsv, 1);

    getJoystick();
    if (!(inpt4 & 0x80) || finished)
        setGameState(GS_GAME);
}


void OS_Globe() {

#define ADJ 0


    interleaveChronoColour(&roller);
    adjustLuminance(1);

    drawPlanet(0);
    //    drawGridPreview();

    if (!drawNextChar() && --wait < 0) {

        switch (infoPhase++) {

        case INFO_FADEUP:
            if (luminance)
                infoPhase = INFO_FADEUP;
            break;


        case INFO_NAME:

            drawString(FONT_LARGE, 0x8, 10, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planets[planet].name,
                       _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - FONTLARGE_FONT_HEIGHT - 25);
            wait = 10;
            break;


        case INFO_PHYSICS:
            drawString(FONT_COMPACT, 0x16, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planets[planet].physics,
                       _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - 25 + 5);
            wait = 200;
            break;


        case INFO_CLEAR1: {

            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            ADDAUDIO(SFX_DOGE2);


            int lastpif = pif;
            pif = rangeRandom(PHRASE_COUNT);

            int reviews = PHRASE_COUNT;
            if (reviews > 2) {
                int pif2 = pif;

                while (seen[pif]) {
                    pif++;
                    if (pif >= reviews)
                        pif = 0;

                    if (pif == pif2) {
                        for (int i = 0; i < reviews; i++)
                            seen[i] = false;
                        seen[lastpif] = true;
                    }
                }

                // pif = 0;    // tmp

                seen[pif] = true;
            }


            lines = FONTCOMPACT_FONT_HEIGHT;

#ifdef HUFFMAN

            phrase_decode(phrases[pif].data, huffBuff, PHRASE_BUF_SIZE);
            const char *p = huffBuff;

#else
            const char *p = planetInfo[pif].review;

#endif

            do {
                if (*p == '|')
                    lines += FONTCOMPACT_FONT_HEIGHT;
                if (*(p - 1) == '}')
                    lines += FONTCOMPACT_FONT_HEIGHT + (FONTCOMPACT_FONT_HEIGHT >> 1);
            } while (*++p);


#define INFO_GAP 10
#define TITLE_HEIGHT (FONTCOMPACT_FONT_HEIGHT + 2)
#define RATING_HEIGHT (FONTCOMPACT_FONT_HEIGHT)

            pa_lines = (_SCANLINES >> 1) - ((lines + (INFO_GAP * 2 + TITLE_HEIGHT + RATING_HEIGHT)) >> 1) + ADJ;
            wait = 30;
        } break;


        case INFO_PLANETADVISOR:
            drawString(FONT_COMPACT, 0x8, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, "=_Planet@Visor", pa_lines);
            pa_lines += TITLE_HEIGHT + INFO_GAP;
            wait = 25;
            break;


        case INFO_INFO:
#ifdef HUFFMAN
            drawString(FONT_COMPACT, 0xD8, 4, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, huffBuff, pa_lines);
#else
            drawString(FONT_COMPACT, 0xD8, 4, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planetInfo[pif].review, pa_lines);
#endif
            pa_lines += lines + INFO_GAP;
            wait = 50;
            break;


        case INFO_RATING1:
            drawString(FONT_COMPACT, 0x18, 0, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[0], pa_lines);
            break;


        case INFO_RATING2:
#ifdef HUFFMAN
            drawString(FONT_COMPACT, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[phrases[pif].stars], pa_lines);
#else
            drawString(FONT_COMPACT, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[planetInfo[pif].stars],
                       pa_lines);
#endif
            wait = 200;
            break;


        case INFO_CLEAR:
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            wait = 30;
            break;


        case INFO_NEXTPLANET:

            if (scalex > (SCALE_FAR - 0x1000) && planetDir > 0)
                lumTarget = -15;
            else
                infoPhase = INFO_NEXTPLANET;
            break;


        case INFO_FADE_DOWN:


            if (luminance == lumTarget)
                finished = true;

            else
                infoPhase = INFO_FADE_DOWN;

            // if (luminance == lumTarget) {

            //     myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_PF), 0, 6 * _BUFFER_SIZE / 4);
            //     //                lumTarget = 0;
            //     infoPhase = INFO_FADEUP;
            //     planet = nextPlanet();

            // }

            // else
            //     finished = true;

            break;
        }
    }
}

// EOF