#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "decodeCaves.h"
#include "main.h"
#include "schedule.h"

enum SCHEDULE gameSchedule;


void setSchedule(enum SCHEDULE nextGameSchedule) {
    gameSchedule = nextGameSchedule;
};


void scheduleUnpackCave() {

    while (T1TC < 20000)    // <-- arbitrary time allowance for slowest cave decode

        if (!decodeExplicitData(false)) {

            if (!totalDogePossible)
                totalDogePossible = -1;    // indicates "perfect" not possible

            setSchedule(SCHEDULE_START);
            break;
        }
}

void scheduledTasks() {

    static void (*const scheduleFunc[])() = {

        scheduleUnpackCave,     // SCHEDULE_UNPACK_CAVE
        setupBoardScanner,      // SCHEDULE_START
        processBoardSquares,    // SCHEDULE_PROCESSBOARD
    };

    (*scheduleFunc[gameSchedule])();
}


// EOF
