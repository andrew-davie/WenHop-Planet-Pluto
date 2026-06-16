#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "board.h"
#include "decodeCaves.h"
#include "main.h"
#include "schedule.h"

enum SCHEDULE gameSchedule;


void setSchedule(enum SCHEDULE nextGameSchedule) {
    gameSchedule = nextGameSchedule;
};


void scheduleUnpackCave() {

    while (availableIdleTime - 10000 > T1TC)    // <-- arbitrary time allowance for slowest cave decode
        if (!decodeExplicitData(false)) {

            if (!totalDogePossible)
                totalDogePossible = -1;    // indicates "perfect" not possible

            setSchedule(SCHEDULE_START_SCAN);
            //          break;
        }
}

void scheduledTasks() {

    static void (*const scheduleFunc[])() = {

        scheduleUnpackCave,     // SCHEDULE_UNPACK_CAVE
        setupBoardScanner,      // SCHEDULE_START_SCAN
        processBoardSquares,    // SCHEDULE_PROCESS_BOARD
    };

    (*scheduleFunc[gameSchedule])();

    if (T1TC > availableIdleTime)
        while (true)
            ;

    // while (T1TC < availableIdleTime)
    //     ;
}


// EOF
