#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "board.h"
#include "decodeCaves.h"
#include "main.h"
#include "mellon.h"
#include "schedule.h"
#include "scroll.h"
#include "swipe.h"


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

#if ENABLE_SWIPE
            // Decode is genuinely finished now (not just "this object's
            // done, more to come" -- decodeExplicitData() returned false,
            // meaning the whole cave, including CH_MELLON_HUSK_BIRTH's
            // resetTracking() call, has run). playerX/Y and scrollX/scrollY
            // are all at their final settled values, and VB_Game() is about
            // to stop skipping the sprite/board draw once gameSchedule
            // leaves SCHEDULE_UNPACK_CAVE. This is the right, and only
            // correct, moment to start the iris-in for real -- centred on
            // the player's actual on-screen position, same world-to-screen
            // math used to draw the player (see drawPlayer.c), minus the
            // sprite-specific animation/shake offsets we don't want here.
            // playerX/Y are the character CELL (top-left corner, in trix),
            // not the middle of it -- a character is CHAR_TRIX_X x
            // CHAR_TRIX_Y trix, so +CHAR_CENTER_X gets to the true
            // horizontal centre (same offset particle.c's baseX/baseY use
            // to centre effects on the player).
            //
            // Y deliberately does NOT add +CHAR_CENTER_Y, unlike X -- this
            // was the actual off-centre bug (star landing well below the
            // player). resetTracking() (scroll.c) sets scrollX to put the
            // player's cell MIDDLE at screen centre (it adds its own
            // "+ (CHAR_TRIX_X << 15)" half-cell term), but sets scrollY to
            // put the player's cell TOP at screen centre (no analogous term
            // -- see the commented-out attempt right there, which used the
            // wrong shift amount and was abandoned rather than fixed). So
            // resetTracking() itself treats X and Y differently; matching
            // that (not "correcting" it here) is what actually lines the
            // star up with where the player ends up on screen. Ongoing
            // gameplay scrolling (scroll()'s tX/hX/tY/hY) isn't affected
            // either way -- it has its own separate "+ (CHAR_TRIX_Y << 15)"
            // compensation for this same asymmetry (see the "@navel"
            // comment in scroll.c), which is why the CIRCLE shrink on death
            // (board.c, driven by that ongoing scroll position) has never
            // been seen off-centre -- only this one-time star GROW is
            // exposed to resetTracking()'s own Y asymmetry directly.
            // Not touching resetTracking()/scroll() to fix this instead:
            // they're the live gameplay camera, tuned and working -- safer
            // to make the swipe's one-time centring match what they
            // actually do than to risk destabilising them to match an
            // assumption the swipe made.
            randomizeStarAngle();         // fresh look each time -- see randomizeStarAngle()'s comment
            markCircleFreshSequence();    // fresh sequence -- see markCircleFreshSequence()'s comment
            // Cruise step bumped from 768 (3 trix/lap) to 1024 (4
            // trix/lap) -- swipe.c's ease-in (see STAR_GROW_RAMP_LAPS)
            // now slows the first few laps down, so the overall grow
            // is faster to make up for it once past the ease-in.
            setSwipe(playerX * CHAR_TRIX_X + CHAR_CENTER_X - (scrollX >> 16),
                     playerY * CHAR_TRIX_Y - (scrollY >> 16), 0, 1024, SWIPE_GROW);
#endif
            break;
        }
}

void scheduledTasks() {

    static void (*const scheduleFunc[])() = {

        scheduleUnpackCave,     // SCHEDULE_UNPACK_CAVE
        setupBoardScanner,      // SCHEDULE_START_SCAN
        processBoardSquares,    // SCHEDULE_PROCESS_BOARD
    };

    (*scheduleFunc[gameSchedule])();
}


// EOF
