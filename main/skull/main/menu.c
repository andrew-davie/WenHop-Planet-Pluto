// clang-format off

#include <limits.h>

#include "defines.h"
#include "defines_cdfj.h"
#include "defines_from_dasm_for_c.h"

#include "main.h"
#include "menu.h"

#include "bitpatterns.h"
 #include "colour.h"
#include "random.h"
 #include "sound.h"


static int eyeX = 0;
static int eyeY;
int rndm = 0;
static int sin = 0;


static int eyeXOffset;
static int eyeYOffset;
static int eyeRelocate;

// clang-format on

bool enableICC = true;
// int menuLineTVType;

static int mustWatchDelay;
static int frame;
unsigned int detectedPeriod;

int thumbnailSpeed;

unsigned int sline = 0;

void detectConsoleType() {

    switch (frame) {

    case 0:

        mm_tv_type = TV_TYPE = 0; // force NTSC frame

        T1TC = 0;
        T1TCR = 1;
        break;

    case DETECT_FRAME_COUNT: {

        detectedPeriod = T1TC;

        static const struct fmt {

            int frequency;
            unsigned char format;

        } mapTimeToFormat[] = {

#define NTSC_70MHZ (0xB240F6 * DETECT_FRAME_COUNT / 10)
#define PAL_70MHZ (0xB3E40D * DETECT_FRAME_COUNT / 10)
#define SECAM_70MHZ (0xB294EA * DETECT_FRAME_COUNT / 10)

            {
                NTSC_70MHZ,
                NTSC,
            },
            {
                SECAM_70MHZ,
                SECAM,
            },
            {
                PAL_70MHZ,
                PAL_60,
            },

#if ENABLE_60MHZ_AUTODETECT

#define NTSC_60MHZ (0x98EB2F * DETECT_FRAME_COUNT / 10)
#define PAL_60MHZ (0x9A0EEF * DETECT_FRAME_COUNT / 10)
#define SECAM_60MHZ (((PAL_60MHZ - NTSC_60MHZ) / 2 + NTSC_60MHZ) * DETECT_FRAME_COUNT / 10)

            {
                NTSC_60MHZ,
                NTSC,
            },
            {
                SECAM_60MHZ,
                SECAM,
            },
            {
                PAL_60MHZ,
                PAL_60,
            },
#endif
        };

        int delta = INT_MAX;
        for (unsigned int i = 0; i < sizeof(mapTimeToFormat) / sizeof(struct fmt); i++) {

            // actualScore = detectedPeriod >> 16;
            int dist = detectedPeriod - mapTimeToFormat[i].frequency;
            if (dist < 0)
                dist = -dist;

            if (dist < delta) {
                delta = dist;
                mm_tv_type = mapTimeToFormat[i].format;
            }
        }

        //        mm_tv_type = SECAM; // tmp
        /*menuLineTVType = */ TV_TYPE = mm_tv_type;

        break;
    }

    default:
        break;
    }

    frame++;
}

void zeroBuffer(int *buffer, int size) {
    for (int i = 0; i < size; i++)
        *buffer++ = 0;
}

void clearBuffer(int *buffer, int size) {

    for (int i = 0; i < size; i++)
        *buffer++ = getRandom32();
}

