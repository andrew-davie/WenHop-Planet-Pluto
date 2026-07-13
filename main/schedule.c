#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "board.h"
#include "colour.h"
#include "decodeCaves.h"
#include "main.h"
#include "schedule.h"
#include "score.h"


enum SCHEDULE gameSchedule;


void setSchedule(enum SCHEDULE nextGameSchedule) {
    gameSchedule = nextGameSchedule;
};


void scheduleUnpackCave() {

    while (T1TC < availableIdleTime - 20000)    // <-- arbitrary time allowance for slowest cave decode
        if (!decodeExplicitData()) {

            if (!totalDogePossible)
                totalDogePossible = -1;    // indicates "perfect" not possible

            setSchedule(SCHEDULE_START_SCAN);
            break;
        }
}

void scheduledTasks() {


    int st = T1TC;

    // while (gameSchedule < 0 || gameSchedule > SCHEDULE_PROCESS_BOARD)
    //     ;


#if 0

    static void (*const scheduleFunc[])() = {

        scheduleUnpackCave,     // SCHEDULE_UNPACK_CAVE
        setupBoardScanner,      // SCHEDULE_START_SCAN
        processBoardSquares,    // SCHEDULE_PROCESS_BOARD
    };

    (*scheduleFunc[gameSchedule])();

    if (gameSchedule == SCHEDULE_START_SCAN)
        if (T1TC - st > actualScore)
            actualScore = T1TC - st;

#else


    if (gameSchedule == SCHEDULE_UNPACK_CAVE)
        scheduleUnpackCave();

    else if (gameSchedule == SCHEDULE_START_SCAN) {
        setupBoardScanner();
        if (T1TC - st > actualScore)
            actualScore = T1TC - st;
    }

    else if (gameSchedule == SCHEDULE_PROCESS_BOARD)
        processBoardSquares();


#endif


    if (T1TC > availableIdleTime)
        FLASH(0x94, 4);
}


// EOF
