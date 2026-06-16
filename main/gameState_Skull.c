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
#include "sound.h"

#define CHAMP_VOL 100
#define DURATION_SKULL 100
#define PRESENTS_LUM 8
#define FADE_SPEED 17000
#define FADE_SHIFT 16

static int eyeX = 0;
static int eyeY;
int rndm = 0;
static int sin = 0;


static int eyeXOffset;
static int eyeYOffset;
static int eyeRelocate;

const unsigned char sinoid[] = {0, 1, 1, 3, 5, 6, 6, 7, 7, 6, 6, 5, 3, 1, 1, 0};


//__iCC_skull
#define TRIGGER % 00011010


const unsigned char __skullEye[] = {

    18,                              //
    ________, ___X____, ________,    //
    ________, __XXX___, ________,    //
    ___X____, __X_X___, ________,    //
    ___X____, __X_X___, ________,    //
    ________, __XXX___, ________,    //
    ________, ___X____, ________,    //
};


const unsigned char __componentskulla[] = {

    52 * 3,    //

    0b11100000, 0b00010000, 0b11100000,    // 0
    0b11100000, 0b00111000, 0b11110000,    // 1
    0b01001000, 0b00110000, 0b11111000,    // 2
    0b00110100, 0b00001100, 0b11111000,    // 3
    0b01101000, 0b10010000, 0b11111100,    // 4
    0b10101100, 0b11010010, 0b11111100,    // 5
    0b01001100, 0b11110000, 0b11111110,    // 6
    0b11001100, 0b11110000, 0b11111110,    // 7
    0b10100110, 0b11111000, 0b11111110,    // 8
    0b10100100, 0b11111001, 0b11111110,    // 9
    0b11110101, 0b11111001, 0b11111110,    // 10
    0b11111101, 0b11111001, 0b11111110,    // 11
    0b11000100, 0b11111000, 0b11111111,    // 12
    0b00010110, 0b11111010, 0b11111101,    // 13
    0b11110110, 0b11111010, 0b11111101,    // 14
    0b01010100, 0b11111010, 0b11111101,    // 15
    0b01100100, 0b11111010, 0b11111101,    // 16
    0b11111110, 0b11111010, 0b11111101,    // 17
    0b11010101, 0b11111001, 0b11111110,    // 18
    0b11111101, 0b10110001, 0b11111110,    // 19
    0b11111001, 0b10101001, 0b11110110,    // 20
    0b11111101, 0b01010011, 0b11100010,    // 21
    0b11100000, 0b11100001, 0b01000010,    // 22
    0b00110000, 0b01000001, 0b10000010,    // 23
    0b00000000, 0b00000001, 0b10000010,    // 24
    0b11010010, 0b00000011, 0b10000000,    // 25
    0b01000100, 0b10000001, 0b10000010,    // 26
    0b11110100, 0b10000000, 0b10000010,    // 27
    0b00000000, 0b11100101, 0b10000010,    // 28
    0b01100001, 0b01111100, 0b10000011,    // 29
    0b01100001, 0b01111100, 0b10000011,    // 30
    0b00011000, 0b11100000, 0b01000111,    // 31
    0b01010110, 0b01010010, 0b01101111,    // 32
    0b10111100, 0b01110111, 0b01111111,    // 33
    0b01111111, 0b00111110, 0b01111111,    // 34
    0b01101110, 0b00111110, 0b01111111,    // 35
    0b00110111, 0b00010110, 0b01101110,    // 36
    0b10110110, 0b00010100, 0b01101110,    // 37
    0b11010110, 0b00110000, 0b11101110,    // 38
    0b01101010, 0b11000100, 0b11110000,    // 39
    0b11111100, 0b11100000, 0b11110000,    // 40
    0b11010100, 0b11101000, 0b11110000,    // 41
    0b01101000, 0b10001000, 0b11110000,    // 42
    0b11001000, 0b00101000, 0b11110000,    // 43
    0b00011000, 0b11111000, 0b10100000,    // 44
    0b01001100, 0b10111100, 0b00001000,    // 45
    0b01101000, 0b00011100, 0b00001100,    // 46
    0b00000100, 0b01001100, 0b00001000,    // 47
    0b00000100, 0b01001100, 0b00001000,    // 48
    0b01000100, 0b00000100, 0b00001000,    // 49
    0b00000000, 0b00000000, 0b00001100,    // 50
    0b00001100, 0b00001000, 0b00000000,    // 51
};

