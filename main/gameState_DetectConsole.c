#include "defines_dasm.h"
#include <limits.h>

#include "cdfjplus.h"

#include "gameState.h"
#include "main.h"
#include "score.h"

#define DETECT_FRAME_COUNT 10


int detectionFrame;


const struct fmt {

    int armFrequency;
    int count;
    unsigned char format;

} mapTimeToFormat[] = {

    // clang-format off
    // Measured with Gopher over DETECT_FRAME_COUNT frames

    // 70MHz ARM
    { 70000000, 0xB26349, _TV_SYSTEM_NTSC,  },
    { 70000000, 0xB33EF7, _TV_SYSTEM_SECAM, },
    { 70000000, 0xB384BA, _TV_SYSTEM_PAL60, },

    // 60MHz ARM
    { 60000000, 0x98E311, _TV_SYSTEM_NTSC,  },
    { 60000000, 0x99B301, _TV_SYSTEM_SECAM, },
    { 60000000, 0x9A0260, _TV_SYSTEM_PAL60, },

    // clang-format on
};


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

            int dist = detectedPeriod - mapTimeToFormat[i].count;
            if (dist < 0)
                dist = -dist;

            if (dist < delta) {
                delta = dist;
                tvSystem = mapTimeToFormat[i].format;
                armFrequency = mapTimeToFormat[i].armFrequency;
            }
        }


        // Calculate ARM cycles per TIA clock tick.
        // e.g., 70MHz --> 70000000 cycles/sec
        // /60 = 1,166,666 cycles/frame
        // /262 = 4,452 cycles/scanline
        // /76 = 58 cycles/TIA clock
        // *64 = ~3712 cycles per INTIM tick

        // Following code does the above, avoiding overflow

        armCycles = ((armFrequency >> 8) * (0x10000 / 60)) >> 8;    // cycles/frame
        armCycles = (armCycles * (0x10000 / 262)) >> 16;            // cycles/scanline
        armCycles = (armCycles * (64 * 0x10000 / 76)) >> 16;        // cycles/INTIM

        setGameState(GS_GLOBE);
        // setGameState(GS_MENU);
        // setGameState(GS_SKULL);
    }
    }

    detectionFrame++;
}

void OS_DetectConsole() {
}

// EOF
