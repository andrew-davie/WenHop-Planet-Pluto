#include "defines_dasm.h"

#include "cdfjplus.h"
#include "main.h"

#include "colour.h"
#include "draw.h"
#include "gameState.h"
#include "joystick.h"
#include "rasterBleed.h"


#define DURATION_RASTER_BLEED 250


void initDataStreams_RasterBleed() {

    static const struct dataStreams streams[] = {

        {DSJMP1PTR, _BUF_RASTER_BLEED_JUMP},

        {_DS_CP_GRP0A + 0, _BUF_RASTER_BLEED_GRP + 0 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 1, _BUF_RASTER_BLEED_GRP + 1 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 2, _BUF_RASTER_BLEED_GRP + 2 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 3, _BUF_RASTER_BLEED_GRP + 3 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 4, _BUF_RASTER_BLEED_GRP + 4 * _BUFFER_SIZE},
        {_DS_CP_GRP0A + 5, _BUF_RASTER_BLEED_GRP + 5 * _BUFFER_SIZE},

        {_DS_CP_PF, _BUF_RASTER_BLEED_PF},

        {_DS_CP_COLUPF, _BUF_RASTER_BLEED_COLUPF},
        {_DS_CP_COLUP0, _BUF_RASTER_BLEED_COLUP0},
        {_DS_CP_COLUBK, _BUF_RASTER_BLEED_COLUBK},
    };

    initDataStreams(streams, sizeof(streams) / sizeof(struct dataStreams));
}

void initKernel_RasterBleed() {

    // Note: kernel shared with GS_COPYRIGHT
    // Runs AFTER initGameState_RasterBleed

    setJumpVectors(_BUF_RASTER_BLEED_JUMP, _copyrightLoop, _copyrightExit, _SCANLINES);
    initDataStreams_RasterBleed();
}


void initGameState_RasterBleed() {

    luminance = -15;
    lumTarget = 0;
    frame = 0;

    myMemsetInt((unsigned int *)(RAM + _BUF_RASTER_BLEED_COLUPF), 0, _BUFFER_SIZE);
    myMemsetInt((unsigned int *)(RAM + _BUF_RASTER_BLEED_GRP), 0, 6 * _BUFFER_SIZE / 4);
}


void VB_RasterBleed() {

    initDataStreams_RasterBleed();
    getJoystick();


    static int cycleImage = 0;
    if (!(frame & 127)) {
        while (++cycleImage >= MAX_RASTERBLEED_FRAME)
            cycleImage = 0;
    }


    static int y = (_SCANLINES - 128) >> 1;    //_SCANLINES - 30;
    rasterBleed(SKULL_FRAME, y);


    if (luminance == lumTarget)
        drawNextChar();

    if (gameState == nextGameState && (!(inpt4 & 0x80) || frame > 200))    // || !(RAM[_INPT4] & 0x80))
        setGameState(GS_MENU);
}


void OS_RasterBleed() {

    interleaveChronoColour(&roller);
    adjustLuminance(1);
}

// EOF
