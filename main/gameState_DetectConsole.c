#include "defines_dasm.h"
#include <limits.h>

#include "cdfjplus.h"

#include "gameState.h"
#include "main.h"


#define DETECT_FRAME_COUNT 10


int detectionFrame;


// clang-format off

const struct fmt {

    int frequency;
    unsigned char format;

} mapTimeToFormat[] = {

    // Measured with Gopher over DETECT_FRAME_COUNT frames
    // 70MHz ARM

    { 0xB26349, _TV_SYSTEM_NTSC,  },
    { 0xB33EF7, _TV_SYSTEM_SECAM, },
    { 0xB384BA, _TV_SYSTEM_PAL60, },
};

// clang-format on

void initKernel_DetectConsole() {
}


void initGameState_DetectConsole() {

    detectionFrame = 0;
}


void VB_DetectConsole() {


    switch (detectionFrame) {

    case 0:

        tvSystem = _TV_SYSTEM_NTSC;    // force NTSC frame

        T1TC = 0;
        T1TCR = 1;
        break;

    case DETECT_FRAME_COUNT: {

        unsigned int detectedPeriod = T1TC;

        int delta = INT_MAX;
        for (unsigned int i = 0; i < sizeof(mapTimeToFormat) / sizeof(struct fmt); i++) {

            int dist = detectedPeriod - mapTimeToFormat[i].frequency;
            if (dist < 0)
                dist = -dist;

            if (dist < delta) {
                delta = dist;
                tvSystem = mapTimeToFormat[i].format;
            }
        }

        //        setGameState(GS_COPYRIGHT);
        setGameState(GS_MENU);
    }
    }

    detectionFrame++;
}

void OS_DetectConsole() {
}

// EOF