const unsigned char __componentskullb[] = {

    53 * 3,    //

    0b00000101, 0b00001010, 0b00000111,    // 0
    0b00001111, 0b00010110, 0b00001111,    // 1
    0b00001101, 0b00000110, 0b00011111,    // 2
    0b00001110, 0b00110100, 0b00011111,    // 3
    0b00010001, 0b00001100, 0b00111111,    // 4
    0b00010000, 0b00001111, 0b00111111,    // 5
    0b00010000, 0b01101111, 0b00111111,    // 6
    0b01011100, 0b01101111, 0b00111111,    // 7
    0b00100010, 0b00011111, 0b01111111,    // 8
    0b00100111, 0b10011111, 0b01111111,    // 9
    0b01000111, 0b11111111, 0b00111111,    // 10
    0b01110011, 0b11001111, 0b00111111,    // 11
    0b11100111, 0b11011111, 0b00111111,    // 12
    0b10010100, 0b11101111, 0b00111111,    // 13
    0b11010010, 0b10101111, 0b00111111,    // 14
    0b11011110, 0b10101111, 0b00111111,    // 15
    0b01000111, 0b10111011, 0b00111111,    // 16
    0b01000111, 0b11111011, 0b00111111,    // 17
    0b01001011, 0b11111101, 0b00111111,    // 18
    0b00100111, 0b10001101, 0b01111111,    // 19
    0b10010111, 0b00100101, 0b01001111,    // 20
    0b01001101, 0b01000010, 0b00000111,    // 21
    0b11000101, 0b01000111, 0b00000010,    // 22
    0b00101011, 0b11000000, 0b00000001,    // 23
    0b10000010, 0b11000001, 0b00000001,    // 24
    0b00100111, 0b11000000, 0b00000001,    // 25
    0b11000100, 0b01000011, 0b00000001,    // 26
    0b10010101, 0b00000011, 0b01000001,    // 27
    0b10000101, 0b10001001, 0b01000010,    // 28
    0b11110001, 0b00001000, 0b11000010,    // 29
    0b11110001, 0b00001000, 0b11000010,    // 30
    0b11110100, 0b11011110, 0b11100010,    // 31
    0b11011000, 0b11111100, 0b11111110,    // 32
    0b11111110, 0b11111110, 0b11111100,    // 33
    0b11111110, 0b01111110, 0b11111100,    // 34
    0b01111011, 0b11111110, 0b01111100,    // 35
    0b01111010, 0b01011011, 0b01101100,    // 36
    0b01101000, 0b00111001, 0b01001110,    // 37
    0b00011010, 0b00000111, 0b00001111,    // 38
    0b00101111, 0b00000110, 0b00001111,    // 39
    0b00011111, 0b00100111, 0b00001111,    // 40
    0b00101011, 0b00000111, 0b00001111,    // 41
    0b00000010, 0b00100001, 0b00001111,    // 42
    0b00101011, 0b00110100, 0b00001011,    // 43
    0b00111010, 0b00111101, 0b00000010,    // 44
    0b00000110, 0b00010001, 0b00110000,    // 45
    0b00010101, 0b00000100, 0b00110000,    // 46
    0b00010100, 0b00000000, 0b00110000,    // 47
    0b00000001, 0b00000000, 0b00110000,    // 48
    0b00000001, 0b00000000, 0b00110000,    // 49
    0b00100000, 0b00100000, 0b00010000,    // 50
    0b00010000, 0b00010000, 0b00000000,    // 51
    0b00000000, 0b00010000, 0b00000000,    // 52
};    //