void initMenuDatastreams() {

    static const struct ptrs {

        unsigned char dataStream;
        unsigned short buffer;

    } streamInits[] = {

        {_DS_PF0_LEFT, _BUF_MENU_PF0_LEFT},
        {_DS_PF1_LEFT, _BUF_MENU_PF1_LEFT},
        {_DS_PF2_LEFT, _BUF_MENU_PF2_LEFT},
        {_DS_PF0_RIGHT, _BUF_MENU_PF0_RIGHT},
        {_DS_PF1_RIGHT, _BUF_MENU_PF1_RIGHT},
        {_DS_PF2_RIGHT, _BUF_MENU_PF2_RIGHT},
        {_DS_AUDV0, _BUF_AUDV},
        {_DS_AUDC0, _BUF_AUDC},
        {_DS_AUDF0, _BUF_AUDF},
#if __ENABLE_ATARIVOX
        {_DS_SPEECH, _BUF_SPEECH},
#endif
        {_DS_COLUPF, _BUF_MENU_COLUPF},
        {_DS_COLUP0, _BUF_MENU_COLUP0},
        // {_DS_GRP0a, _BUF_MENU_GRP0A},
        // {_DS_GRP1a, _BUF_MENU_GRP1A},
        // {_DS_GRP0b, _BUF_MENU_GRP0B},
        // {_DS_GRP1b, _BUF_MENU_GRP1B},
        // {_DS_GRP0c, _BUF_MENU_GRP0C},
        // {_DS_GRP1c, _BUF_MENU_GRP1C},
        {0x21, _BUF_JUMP1},

    };

    for (unsigned int i = 0; i < sizeof(streamInits) / sizeof(struct ptrs); i++)
        setPointer(streamInits[i].dataStream, streamInits[i].buffer);

    // QINC[_DS_SPEECH] = 0;
}

#if ENABLE_ANIMATING_MAN
const unsigned char manPushing[][73] = {

    {
        72,  0,   0,
        0,   14,  14,
        0,   30,  30,
        0,   24,  28,
        4,   16,  90,
        74,  16,  92,
        76,  64,  89,
        89,  64,  125,
        125, 97,  113,
        113, 115, 115,
        115, 127, 126,
        127, 124, 124,
        124, 120, 104,
        120, 240, 240,
        240, 32,  32,
        32,  192, 0,
        0,   160, 0,
        0,   240, 0,
        0,   120, 0,
        0,   120, 0,
        0,   208, 0,
        0,   0,   144,
        0,   0,   144 + 64 + 8,
        0,   0,   144 + 64 + 8,
        0,
    },

    {
        72,  28,  28,  0,   60,  60,  0,   48,  56,          8,   32,  180,         148, 32,  184,
        152, 128, 176, 176, 128, 186, 186, 224, 242,         242, 242, 242,         242, 254, 254,
        254, 126, 124, 126, 124, 108, 124, 112, 112,         112, 112, 112,         112, 16,  16,
        16,  96,  0,   0,   80,  0,   0,   120, 0,           0,   120, 0,           0,   88,  0,
        0,   88,  0,   0,   0,   72,  0,   0,   72 + 32 + 4, 0,   0,   72 + 32 + 4, 0,
    },
};
#endif

//char RGB[6];

void doDrawBitmap(const unsigned char *shape, int x, int y) {

    unsigned char *pf1L = RAM + _BUF_MENU_PF1_LEFT + y;
    unsigned char *pf2L = pf1L + _ARENA_SCANLINES;
    unsigned char *pf1R = pf2L + _ARENA_SCANLINES;
    unsigned char *pf2R = pf1R + _ARENA_SCANLINES;

    int size = shape[0];
    const unsigned char *bf = shape + 1;

    int baseRoll = roller;

    union g {
        int bGraphic;
        unsigned char g[4];
    } gfx;

    union masker {
        int mask;
        unsigned char mask2[4];
    } mask;

    for (int i = 0; i < size; i += 3) {

        mask.mask = ((bf[0] | bf[1] | bf[2]) << x) ^ 0xFFFFFFFF;

        for (int line = 0; line < 3; line++) {

            gfx.bGraphic = bf[baseRoll] << x;

            *pf1L = (*pf1L & mask.mask2[3]) | gfx.g[3];
            *pf1R = (*pf1R & BitRev[mask.mask2[0]]) | BitRev[gfx.g[0]];
            *pf2R = (*pf2R & mask.mask2[1]) | gfx.g[1];
            *pf2L = (*pf2L & BitRev[mask.mask2[2]]) | BitRev[gfx.g[2]];

            if (++baseRoll > 2)
                baseRoll = 0;

            pf1L++;
            pf1R++;
            pf2L++;
            pf2R++;
        }

        bf += 3;
    }
}

