#include "defines_dasm.h"


#include "cdfjplus.h"
#include "main.h"

#include "colour.h"
#include "decodeCaves.h"
#include "drawScreen.h"
#include "schedule.h"


void initKernel_Game() {

    setJumpVectors(_BUF_GAME_JUMP, _gameLoop, _gameExit, _SCANLINES);
    setPointer(DSJMP1PTR, _BUF_GAME_JUMP);
}


void initGameState_Game() {

    setSchedule(SCHEDULE_UNPACK_CAVE);

    initBoard();
    initNewGame();


#if 0    // original initnextlife()



void initNextLife() {


#if __ENABLE_WATER
    //    water = 0;
    //    lava = 0;
    lastWater = 0;
#endif

#if ENABLE_SHAKE
    shakeTime = 0;
    // shakeX = 0;
    // shakeY = 0;
#endif

    bufferedSWCHA = 0xFF;

    caveCompleted = false;
    exitTrigger = false;

    perfectTimer = 80;

    exitMode = 0;
    idleTimer = 0;
    sparkleTimer = 0;
    gameFrame = 0;
    triggerPressCounter = 0;
    triggerOffCounter = 0;
    // expandSpeed = 0;
    nextGravity = 1;

    // dogeBlockCount = 0;
    // cumulativeBlockCount = 0;
    explodeCount = 0;

    resetDelay = 0;
    // selectResetDelay = 0;

    showTool = false;

#if ENABLE_DEBUG
    selectDelay = 0;
#endif

    lavaSurfaceTrixel = 10000; // 0x1C2/3; //22 * PIECE_DEPTH - 1;
    showLava = false;
    showWater = false;

    // lastDisplayMode = DISPLAY_NONE;

#ifdef ENABLE_SWITCH
    switchOn = true;
#endif

    frameCounter = gameSpeed; // force initial
    selectorCounter = 0;

    initWyrms();
    initPlayer();
    initSprites();

    for (int i = 0; i < PARTICLE_COUNT; i++)
        particle[i].age = -1;

#if CIRCLE
    initSwipeCircle(CIRCLE_ZOOM_ZERO + 1);
#endif

    initCharAnimations();

    spacing = 0;

    setScoreCycle(SCORELINE_CAVELEVEL);
}
#endif

    frame = 0;
}


void setAvailableTime(int time) {

    T1TC = 0;
    T1TCR = 1;

    availableIdleTime = 90000;
}


void VB_Game() {

    setAvailableTime(90000);    // TODO: find correct available time


    setPointer(DSJMP1PTR, _BUF_GAME_JUMP);

    setPointer(_DS_GAME_COLUBK, _BUF_GAME_COLUBK);
    setPointer(_DS_GAME_COLUPF, _BUF_GAME_COLUPF);
    setPointer(_DS_GAME_COLUP0, _BUF_GAME_COLUP0);
    setPointer(_DS_GAME_COLUP1, _BUF_GAME_COLUP1);

    setPointer(_DS_GAME_PF0_LEFT, _BUF_GAME_PF0_LEFT);
    setPointer(_DS_GAME_PF1_LEFT, _BUF_GAME_PF1_LEFT);
    setPointer(_DS_GAME_PF2_LEFT, _BUF_GAME_PF2_LEFT);
    setPointer(_DS_GAME_PF0_RIGHT, _BUF_GAME_PF0_RIGHT);
    setPointer(_DS_GAME_PF1_RIGHT, _BUF_GAME_PF1_RIGHT);
    setPointer(_DS_GAME_PF2_RIGHT, _BUF_GAME_PF2_RIGHT);


    static unsigned int col;
    col++;

    unsigned char *p = RAM + _BUF_GAME_COLUBK;
    unsigned char *c = RAM + _BUF_GAME_COLUPF;

    for (int i = 0; i < _SCANLINES; i++) {
        *p++ = 0;    // convertColour((col + (i >> 1)) & 0xFF);

        // *c++ = 0xF;
    }

    setPointer(_DS_GAME_COLUBK, _BUF_GAME_COLUBK);

    // if (frame > 500)
    //     setGameState(GS_RAINBOW);

    scheduledTasks();
}

void OS_Game() {

    setAvailableTime(90000);    // TODO: find available time

    interleaveChronoColour(&roller);
    setPFColours((unsigned char *)(RAM + _BUF_GAME_COLUPF));

    drawScreen();


    scheduledTasks();
}

// EOF