const unsigned char __componentbone1a[] = {

    17 * 3,                                //
    0b00100000, 0b00000000, 0b00000000,    // 0
    0b01000000, 0b01100000, 0b00110000,    // 1
    0b00010000, 0b00110000, 0b01110000,    // 2
    0b00001000, 0b00111000, 0b01110000,    // 3
    0b00100000, 0b00010000, 0b01111000,    // 4
    0b01111000, 0b01011000, 0b00111000,    // 5
    0b01110000, 0b00011000, 0b00111000,    // 6
    0b01101100, 0b00011100, 0b00111000,    // 7
    0b11101000, 0b10111100, 0b01111100,    // 8
    0b01100000, 0b00011100, 0b11111110,    // 9
    0b10001111, 0b01110111, 0b11111110,    // 10
    0b11101100, 0b00001011, 0b11110111,    // 11
    0b00100011, 0b00001101, 0b11110011,    // 12
    0b01011111, 0b11010010, 0b00100001,    // 13
    0b00000101, 0b01100011, 0b00000000,    // 14
    0b00000010, 0b00000001, 0b00000000,    // 15
    0b00000001, 0b00000000, 0b00000000,    // 16
};

const unsigned char __componentbone1b[] = {
    0,
    //     15*3
    //     0b00000001, 0b00000000, 0b00000001, //0
    //     0b00000000, 0b00000001, 0b00000011, //1
    //     0b00000001, 0b00000110, 0b00000011, //2
    //     0b00000000, 0b00001111, 0b00000110, //3
    //     0b00001111, 0b00000101, 0b00001110, //4
    //     0b00000101, 0b00001000, 0b00011110, //5
    //     0b00001011, 0b00110010, 0b00011100, //6
    //     0b01011110, 0b00101100, 0b00110000, //7
    //     0b10111100, 0b00010000, 0b01100000, //8
    //     0b10110000, 0b11011000, 0b01100000, //9
    //     0b00001000, 0b11010000, 0b11100000, //10
    //     0b00110000, 0b11100000, 0b11000000, //11
    //     0b11100000, 0b01000000, 0b10000000, //12
    //     0b01000000, 0b10000000, 0b00000000, //13
    //     0b10000000, 0b00000000, 0b00000000, //14
};

const unsigned char __componentbone1c[] = {
    14 * 3,                                //
    0b10000000, 0b00000000, 0b00000000,    // 0
    0b00000000, 0b00000000, 0b10000000,    // 1
    0b11000000, 0b10000000, 0b11000000,    // 2
    0b01100000, 0b11100000, 0b11000000,    // 3
    0b10100100, 0b01010000, 0b11100000,    // 4
    0b01010000, 0b00101100, 0b11110110,    // 5
    0b11110000, 0b01001110, 0b00111110,    // 6
    0b11011010, 0b00110101, 0b00001110,    // 7
    0b01110101, 0b00000011, 0b00001110,    // 8
    0b00001111, 0b00100001, 0b00011110,    // 9
    0b00010010, 0b00001001, 0b00111110,    // 10
    0b00001100, 0b00000101, 0b00111010,    // 11
    0b00111000, 0b00100100, 0b00011010,    // 12
    0b00011000, 0b00011010, 0b00000000,    // 13
};

const unsigned char __componentbone1d[] = {
    16 * 3,                                //
    0b00000000, 0b00000000, 0b10000000,    // 0
    0b00000000, 0b11000000, 0b10000000,    // 1
    0b00100000, 0b11000000, 0b11000000,    // 2
    0b00000000, 0b01000000, 0b11100000,    // 3
    0b01000000, 0b10100000, 0b01110000,    // 4
    0b11111000, 0b01011000, 0b00110000,    // 5
    0b10110000, 0b01001100, 0b00111000,    // 6
    0b01100100, 0b00101010, 0b00011100,    // 7
    0b00001111, 0b00110100, 0b00001110,    // 8
    0b00011101, 0b00001010, 0b00000111,    // 9
    0b00011110, 0b00000101, 0b00000011,    // 10
    0b00001111, 0b00000010, 0b00000001,    // 11
    0b00000111, 0b00000000, 0b00000001,    // 12
    0b00000010, 0b00000001, 0b00000000,    // 13
    0b00000000, 0b00000001, 0b00000000,    // 14
    0b00000001, 0b00000000, 0b00000000,    // 15
};

