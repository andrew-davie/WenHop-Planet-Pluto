#include "animations.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "animations.h"
#include "board.h"
#include "caveData.h"
#include "colour.h"
#include "decodeCaves.h"
#include "drawPlayer.h"
#include "drawScreen.h"
#include "gameState.h"
#include "joystick.h"
#include "kernels.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "player.h"
#include "random.h"
#include "schedule.h"
#include "score.h"
#include "scroll.h"
#include "sound.h"
#include "swipe.h"
#include "wyrm.h"


extern const unsigned char trackGridLockMelodyIntro[];
extern const unsigned char trackGridLockBase[];

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

        // {_DS_GAME_AUDV, _BUF_AUDV},
        // {_DS_GAME_AUDC, _BUF_AUDC},
        // {_DS_GAME_AUDF, _BUF_AUDF},

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
    loadPalette();

    setSchedule(SCHEDULE_UNPACK_CAVE);

    myMemsetInt((unsigned int *)(RAM + _GAME_BUFFERS_START), 0, _GAME_BUFFERS_SIZE / 4);

    gameSpeed = 6;
    gameFrame = gameSpeed;    // force rollover
    gameTick = 0;


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
    // loadTrack(0, trackGridLockMelodyIntro, 50, 0xC0, 1);
    // loadTrack(10, trackGridLockBase, 100, 0xC0, 0);
}


void VB_Game() {

    T1TC = 0;
    T1TCR = 1;


    // if (!rangeRandom(500)) {
    //     //        FLASH(0x94,4);

    //     loadTrack(0, trackGridLockMelodyIntro, 50, 0xC0, 1);
    //     loadTrack(10, trackGridLockBase, 100, 0xC0, 0);
    // }

    initDataStreams_Game();

    gameFrame++;

#if ENABLE_SHAKE

    // if (!rangeRandom(100)) {
    //     FLASH(0x26, 3);
    //     setShake(20);
    // }

    if (shakeTime) {
        shakeTime--;

        int richter = shakeTime >> 3 << 1;

        if (richter < 2)
            richter = 2;
        if (richter > 3)
            richter = 3;

        shakeX = (rangeRandom(richter + 1) - (richter >> 1)) << 16;
        shakeY = (rangeRandom(richter + 1) - (richter >> 1)) << 16;
    }

    else
        shakeX = shakeY = 0;
#endif

    extern int actualScore;
    actualScore = RAM[_SWCHB];

    //    if (frame > 20000)
    if (RAM[_SWCHB] != 0x3F)
        setGameState(GS_GLOBE);    // GS_COUCH_COMPLIANT);

    processCharAnimations();
    setPalette(_BUF_GAME_COLUBK);

    if (gameSchedule != SCHEDULE_UNPACK_CAVE) {

        drawPlayerSprite();

        drawMace();
        drawRope();
        drawGun();

        drawParticles();
    }

    scheduledTasks();
}

void OS_Game() {

    T1TC = 0;
    T1TCR = 1;

    (*caveList[cave].handler)();

    setScoreCycle(SCORELINE_SCORE);    // tmp

    interleaveChronoColour(&roller);
    adjustLuminance();
    setPFColours((unsigned char *)(RAM + _BUF_GAME_COLUPF));

    updatePlayerAnimation();
    scroll();

    if (gameSchedule != SCHEDULE_UNPACK_CAVE) {

        drawScreen();
        drawScore();
        drawPlayerSprite();
    }

    getJoystick();
    bufferedSWCHA &= swcha;    // | inhibitSWCHA;

    scheduledTasks();
}

// EOF
