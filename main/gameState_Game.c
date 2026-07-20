#include "animations.h"
#include "defines_dasm.h"

#include "gameState.h"

#include "cdfjplus.h"

#include "animations.h"
#include "board.h"
#include "caveData.h"
#include "colour.h"
#include "decodeCaves.h"
#include "draw.h"
#include "drawPlayer.h"
#include "drawScreen.h"
#include "gameState.h"
#include "joystick.h"
#include "kernels.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "random.h"
#include "schedule.h"
#include "scroll.h"
#include "sound.h"
#include "swipe.h"
#include "wyrm.h"

int attachment = 0;
const OFFSET *attachmentOffset = 0;

void initDataStreams_Game() {

    static const struct dataStreams streams[] = {

        {_DS_GAME_PF0_LEFT, _BUF_GAME_PF0_LEFT},
        {_DS_GAME_PF1_LEFT, _BUF_GAME_PF1_LEFT},
        {_DS_GAME_PF2_LEFT, _BUF_GAME_PF2_LEFT},
        {_DS_GAME_PF0_RIGHT, _BUF_GAME_PF0_RIGHT},
        {_DS_GAME_PF1_RIGHT, _BUF_GAME_PF1_RIGHT},
        {_DS_GAME_PF2_RIGHT, _BUF_GAME_PF2_RIGHT},

        {_DS_GAME_COLUPF, _BUF_GAME_COLUPF},
        {_DS_GAME_COLUBK, _BUF_GAME_COLUBK},
        {_DS_GAME_COLUP0, _BUF_GAME_COLUP0},
        {_DS_GAME_COLUP1, _BUF_GAME_COLUP1},
        {_DS_GAME_GRP0A, _BUF_GAME_GRP0},
        {_DS_GAME_GRP1A, _BUF_GAME_GRP1},

        {DSJMP1PTR, _BUF_GAME_JUMP},
    };

    initDataStreams(streams, sizeof(streams) / sizeof(struct dataStreams));
}


void initKernel_Game() {

    setJumpVectors(_BUF_GAME_JUMP, _gameLoop, _gameExit, _SCANLINES);
    initDataStreams_Game();
}


void initGameState_Game() {


    initBoard();
    initNewGame();
    initCharVector();
    initCharAnimations();

    initSprites();
    initParticles();
    initTool();

    initWyrms();     // todo: --> initNextLife
    initPlayer();    // --> initNextLife

#if ENABLE_SWIPE
    setSwipeType(SWIPE_STAR);
    initStarSwipe();    // clears to fully hidden and holds idle -- doesn't start growing yet,
                        // since playerX/Y aren't placed until the cave decode runs. decodeCaves.c
                        // calls setSwipe() for real, centred on the player, once that's known.
#endif

    exitMode = 0;    // --> initNextlife

    decodeCave(cave);    // TODO: in initNextLife instead

    luminance = -15;
    lumTarget = 0;
    loadPalette();

    setSchedule(SCHEDULE_UNPACK_CAVE);

    myMemsetInt((unsigned int *)(RAM + _GAME_BUFFERS_START), 0, _GAME_BUFFERS_SIZE / 4);

    gameSpeed = SPEED_BASE;
    gameFrame = gameSpeed;    // force rollover

#if ENABLE_SHAKE
    setShake(0);
    shakeX = 0;
    shakeY = 0;
#endif


    gravity = 1;
    nextGravity = gravity;

    lavaSurfaceTrixel = 10000;
    showLava = false;
    showWater = false;

    frame = 0;


    sound_volume = VOLUME_PLAYING;
}


void VB_Game() {

    T1TC = 0;
    T1TCR = 1;

    updatePlayerAnimation();
    scroll();

#if ENABLE_SWIPE
    // Don't advance the swipe while the cave is still being decoded --
    // setSwipe() itself isn't called until scheduleUnpackCave() (schedule.c)
    // confirms decode is fully done, so swipe() would just be idling/holding
    // black anyway during SCHEDULE_UNPACK_CAVE. But it's not free to call:
    // it still burns some of this frame's time budget (the finish-clear
    // state machine, etc.), which is time taken away from
    // scheduleUnpackCave()'s own per-frame decode slice (see schedule.c's
    // "while (T1TC < availableIdleTime - 20000)"). Skipping it here lets
    // cave decode use the full frame budget instead of sharing it with an
    // idle swipe. applySwipeMask() below still runs unconditionally every
    // frame -- that's what forces the screen black while drawScreen() itself
    // is also skipped during unpack (see OS_Game()), regardless of whatever
    // stale buffer contents are sitting there.
    if (gameSchedule != SCHEDULE_UNPACK_CAVE)
        swipe(50000);    // Bumped from 35000, confirmed on hardware -- now safe to hold back more of the
                         // frame for other VB_Game systems without any visible cost, because circle()'s
                         // border is a real double buffer now (see swipe.c's borderShowA/B): a lap that
                         // takes more frames to get through (the direct result of giving swipe() a
                         // smaller slice here) just holds last lap's finished ring steady for longer
                         // instead of showing a half-drawn one. Before that fix, raising this value
                         // would have made the old bottom-of-circle flashing worse, not better.

#endif

    initDataStreams_Game();

    gameFrame++;


#if ENABLE_SHAKE

    if (shakeTime) {
        shakeTime--;
        shakeX = (rangeRandom(3) - 1) << 16;
        shakeY = (rangeRandom(5) - 2) << 16;
    }

    else
        shakeX = shakeY = 0;
#endif

    if (RAM[_SWCHB] != 0x3F)
        setGameState(GS_MENU);

    processCharAnimations();
    setPalette(_BUF_GAME_COLUBK);

    if (gameSchedule != SCHEDULE_UNPACK_CAVE) {

        if (!exitMode || autoMoveX || autoMoveY)
            drawPlayerSprite();

        if (!maskNeeded) {
            drawMace();
            drawRope();
            drawGun();
            drawParticles();

            drawAttachedChar(attachment);
        }
    }

    interleaveChronoColour(&roller);
    adjustLuminance(0);

#if ENABLE_SWIPE
    applySwipeMask(_BUF_GAME_PF0_LEFT);    // must happen after everything else has drawn
#endif

    scheduledTasks();    // gets the MOST time
}

void OS_Game() {

    T1TC = 0;
    T1TCR = 1;

    (*caveList[cave].handler)();

    if (gameSchedule != SCHEDULE_UNPACK_CAVE)
        drawScreen();

    getJoystick();
    bufferedSWCHA &= swcha;    // | inhibitSWCHA;

    setPFColours((unsigned char *)(RAM + _BUF_GAME_COLUPF));
    scheduledTasks();    // gets the LEAST time because of drawScreen (~78K already used)
}

// EOF
