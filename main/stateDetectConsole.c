#include <limits.h>

#include "cdfjplus.h"
#include "defines_dasm.h"

#include "main.h"
#include "state.h"


#define DETECT_FRAME_COUNT 10


static int detectionFrame;


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

// ---------------------------------------
// Console detection displays black screen

void initialise_GS_DetectConsole() {

    detectionFrame = 0;
}

void VB_GS_DetectConsole() {


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

        setNextGameState(GS_COPYRIGHT);
    }
    }

    detectionFrame++;
}

void OS_GS_DetectConsole() {
}

// EOF