const unsigned char __componentbone2a[] = {

    13 * 3,                                //
    0b00000001, 0b00000001, 0b00000000,    // 0
    0b00000000, 0b00000011, 0b00000001,    // 1
    0b00000001, 0b00000000, 0b00000011,    // 2
    0b00000110, 0b00000110, 0b00000011,    // 3
    0b00011100, 0b00001001, 0b00110110,    // 4
    0b01001001, 0b00110010, 0b00111100,    // 5
    0b01111100, 0b01000111, 0b00111000,    // 6
    0b00110001, 0b00001100, 0b01111110,    // 7
    0b00100110, 0b00011100, 0b01111110,    // 8
    0b01011010, 0b01001100, 0b00111110,    // 9
    0b00110110, 0b01111000, 0b00001110,    // 10
    0b01011000, 0b00010000, 0b00101110,    // 11
    0b00011100, 0b00101100, 0b00000000,    // 12
};

const unsigned char __componentbone2b[] = {

    19 * 3,                                //
    0b00000000, 0b00000001, 0b00000000,    // 0
    0b00000001, 0b00000000, 0b00000001,    // 1
    0b00000000, 0b00000011, 0b00000001,    // 2
    0b00000000, 0b00000011, 0b00000011,    // 3
    0b00000111, 0b00000011, 0b00000110,    // 4
    0b00001100, 0b00001101, 0b00000110,    // 5
    0b00011001, 0b00011110, 0b00001100,    // 6
    0b00010111, 0b00001100, 0b00011000,    // 7
    0b00110101, 0b00111110, 0b00011000,    // 8
    0b01001000, 0b00111110, 0b00110000,    // 9
    0b00011000, 0b01111100, 0b00100000,    // 10
    0b00000000, 0b01011000, 0b01100000,    // 11
    0b10111000, 0b01110000, 0b11000000,    // 12
    0b00000000, 0b10110000, 0b11000000,    // 13
    0b00000000, 0b01100000, 0b10000000,    // 14
    0b00100000, 0b11000000, 0b00000000,    // 15
    0b01000000, 0b11000000, 0b00000000,    // 16
    0b01000000, 0b10000000, 0b00000000,    // 17
    0b10000000, 0b00000000, 0b00000000,    // 18
};

const unsigned char __componentbone2c[] = {

    18 * 3,                                //
    0b00000100, 0b00000010, 0b00001100,    // 0
    0b00000010, 0b00001110, 0b00001100,    // 1
    0b00001010, 0b00011110, 0b00001100,    // 2
    0b00000010, 0b00001010, 0b00011100,    // 3
    0b00011100, 0b00001010, 0b00011100,    // 4
    0b00000000, 0b00011000, 0b00011100,    // 5
    0b00001000, 0b00111010, 0b00011100,    // 6
    0b00000111, 0b00111001, 0b00011110,    // 7
    0b00010010, 0b00111100, 0b00111111,    // 8
    0b00100000, 0b00111110, 0b01111111,    // 9
    0b11001110, 0b11110000, 0b01101111,    // 10
    0b11111011, 0b11101101, 0b11000110,    // 11
    0b00011001, 0b10101110, 0b11000110,    // 12
    0b10010100, 0b01100010, 0b10000000,    // 13
    0b10100000, 0b11000000, 0b00000000,    // 14
    0b00000000, 0b11000000, 0b00000000,    // 15
    0b11000000, 0b00000000, 0b00000000,    // 16
    0b10000000, 0b00000000, 0b00000000,    // 17
};

const unsigned char __componentbone2d[] = {

    0,    //

    // 19*3
    // 0b00000001, 0b00000000, 0b00000000, //0
    // 0b00000001, 0b00000001, 0b00000000, //1
    // 0b00000011, 0b00000001, 0b00000001, //2
    // 0b00000000, 0b00000011, 0b00000011, //3
    // 0b00000001, 0b00000011, 0b00000110, //4
    // 0b00000101, 0b00001101, 0b00000110, //5
    // 0b00000010, 0b00001111, 0b00001100, //6
    // 0b00010111, 0b00001010, 0b00011100, //7
    // 0b00000000, 0b00110010, 0b00011100, //8
    // 0b00001100, 0b00111100, 0b00110000, //9
    // 0b01000100, 0b00101000, 0b01110000, //10
    // 0b00110000, 0b11011000, 0b01100000, //11
    // 0b10101000, 0b11110000, 0b11000000, //12
    // 0b01110000, 0b10100000, 0b11000000, //13
    // 0b11000000, 0b01100000, 0b10000000, //14
    // 0b00100000, 0b11000000, 0b00000000, //15
    // 0b01000000, 0b10000000, 0b00000000, //16
    // 0b01000000, 0b10000000, 0b00000000, //17
    // 0b10000000, 0b00000000, 0b00000000, //18
};

