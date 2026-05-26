#include "animations.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "board.h"
#include "colour.h"
#include "decodeCaves.h"
#include "drawPlayer.h"
#include "drawScreen.h"
#include "gameState.h"
#include "kernels.h"
#include "main.h"
#include "random.h"
#include "schedule.h"
#include "scroll.h"
#include "wyrm.h"


void initDataStreams_Game() {

    static const struct dataStreams initStreams[] = {

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

        // {_DS_GAME_AUDV, _BUF_AUDV},
        // {_DS_GAME_AUDC, _BUF_AUDC},
        // {_DS_GAME_AUDF, _BUF_AUDF},

        {DSJMP1PTR, _BUF_GAME_JUMP},
    };

    initDataStreams(initStreams, sizeof(initStreams) / sizeof(struct dataStreams));
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

    initWyrms();    // todo: --> initNextLife


    decodeCave(cave);    // TODO: in initNextLife instead

    setSchedule(SCHEDULE_UNPACK_CAVE);


    // myMemset((unsigned char *)(RAM + _BUF_GAME_GRP0), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_GRP1), 0, _SCANLINES);

    // myMemset((unsigned char *)(RAM + _BUF_GAME_PF0_LEFT), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_PF1_LEFT), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_PF2_LEFT), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_PF0_RIGHT), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_PF1_RIGHT), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_PF2_RIGHT), 0, _SCANLINES);

    myMemset((unsigned char *)(RAM + _BUF_GAME_COLUBK), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_COLUP0), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_COLUP1), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_COLUPF), 0, _SCANLINES);

    // myMemset((unsigned char *)(RAM + _BUF_GAME_AUDV0), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_AUDC0), 0, _SCANLINES);
    // myMemset((unsigned char *)(RAM + _BUF_GAME_AUDF0), 0, _SCANLINES);

    gameSpeed = 6;
    gameFrame = gameSpeed;    // force rollover


    gravity = 1;
    nextGravity = 1;

    lavaSurfaceTrixel = 10000;
    showLava = false;
    showWater = false;


    frame = 0;
}


void setAvailableTime(int time) {

    T1TC = 0;
    T1TCR = 1;

    availableIdleTime = time;
}


void VB_Game() {

    setAvailableTime(50000);    // TODO: find correct available time

    initDataStreams_Game();

    gameFrame++;


    if (frame > 1000)
        // setGameState(GS_RAINBOW);
        setGameState(GS_COUCH_COMPLIANT);    // TODO: GS_MENU has sprite issues

    // static int xspeed = 0;
    // int xOff = 5000;    // rangeRandom(15000) - 7500;

    // if (xspeed < xOff)
    //     xspeed += 2000;
    // if (xspeed > xOff)
    //     xspeed -= 2000;


    // if (!rangeRandom(200)) {
    //     scrollX += 2222400;
    //     scrollY = 2500000;
    // }
    // if (scrollX < 0)
    //     scrollX = 0;


    processCharAnimations();

    if (gameSchedule != SCHEDULE_UNPACK_CAVE) {

        drawScreen();
        drawPlayerSprite();
    }


    scheduledTasks();
}

void OS_Game() {


    setAvailableTime(50000);    // TODO: find available time

    interleaveChronoColour(&roller);
    setPFColours((unsigned char *)(RAM + _BUF_GAME_COLUPF));

    scheduledTasks();
}

// EOF
