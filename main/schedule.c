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


#if 1

    // int st = T1TC;
    // enum SCHEDULE sch = gameSchedule;    // capture before the call: setupBoardScanner() mutates
    // gameSchedule to SCHEDULE_PROCESS_BOARD on its real-work
    // path, so checking gameSchedule after the call missed it

    enum SCHEDULE ogs = gameSchedule;
    int st = T1TC;

    static void (*const scheduleFunc[])() = {

        scheduleUnpackCave,     // SCHEDULE_UNPACK_CAVE
        setupBoardScanner,      // SCHEDULE_START_SCAN
        processBoardSquares,    // SCHEDULE_PROCESS_BOARD
    };

    (*scheduleFunc[gameSchedule])();

    // if (ogs == SCHEDULE_START_SCAN)
    //     if (T1TC - st > actualScore)
    //         actualScore = T1TC - st;

#else

    int st = T1TC;

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


    // if (T1TC > availableIdleTime && ogs == 1)
    //     actualScore = T1TC;
    //        FLASH(0x94, 4);
}


// EOF