// clang-format off

// clang-format on
// unsigned int presentsColour;


void initDataStreams_Skull() {

    static const struct dataStreams streams[] = {

        {_DS_SKULL_COLUBK, _BUF_SKULL_COLUBK},

        {_DS_SKULL_PF1_LEFT, _BUF_SKULL_PF},
        {_DS_SKULL_PF1_RIGHT, _BUF_SKULL_PF + 3 * _BUFFER_SIZE},
        {_DS_SKULL_PF2_LEFT, _BUF_SKULL_PF + 1 * _BUFFER_SIZE},
        {_DS_SKULL_PF2_RIGHT, _BUF_SKULL_PF + 2 * _BUFFER_SIZE},

        {_DS_SKULL_AUDV0, _BUF_AUDV},
        {_DS_SKULL_AUDC0, _BUF_AUDC},
        {_DS_SKULL_AUDF0, _BUF_AUDF},
        {_DS_SKULL_COLUPF, _BUF_SKULL_COLUPF},
        {_DS_SKULL_COLUP0, _BUF_SKULL_COLUP0},

        {_DS_SKULL_GRP0A, _BUF_SKULL_GRP + 0 * _BUFFER_SIZE},
        {_DS_SKULL_GRP1A, _BUF_SKULL_GRP + 1 * _BUFFER_SIZE},
        {_DS_SKULL_GRP0B, _BUF_SKULL_GRP + 2 * _BUFFER_SIZE},
        {_DS_SKULL_GRP1B, _BUF_SKULL_GRP + 3 * _BUFFER_SIZE},
        {_DS_SKULL_GRP0C, _BUF_SKULL_GRP + 4 * _BUFFER_SIZE},
        {_DS_SKULL_GRP1C, _BUF_SKULL_GRP + 5 * _BUFFER_SIZE},

        {DSJMP1PTR, _BUF_SKULL_JUMP},

    };

    initDataStreams(streams, sizeof(streams) / sizeof(struct dataStreams));
}

void initKernel_Skull() {

    setJumpVectors(_BUF_SKULL_JUMP, _skullLoop, _skullExit, _SCANLINES);
    initDataStreams_Skull();


    killRepeatingAudio();

    //    flashTime2 = 0;

    // static int showCave = -1;
    // if (++showCave >= caveCount)
    //     showCave = 0;

    // cave = showCave;
    // level = 4;    // tmp;
    // menuLineTVType = 0;

    colubk = 0;

    // tempName = rangeRandom(sizeof(showCaveName) / sizeof(showCaveName[0]));

    // ARENA_COLOUR = 0;

    // if (rageQuit) {
    //     rageQuit = false;
    //     SAY(__WORD_RAGEQUIT);
    //     FLASH(0x48, 8);
    // }


    //    bufferedSWCHA = 0xFF;
    //    waitRelease = true;
    sound_max_volume = VOLUME_MAX;

    myMemsetInt((unsigned int *)(RAM + _SKULL_BUFFERS_START), 0, _SKULL_BUFFERS_SIZE / 4);


    // draw6Bitmap(_BUF_SKULL_GRP, _BUF_SKULL_COLUP0,    //
    //             gfx_grid_menu_planet_gif, gfx_grid_menu_planet_gif_HEIGHT, 94 + 30, 6);

    // draw6Bitmap(_BUF_SKULL_GRP, _BUF_SKULL_COLUP0,    //
    //             gfx_grid_SKULL_bravado_gif, gfx_grid_SKULL_bravado_gif_HEIGHT, 120 + 30, 6);

    // menuLine = 0;
    // base2 = 0;
    // pushCount = 0;

    //    chooseColourScheme();

    //    mustWatchDelay = MUSTWATCH_SKULL;
}