// clang-format on

void drawPalette(const unsigned char *palette) {

    unsigned char *p = RAM + _BUF_MENU_COLUPF;

    unsigned char pal[3];

    int baseRoller = roller;
    for (int i = 0; i < 3; i++) {
        pal[i] = convertColour(palette[baseRoller]);
        if (++baseRoller > 2)
            baseRoller = 0;
    }

    for (int i = 0; i < _ARENA_SCANLINES; i += 3) {
        p[i] = pal[0];
        p[i + 1] = pal[1];
        p[i + 2] = pal[2];
    }
}

// clang-format on

void SchedulerMenu() {}

// int showAuthor;

void initKernel(int kernel) {

    //    showAuthor = 10;

    // T1TC = 0;
    // T1TCR = 1; // tmp

    KERNEL = kernel;
    setJumpVectors(_MENU_KERNEL, _EXIT_MENU_KERNEL);

    //    initAudio();
    killRepeatingAudio();

    ARENA_COLOUR = 1;

    P0_X = 80; // sets the menu sprites position
    P1_X = 88;

    sound_max_volume = VOLUME_MAX;

//    zeroBuffer((int *)(RAM + _BUF_MENU_PF1_LEFT), 6 * _ARENA_SCANLINES / 4);

    switch (kernel) {

    case KERNEL_COPYRIGHT:

        frame = 0;          // for auto-detect
        currentPalette = 0; // 4;

        mustWatchDelay = 15;
        break;

    // case KERNEL_RAGE:
    case KERNEL_MENU:

        sound_volume = VOLUME_NONPLAYING;

        eyeX = 0;
        eyeY = 0;
        eyeRelocate = 0;

        if (sound_volume < VOLUME_NONPLAYING)
            sound_volume++; // = VOLUME_MAX;
        else if (sound_volume > VOLUME_NONPLAYING)
            sound_volume--;

        break;
    }
}

void MenuOverscan() {

    initMenuDatastreams();
    playAudio();

    if (KERNEL == KERNEL_COPYRIGHT)
        detectConsoleType();

    else

        clearBuffer((int *)(RAM + _BUF_MENU_PF1_LEFT), (6*_ARENA_SCANLINES/4));
}

// clang-format off


const unsigned char componentskulljawa[] = {

    12*3,
    0b00000110, 0b10000000, 0b00000100, //0
    0b10000111, 0b01000000, 0b00000110, //1
    0b10010011, 0b00010100, 0b01000110, //2
    0b11111111, 0b00000000, 0b11010110, //3
    0b11011101, 0b00000010, 0b11111110, //4
    0b11111111, 0b00000000, 0b11111110, //5
    0b01111000, 0b10000100, 0b11111110, //6
    0b00101000, 0b11011110, 0b11111100, //7
    0b00001100, 0b11111000, 0b11111100, //8
    0b11001000, 0b11111000, 0b11111100, //9
    0b00011000, 0b11001100, 0b11111000, //10
    0b10101000, 0b11010000, 0b01111000, //11
};

const unsigned char componentskulljawb[] = {

    11*3,
    0b00000001, 0b00000001, 0b00000000, //0
    0b00000001, 0b00000000, 0b00000001, //1
    0b00000000, 0b00000001, 0b00000001, //2
    0b00000001, 0b00000001, 0b00000001, //3
    0b00000010, 0b00000001, 0b00000001, //4
    0b00000010, 0b00000001, 0b00000001, //5
    0b00000011, 0b00000001, 0b00000001, //6
    0b00000011, 0b00000000, 0b00000001, //7
    0b00000000, 0b00000000, 0b00000001, //8
    0b00000000, 0b00000000, 0b00000001, //9
    0b00000001, 0b00000001, 0b00000000, //10
};

// clang-format on

static int firstTime = 500;

