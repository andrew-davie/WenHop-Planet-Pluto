#include "animations.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "animations.h"
#include "board.h"
#include "colour.h"
#include "decodeCaves.h"
#include "drawPlayer.h"
#include "drawScreen.h"
#include "gameState.h"
#include "kernels.h"
#include "main.h"
#include "player.h"
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

    myMemsetInt((unsigned int *)(RAM + _GAME_BUFFERS_START), 0, _GAME_BUFFERS_SIZE / 4);

    gameSpeed = 6;            // tmp 6;
    gameFrame = gameSpeed;    // force rollover


    gravity = 1;
    nextGravity = 1;

    lavaSurfaceTrixel = 10000;
    showLava = false;
    showWater = false;

    frame = 0;
}


void VB_Game() {

    T1TC = 0;
    T1TCR = 1;

    initDataStreams_Game();

    gameFrame++;


    if (frame > 1000)
        setGameState(GS_COUCH_COMPLIANT);

    processCharAnimations();

    if (gameSchedule != SCHEDULE_UNPACK_CAVE) {

        drawScreen();
        drawPlayerSprite();
    }


    for (int i = 0; i < _SCANLINES; i++) {

        RAM[_BUF_GAME_COLUBK + i] = colubk;
    }


    scheduledTasks();
}

void OS_Game() {

    T1TC = 0;
    T1TCR = 1;

    interleaveChronoColour(&roller);
    setPFColours((unsigned char *)(RAM + _BUF_GAME_COLUPF));

    updatePlayerAnimation();
    scroll();

    scheduledTasks();
}

// EOF