void drawPFSkull(int buffer, const unsigned char image[66][4][3]) {

    // image data generated by pcc.py

    unsigned char *pf = RAM + buffer;

    int roll = roller;
    for (int col = 0; col < 4; col++) {
        for (int i = 0; i < 66; i++) {
            for (int icc = 0; icc < 3; icc++) {
                pf[col * _BUFFER_SIZE + i * 3 + icc] = image[i][col][roll];
                if (++roll > 2)
                    roll = 0;
            }
        }
    }
}


void initGameState_Skull() {

    // sound_volume = VOLUME_NONPLAYING;
    // loadTrack(20, trackChamp1, CHAMP_VOL + 80, 0x54, 0);
    // loadTrack(10, trackChamp2, CHAMP_VOL, 0x54, 1);

    // myMemsetInt((unsigned int *)(RAM + _BUF_SKULL_GRP), 0, _BUFFER_SIZE * 6 / 4);
    // myMemsetInt((unsigned int *)(RAM + _BUF_SKULL_COLUBK), 0, _BUFFER_SIZE / 4);

    //    presentsColour = 2 << FADE_SHIFT;

    frame = 0;
}


const unsigned char componentskulljawa[] = {

    12 * 3,     0b00000110, 0b10000000, 0b00000100,    // 0
    0b10000111, 0b01000000, 0b00000110,                // 1
    0b10010011, 0b00010100, 0b01000110,                // 2
    0b11111111, 0b00000000, 0b11010110,                // 3
    0b11011101, 0b00000010, 0b11111110,                // 4
    0b11111111, 0b00000000, 0b11111110,                // 5
    0b01111000, 0b10000100, 0b11111110,                // 6
    0b00101000, 0b11011110, 0b11111100,                // 7
    0b00001100, 0b11111000, 0b11111100,                // 8
    0b11001000, 0b11111000, 0b11111100,                // 9
    0b00011000, 0b11001100, 0b11111000,                // 10
    0b10101000, 0b11010000, 0b01111000,                // 11
};

const unsigned char componentskulljawb[] = {

    11 * 3,     0b00000001, 0b00000001, 0b00000000,    // 0
    0b00000001, 0b00000000, 0b00000001,                // 1
    0b00000000, 0b00000001, 0b00000001,                // 2
    0b00000001, 0b00000001, 0b00000001,                // 3
    0b00000010, 0b00000001, 0b00000001,                // 4
    0b00000010, 0b00000001, 0b00000001,                // 5
    0b00000011, 0b00000001, 0b00000001,                // 6
    0b00000011, 0b00000000, 0b00000001,                // 7
    0b00000000, 0b00000000, 0b00000001,                // 8
    0b00000000, 0b00000000, 0b00000001,                // 9
    0b00000001, 0b00000001, 0b00000000,                // 10
};

// clang-format on

void drawPalette(const unsigned char *palette) {

    unsigned char *p = RAM + _BUF_SKULL_COLUPF;

    unsigned char pal[3];

    int baseRoller = roller;
    for (int i = 0; i < 3; i++) {
        pal[i] = convertColour(palette[baseRoller]);
        if (++baseRoller > 2)
            baseRoller = 0;
    }

    for (int i = 0; i < _SCANLINES; i += 3) {
        p[i] = pal[0];
        p[i + 1] = pal[1];
        p[i + 2] = pal[2];
    }
}