void drawICCSkull() {

#define GAME_RESET_PRESSED (!(SWCHB & 0x01))
#define GAME_SELECT_PRESSED (!(SWCHB & 0x02))

    if (firstTime || (GAME_RESET_PRESSED && GAME_SELECT_PRESSED)) {

        // if (firstTime)
        //     firstTime--;

        static int skully = 15;

        //        unsigned char skullPalette[] = {0x54, 0x66, 0x28}; // green eyes!
        unsigned char skullPalette[] = {0xF2, 0xF4, 0xF6};
        drawPalette(skullPalette);

        if (rndm)
            rndm -= (rndm >> 6) + 1;

        if (!rangeRandom(60)) {
            skully = rangeRandom(3) * 3;
            rndm += 1000;
        }

        if (!rangeRandom(rndm >> 8))
            return;

        int bone1y = sinoid[(++sin >> 3) & 15] * 3;

        static const short shape[] = {
            __componentbone1c,
            __componentbone1d,
            __componentbone1b,
            __componentbone1a,
        };

        static const unsigned char yoff[] = {93 - 6, 60, 33, 0};
        for (int i = 0; i < 4; i++)
            doDrawBitmap(EXTERNAL(shape[i]), i * 8, 30 + yoff[i] + bone1y);

        static const short shape2[] = {
            __componentbone2c,
            __componentbone2d,
            __componentbone2b,
            __componentbone2a,
        };

        static const unsigned char yoff2[] = {0, 33, 69, 108};
        for (int i = 0; i < 4; i++)
            doDrawBitmap(EXTERNAL(shape2[i]), i * 8, 18 + yoff2[i] + 21 - bone1y);

        unsigned char *p = RAM + _BUF_MENU_PF2_LEFT + 60 + skully;
        unsigned char *q = RAM + _BUF_MENU_PF2_RIGHT + 60 + skully;

        for (int block = 0; block < 42; block++) {
            *p++ = 0;
            *q++ = 0;
        }
        for (int block = 42; block < 69; block++) {
            *p++ &= 0b00000011;
            *q++ &= 0b00000111;
        }
        for (int block = 69; block < 96; block++) {
            *p++ &= 0b00001111;
            *q++ &= 0b00001111;
        }

        int jawy = (sinoid[sin & 15] >> 1) * 3;

        doDrawBitmap(componentskulljawa, 11, skully + 141 - 3 + jawy);
        doDrawBitmap(componentskulljawb, 19, skully + 129 + jawy);

        doDrawBitmap(EXTERNAL(__componentskulla), 8, skully);
        doDrawBitmap(EXTERNAL(__componentskullb), 16, skully);

        if (!(sin & 3)) {

            int difference = eyeXOffset - eyeX;
            eyeX += (difference > 0) - (difference < 0);
            difference = eyeYOffset - eyeY;
            eyeY += (difference > 0) - (difference < 0);

            if (!eyeRelocate--) {
                eyeXOffset = rangeRandom(3) - 1;
                eyeYOffset = rangeRandom(3) - 1;
                eyeRelocate = 10 + rangeRandom(10);
            }
        }

        doDrawBitmap(EXTERNAL(__skullEye), 8 + eyeX, 73 + eyeY * 3 + skully);
        doDrawBitmap(EXTERNAL(__skullEye), 15 + eyeX, 73 + eyeY * 3 + skully);
    }
}

void handleMenuVB() {
    interleaveColour();
    drawICCSkull();
}

void MenuVerticalBlank() {

#if __ENABLE_ATARIVOX
    processSpeech();
#endif
    // T1TC = 0;
    // T1TCR = 1; // tmp

    switch (KERNEL) {

    case KERNEL_COPYRIGHT: // VBlank

        if (!--mustWatchDelay) {
            initKernel(KERNEL_MENU);
            return;
        }
        break;

    case KERNEL_MENU:
        handleMenuVB();
        break;
    }
}

// EOF