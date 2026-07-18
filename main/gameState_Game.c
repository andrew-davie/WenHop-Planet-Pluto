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
    initStarSwipe();
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

    initDataStreams_Game();

    gameFrame++;


    updatePlayerAnimation();
    scroll();


#if ENABLE_SHAKE

    if (shakeTime) {
        shakeTime--;
        shakeX = (rangeRandom(5) - 2) << 16;
        shakeY = (rangeRandom(5) - 2) << 16;
    }

    else
        shakeX = shakeY = 0;
#endif

    if (RAM[_SWCHB] != 0x3F)
        setGameState(GS_MENU);    // SKULL);    // GS_COUCH_COMPLIANT);

    processCharAnimations();
    setPalette(_BUF_GAME_COLUBK);

    if (gameSchedule != SCHEDULE_UNPACK_CAVE) {

        drawPlayerSprite();

        drawMace();
        drawRope();
        drawGun();
        drawParticles();

        drawAttachedChar(attachment);
    }

    interleaveChronoColour(&roller);
    adjustLuminance(1);

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