void doDrawBitmap(const unsigned char *shape, int x, int y) {

    unsigned char *pf1L = RAM + _BUF_SKULL_PF + y;
    unsigned char *pf2L = pf1L + _SCANLINES;
    unsigned char *pf2R = pf2L + _SCANLINES;
    unsigned char *pf1R = pf2R + _SCANLINES;

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
            *pf1R = (*pf1R & reverseBits[mask.mask2[0]]) | reverseBits[gfx.g[0]];
            *pf2R = (*pf2R & mask.mask2[1]) | gfx.g[1];
            *pf2L = (*pf2L & reverseBits[mask.mask2[2]]) | reverseBits[gfx.g[2]];

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


static int firstTime = 500;

void drawICCSkull() {

#define GAME_RESET_PRESSED (!(SWCHB & 0x01))
#define GAME_SELECT_PRESSED (!(SWCHB & 0x02))


    if (firstTime) {    //|| (GAME_RESET_PRESSED && GAME_SELECT_PRESSED)) {

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

        // if (!rangeRandom(rndm >> 8))
        //     return;

        int bone1y = sinoid[(++sin >> 3) & 15] * 3;

        static const unsigned char *shape[] = {
            __componentbone1c,
            __componentbone1d,
            __componentbone1b,
            __componentbone1a,
        };

        static const unsigned char yoff[] = {93 - 6, 60, 33, 0};
        for (int i = 0; i < 4; i++)
            doDrawBitmap(shape[i], i * 8, 30 + yoff[i] + bone1y);

        static const unsigned char *shape2[] = {
            __componentbone2c,
            __componentbone2d,
            __componentbone2b,
            __componentbone2a,
        };

        static const unsigned char yoff2[] = {0, 33, 69, 108};
        for (int i = 0; i < 4; i++)
            doDrawBitmap(shape2[i], i * 8, 18 + yoff2[i] + 21 - bone1y);

        unsigned char *p = RAM + _BUF_SKULL_PF + 1 * _SCANLINES + 60 + skully;
        unsigned char *q = RAM + _BUF_SKULL_PF + 2 * _SCANLINES + 60 + skully;

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


        int laugh = 0;

        if (!laugh && !rangeRandom(200))
            laugh = rangeRandom(40);
        else
            --laugh;


        int jawy = laugh ? (sinoid[sin & 15] >> 1) * 3 : 0;

        doDrawBitmap(componentskulljawa, 11, skully + 141 - 3 + jawy);
        doDrawBitmap(componentskulljawb, 19, skully + 129 + jawy);

        doDrawBitmap(__componentskulla, 8, skully);
        doDrawBitmap(__componentskullb, 16, skully);

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

        int iy = 73 + eyeY * 3 + skully;

        doDrawBitmap(__skullEye, 8 + eyeX, iy);
        doDrawBitmap(__skullEye, 15 + eyeX, iy);
    }
}


void VB_Skull() {

    initDataStreams_Skull();
    drawICCSkull();

    if (frame > DURATION_SKULL)
        setGameState(GS_GAME);


    //     if (frame < DURATION_COPYRIGHT) {

    // #define CGSPACER 4
    // #define TOP (_SCANLINES / 2 - BAND - 15)
    // #define CGP (gfx_grid_champgames_champ_gif_HEIGHT)
    // #define BAND (CGP + CGSPACER * 2)


    //         unsigned char *l = RAM + _BUF_SKULL_PF;
    //         unsigned char *r = l + _SCANLINES;
    //         unsigned char *c = RAM + _BUF_SKULL_COLUPF;
    //         unsigned char *spc = RAM + _BUF_SKULL_COLUP0;
    //         unsigned char *bk = RAM + _BUF_SKULL_COLUBK;

    //         for (int i = 0; i < _SCANLINES; i++)
    //             c[i] = bk[i] = colubk;

    //         l += TOP;
    //         r += TOP;
    //         c += TOP;

    //         for (int sl = TOP; sl < TOP + 2 * BAND; sl++) {

    //             *l++ = 255;
    //             *r++ = 255;
    //             *spc++ = 0x6;

    //             if (tvSystem == _TV_SYSTEM_NTSC)
    //                 *c++ = convertColour(sl < TOP + BAND ? 0x90 : 0x40);
    //             else
    //                 *c++ = convertColour(sl < TOP + BAND ? 0x92 : 0x42);
    //         }

    //         // for (int sl = TOP + 2 * BAND; sl < _SCANLINES; sl++) {
    //         //     *l++ = *r++ = *c++ = *spc++ = 0;
    //         // }


    //         draw6Bitmap(_BUF_SKULL_GRP, _BUF_SKULL_COLUP0, gfx_grid_champgames_champ_gif,
    //                     gfx_grid_champgames_champ_gif_HEIGHT, TOP + CGSPACER + 1, 8);
    //         draw6Bitmap(_BUF_SKULL_GRP, _BUF_COPYRIGHT_COLUP0, gfx_grid_champgames_games_gif,
    //                     gfx_grid_champgames_games_gif_HEIGHT, TOP + BAND + CGSPACER + 1, 8);


    //         if (frame > 60) {
    //             // if (presentsColour < (PRESENTS_LUM << FADE_SHIFT))
    //             //     presentsColour += FADE_SPEED;

    //             draw6Bitmap(_BUF_SKULL_GRP, _BUF_SKULL_COLUP0, gfx_grid_champgames_presents_gif,
    //                         gfx_grid_champgames_presents_gif_HEIGHT, TOP + 2 * BAND + 10, (15 >> FADE_SHIFT));
    //         }

    //         if ((RAM[_SK_ID] == _WENHOP_SK_ID) && frame > 70) {
    //             int col = convertColour(frame & 16 ? 0x16 : 0x12);

    //             draw6Bitmap(_BUF_SKULL_GRP, _BUF_SKULL_COLUP0, gfx_grid_savekey_gif, gfx_grid_savekey_gif_HEIGHT,
    //                         _SCANLINES - 11 - gfx_grid_savekey_gif_HEIGHT, col);

    //             if (RAM[_SK_RESET]) {
    //                 draw6Bitmap(_BUF_SKULL_GRP, _BUF_SKULL_COLUP0, gfx_grid_savekey_reset_gif,
    //                             gfx_grid_savekey_reset_gif_HEIGHT,
    //                             _SCANLINES - 16 - gfx_grid_savekey_gif_HEIGHT - gfx_grid_savekey_reset_gif_HEIGHT,
    //                             6);

    //                 // if (!(frame & 15))
    //                 //     ADDAUDIO(SFX_SELECTION);
    //             }
    //         }
    //     }

    //     else {

    //         // setGameState(GS_GAME);
    //         setGameState(GS_COUCH_COMPLIANT);
    //     }
}

void clearBuffer(int *buffer, int size) {

    for (int i = 0; i < size; i++)
        *buffer++ = 0;    // getRandom32();
}


void OS_Skull() {

    interleaveChronoColour(&roller);
    setPFColours((unsigned char *)(RAM + _BUF_SKULL_COLUPF));

    clearBuffer((int *)(RAM + _BUF_SKULL_PF), (4 * _SCANLINES / 4));

    //    myMemset((unsigned char *)(RAM + _BUF_SKULL_PF), 0, _SCANLINES);

    // base2++;

    // static enum MENU_OPTION sline = 0;
    // sline++;
    // if (sline >= 2)    //(int)(sizeof(smallWord) / sizeof(smallWord[0])))
    //     sline = 0;

    // int y = sline * 28 + 96;

    // if (flashTime2)
    //     --flashTime2;

    // const char *dLine = 0;

    // switch (sline) {

    // case MENU_OPTION_CAVE:
    //     //        dLine = showCave[cave];

    //     dLine = showCaveName[tempName];

    //     break;

    // case MENU_OPTION_LEVEL:
    //     dLine = Level[level];
    //     break;

    //     // case MENU_OPTION_TVSYSTEM:
    //     //     dLine = TV[menuLineTVType];
    //     //     break;
    // }

    // //    drawSmallString(y, smallWord[sline]);

    // int colour = sline == menuLine ? (flashTime2 & 4) ? 0x0A : ((base2 << 2) & 0xF0) | 0x16 : 0x26;

    // drawString(0, y + 8 + 30, dLine, colour);


    //--------------------------------------------------------------------------
    // Draw ICC menu PF background


    extern const unsigned char gfx_image_WenHop_png[66][4][3];

    // drawPFSkull(_BUF_SKULL_PF, gfx_image_WenHop_png);

    //--------------------------------------------------------------------------

    // draw6Bitmap(_BUF_SKULL_GRP, _BUF_SKULL_COLUP0,    //
    //             gfx_grid_menu_planetx_gif, gfx_grid_menu_planetx_gif_HEIGHT, 62, 0x96);
}

// EOF
