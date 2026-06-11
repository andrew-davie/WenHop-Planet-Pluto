#include <stdbool.h>

#include "main.h"

#include "bitpatterns.h"
#include "mellon.h"
#include "particle.h"
#include "player.h"
#include "sound.h"
#include "sprites.h"

int autoMoveX;
int autoMoveY;
int autoMoveDeltaX;
int autoMoveDeltaY;
int autoMoveFrameCount;

// Note: to auto-generate the graphical comments for each frame...
// python3 tools/sprite_vis.py main/player.c


// clang-format off

const signed char AnimationDefault[] = {

    0,10,
    1,10,
    2,10,
    3,10,
    4,10,
    5,10,
    6,10,
    7,10,
    8,10,
    9,10,
    10,10,
    11,10,
    12,10,
    13,10,
    14,10,
    15,10,
    16,10,
    17,10,
    18,10,
    19,10,
    20,10,
    21,10,
    22,10,
    23,10,
    24,10,
    25,10,
    26,10,
    27,10,
    28,10,
    29,10,
    30,10,
    31,10,
    32,10,
    33,10,
    34,10,
    35,10,
    36,10,
    37,10,
    38,10,
    39,10,
    40,10,
    41,10,
    42,10,
    43,10,
    44,10,
    45,10,
    46,10,
    47,10,
    48,10,
    49,10,
    50,10,
    51,10,
    52,10,
    53,10,
    54,10,
    55,10,
    56,10,
    57,10,
    58,10,
    59,10,
    60,10,
    61,10,
    62,10,
    63,10,
    64,10,
    65,10,
    66,10,
    67,10,
    68,10,
    69,10,
    70,10,
    71,10,
    72,10,
    73,10,
    74,10,
    75,10,
    76,10,

    ACTION_LOOP,

    FRAME_STAND, 255,
};

const signed char AnimationStandUp[] = {

    FRAME_WALKUP0, 20,
    FRAME_WALK3, 6,
    FRAME_STAND, 255,
};

const signed char AnimationStandLR[] = {
    //    FRAME_WALK3, ,
    FRAME_STAND, 255,
};

const signed char AnimationPush[] = {

    // ACTION_SFX, SFX_PICKAXE,
    ACTION_DOT, 4, 4,
    FRAME_PUSH, 8,
    FRAME_PUSH2, 5,
    FRAME_STAND, 5,
    ACTION_LOOP,
};

const signed char AnimationTapPush[] = {

    // ACTION_SFX, SFX_PICKAXE,
    ACTION_POSITION, 5, 0,
    FRAME_PUSH, 8,
    FRAME_PUSH2, 5,
    FRAME_STAND, 5,
    ACTION_STOP,
};


const signed char AnimationMineUp[] = {

    // ACTION_SFX, SFX_PICKAXE,
 //   ACTION_DOT, 0, 0,
    FRAME_MINE_UP_1, 12,
    FRAME_MINE_UP_0, 8,
    ACTION_LOOP,
};

const signed char AnimationMineDown[] = {

    //    FRAME_MINE_DOWN_0, 2,
    // ACTION_SFX, SFX_PICKAXE,
    ACTION_DOT, 1, 12,
    FRAME_MINE_DOWN_1, 8,
    FRAME_MINE_DOWN_0, 5,
    FRAME_STAND, 5,
    ACTION_LOOP,
};

const signed char AnimationTapUp[] = {

    // ACTION_SFX, SFX_PICKAXE,
    FRAME_MINE_UP_1, 30,
    FRAME_MINE_UP_0, 8,
    ACTION_STOP,
};

const signed char AnimationTapDown[] = {

    //    FRAME_MINE_DOWN_0, 2,
    // ACTION_SFX, SFX_PICKAXE,
    ACTION_POSITION, 0, -10,
    FRAME_MINE_DOWN_1, 30,
    FRAME_MINE_DOWN_0, 5,
    ACTION_STOP,
};

const signed char AnimationLocked[] = {
    FRAME_HUNCH, 12,
    ACTION_STOP,
};

// const signed char AnimationEndPush2[] = {
// //    ACTION_POSITION, 4,0,
// //    FRAME_PUSH2, 15,
// //    ACTION_POSITION, 2,0,
//     FRAME_HUNCH, 5,
//     ACTION_STOP,
// };

// #if ENABLE_SHAKE
// const signed char AnimationShake[] = {
//     ACTION_POSITION, 0,0,
//     FRAME_SHAKE,10,
//     FRAME_SHAKE2,10,
//     FRAME_SHAKE3,10,
//     ACTION_LOOP,
//     ACTION_STOP,
// };
// #endif

// const signed char AnimationWipeHair[] = {
// //    FRAME_WIPE_HAIR, 22,
// //    FRAME_HAIR, 64,
//     FRAME_HAIR2, 4,
//     FRAME_HAIR, 4,
//     FRAME_HAIR2, 4,
//     FRAME_HAIR, 4,
//     ACTION_STOP,
// };

// const signed char AnimationHoldATT_ROCK[] = {
//     FRAME_ARMS_IN_AIR, 10,
//     ACTION_LOOP,
//     FRAME_HOLD_ATT_ROCK_1, 5,
//     ACTION_STOP,
// };

// const signed char AnimationStoop[] = { //=jump
//     ACTION_POSITION, 0,0,
//     FRAME_HUNCH, 1,
//     ACTION_LOOP,
//     ACTION_STOP,
// };

// const signed char AnimationImpatient[] = {
//     FRAME_ARMSCROSSED, 50,
//     FRAME_IMPATIENT, 10,
//     FRAME_IMPATIENT2, 10,
//     FRAME_IMPATIENT, 10,
//     FRAME_IMPATIENT2, 10,
//     FRAME_IMPATIENT, 10,
//     FRAME_IMPATIENT2, 10,
//     // FRAME_IMPATIENT, 10,
//     // FRAME_IMPATIENT2, 10,
//     ACTION_STOP,
// };

// const signed char AnimationLook[] = {
//     ACTION_POSITION, -1,0,
//     FRAME_LOOK1, 5,
//     ACTION_POSITION, -1,0,
//     FRAME_LOOK2, 20,
//     ACTION_POSITION, -1,0,
//     FRAME_LOOK1, 10,
//     FRAME_MOON,10,
//     ACTION_FLIP,
//     FRAME_MOON,10,
//     ACTION_FLIP,
//     FRAME_MOON,10,
//     ACTION_FLIP,
//     FRAME_MOON,10,
//     ACTION_FLIP,
//     FRAME_MOON,10,
//     ACTION_FLIP,
//     FRAME_MOON,10,
//     ACTION_FLIP,
//     FRAME_MOON,10,
//     ACTION_FLIP,
//     FRAME_LOOK1, 10,
//     ACTION_STOP,
// };

// const signed char AnimationBlink[] = {
//     FRAME_BLINK, 6,
//     ACTION_STOP,
// };

const signed char AnimationTurn[] = {
    FRAME_PUSH,6,// _LOOK1, 4,
    FRAME_PUSH2,6,//, 4,
//    ACTION_FLIP,
    ACTION_STOP,
};

// const signed char AnimationShades[] = {
//     FRAME_STAND, 50,
//     FRAME_SHADES_ARM, 20,
//     FRAME_SHADES, 125,
//     FRAME_SHADES_ARM, 25,
//     ACTION_STOP,
// };

const signed char AnimationDie[] = {
    // #if ENABLE_SHAKE
    //     FRAME_SHAKE,6,
    // #endif

    //    FRAME_ARMS_IN_AIR,10,
    // FRAME_HUNCH,100,
    ACTION_STOP,
};

const signed char AnimationWalk[] = {

    FRAME_WALK1, 6,
    FRAME_WALK2, 6,
    FRAME_WALK3, 6,
    FRAME_WALK4, 6,
    ACTION_LOOP,
    //    ACTION_STOP,
};

const signed char AnimationWalkUp[] = {

    FRAME_WALKUP0, 6,
    FRAME_WALKUP1, 6,
    FRAME_WALKUP2, 6,
    FRAME_WALKUP3, 6,
    ACTION_LOOP,
    //    ACTION_STOP,
};

const signed char AnimationWalkDown[] = {

    FRAME_WALKDOWN0, 6,
    FRAME_WALKDOWN1, 6,
    FRAME_WALKDOWN2, 6,
    FRAME_WALKDOWN3, 6,
    ACTION_LOOP,
    //    ACTION_STOP,
};

// const signed char AnimationSnatch[] = {
//     ACTION_POSITION, 5,0,
//     FRAME_PUSH, 10,
//     ACTION_POSITION, 0,0,
// //    FRAME_IMPATIENT, 15,
//     FRAME_WALK4,15,
//     ACTION_STOP,
// };

// const signed char AnimationSnatchDown[] = {
//     ACTION_POSITION, 0,-4,
//     FRAME_SNATCH_DOWN, 10,
//     ACTION_POSITION, 0,0,
//     FRAME_IMPATIENT, 15,
//     ACTION_STOP,
// };

// const signed char AnimationSnatchUp[] = {
//     ACTION_POSITION, 0,4,
//     FRAME_ARMS_IN_AIR, 10,
//     ACTION_POSITION, 0,0,
//     FRAME_IMPATIENT, 15,
//     ACTION_STOP,
// };

// const signed char AnimationSkeleton2[] = {
//     FRAME_SKELETON4, 8,
//     FRAME_SKELETON, 8,
//     ACTION_LOOP,
//     ACTION_STOP,
// };


const signed char AnimationXray[] = {

    FRAME_SKELETON1, 15,
    FRAME_STAND,3,
    FRAME_SKELETON1, 3,
    FRAME_STAND,3,
    FRAME_SKELETON1, 3,
    ACTION_STOP,
};

const signed char AnimationSkeleton[] = {

    FRAME_SKELETON1, 28,
    FRAME_SKELETON2, 8,
    FRAME_SKELETON3, 8,
    FRAME_SKELETON4, 8,
    FRAME_SKELETON5, 8,

    FRAME_BLANK, 255,
    //    ACTION_LOOP,
    //    ACTION_STOP,
};

// const signed char AnimationStartup[] = {
//      FRAME_SKELETON5, 8,
//      FRAME_SKELETON2, 8,
//      FRAME_SKELETON3, 8,
//      FRAME_SKELETON, 15,

// #define DX 4

//     FRAME_STAND,DX,
//     FRAME_SKELETON, DX,
//     FRAME_STAND,DX,
//     FRAME_SKELETON, DX,
//     FRAME_STAND,DX,
//     FRAME_SKELETON, DX,
//     FRAME_STAND,DX,
//     FRAME_SKELETON, DX,

// FRAME_BLANK, 90,
// FRAME_STAND, DX,
// FRAME_BLANK, DX,
// FRAME_STAND, DX,
// FRAME_BLANK, DX,
// FRAME_STAND, DX,
// FRAME_BLANK, DX,
// FRAME_STAND, DX,
// FRAME_BLANK, DX,

// FRAME_STAND, 1,
// ACTION_STOP

// FRAME_STAND,10,

// ACTION_POSITION, -1,0,
// FRAME_LOOK1, 5,
// ACTION_POSITION, -1,0,
// FRAME_LOOK2, 20,
// ACTION_POSITION, -1,0,
// FRAME_LOOK1, 5,
// ACTION_POSITION, 0,0,
//    FRAME_STAND, 255,
//    ACTION_STOP,
// };

// const signed char AnimationArmsCrossed[] = {
//     FRAME_ARMSCROSSED, 80,
//     ACTION_STOP,
// };

// const signed char AnimationTalk[] = {
// #if _ENABLE_ATARIVOX
//     ACTION_SAY, __WORD_DOSOMETHING,
//     FRAME_TALK,10, //FRAMEDELAY_RANDOM,
//     FRAME_STAND,3, //FRAMEDELAY_RANDOM,
//     FRAME_TALK,6, //FRAMEDELAY_RANDOM,
//     FRAME_STAND,4, //FRAMEDELAY_RANDOM,
//     FRAME_TALK,10, //FRAMEDELAY_RANDOM,
// #endif
//     FRAME_STAND,10, //FRAMEDELAY_RANDOM,
//     FRAME_BLINK, 6,
//     ACTION_STOP,
// };

// const signed char AnimationTalk2[] = {
//     FRAME_TALK,10,
//     FRAME_BLINK, 10,
//     ACTION_STOP,
// };

// #if _ENABLE_DRIP

// const signed char AnimationDrip[] = {
//     FRAME_IMPATIENT,25,
//     FRAME_STAND,10,
//     FRAME_HAIR2, 6,
//     FRAME_HAIR, 6,
//     FRAME_HAIR2, 6,
//     FRAME_HAIR, 6,
//     ACTION_STOP,
// };

// const signed char AnimationDrip2[] = {
// //    FRAME_IMPATIENT,25,
// //    FRAME_STAND,10,
//     ACTION_POSITION, -1,0,
//     FRAME_LOOK1, 5,
//     ACTION_POSITION, -1,0,
//     FRAME_LOOK2, 15,
//     ACTION_POSITION, -1,0,
//     FRAME_LOOK1, 5,
// //    ACTION_POSITION, 0,0,
// //    FRAME_ARMSCROSSED, 90,
//     ACTION_STOP,
// };

// #endif // ENABLE_DRIP

// clang-format on

const signed char *const AnimationVector[] = {

    // see (player.h) AnimationIdent

    AnimationDefault,     // 00 ID_Stand
    AnimationStandUp,     // 01 ID_StandUp
    AnimationStandLR,     // 02 ID_StandLR
    AnimationPush,        // 03 ID_Push
    AnimationTurn,        // 04 ID_Turn
    AnimationDie,         // 05 ID_Die
    AnimationWalk,        // 06 ID_Walk
    AnimationSkeleton,    // 07 ID_Skeleton
    AnimationLocked,      // 08 ID_Locked
    AnimationWalkUp,      // 09 ID_WalkUp
    AnimationWalkDown,    // 10 ID_WalkDown
    AnimationMineUp,      // 11 ID_MineUp
    AnimationMineDown,    // 12 ID_MineDown
    AnimationTapUp,       // 13 ID_TapUp
    AnimationTapDown,     // 14 ID_TapDown
    AnimationTapPush,     // 15 ID_TapPush
    AnimationXray,        // 16 ID_Xray
};

enum AnimationIdent playerAnimationID = ID_Stand;
const signed char *playerAnimation = AnimationDefault;
const signed char *playerAnimationLoop = AnimationDefault;
unsigned int playerAnimationCount = 0;

// #define HAIR 0x28
// #define SKIN 0x46
// #define TOP1 0x58
// #define TOP2 0x54
// #define BOOT 0x24
// #define PANT 0x06
// #define BELT 0x98
// #define SOLE 0x08
// #define BONE 0x08

// #define HAIR 0
// #define SKIN 1
// #define TOP1 2
// #define TOP2 3
// #define BOOT 4
// #define PANT 5
// #define BELT 6
// #define SOLE 7


const unsigned char redirect[] = {0, 1, 1, 2};


void processAnimationCommand() {

    while (!playerAnimationCount)
        switch (*playerAnimation) {

        case ACTION_SFX:
            ADDAUDIO(*++playerAnimation);
            playerAnimation++;
            break;

        case ACTION_FLIP:
            faceDirection = -faceDirection;
            playerAnimation++;
            break;

        case ACTION_LOOP:
            if (playerDead)
                startPlayerAnimation(ID_Skeleton);

            // else if (exitMode)
            //     startPlayerAnimation(ID_Shades);

            else {
                if (playerAnimationLoop)
                    playerAnimation = playerAnimationLoop;
                else
                    playerAnimation++;
            }
            break;

        case ACTION_STOP:
            startPlayerAnimation(playerDead ? ID_Skeleton : ID_Stand);
            break;

        case ACTION_POSITION: {
            frameAdjustX = *++playerAnimation;
            frameAdjustY = *++playerAnimation;
            playerAnimation++;
            break;
        }

        case ACTION_DOT: {

            int dotX = 2 + (*++playerAnimation) * faceDirection;
            int dotY = *++playerAnimation;
            nDots(6, playerX, playerY, PT_TWO, 20, dotX, dotY, 100, 7);
            playerAnimation++;
            break;
        }

        case ACTION_SAY:
#if _ENABLE_ATARIVOX
            sayWord(*++playerAnimation);
#endif
            playerAnimation++;
            break;

        default:
            playerAnimationCount = *(playerAnimation + 1);
            break;
        }


    doges = *(playerAnimation);
}

const unsigned short reciprocal[] = {

#if SPEED_BASE == 2
    0x10000 / 3,
#endif

#if SPEED_BASE == 3
    0x10000 / 4,
#endif

#if SPEED_BASE == 4
    0x10000 / 5,
#endif

#if SPEED_BASE == 5
    0x10000 / 6,
#endif

#if SCHEDULER == 1
    0x10000 / 3,
#endif

    //    0x10000/5,
    //    0x10000/6,
    //    0x10000/7,
    0x10000 / SPEED_BASE,
// 0x10000/9,
// 0x10000/10,
// 0x10000/11,
// 0x10000/12,
// 0x10000/13,

#if SPEED_BASE == 6
    0x10000 / 11,
#endif
};

void updatePlayerAnimation() {

    if (autoMoveFrameCount)
        autoMoveFrameCount--;

    autoMoveX = ((autoMoveFrameCount * autoMoveDeltaX * (int)reciprocal[0]) >> 16);
    autoMoveY = ((autoMoveFrameCount * autoMoveDeltaY * (int)reciprocal[0]) >> 16);

    if (playerAnimationCount != 255) {

        if (!playerAnimationCount) {
            playerAnimation += 2;
            processAnimationCommand();
        }

        playerAnimationCount--;
    }
}

void startPlayerAnimation(enum AnimationIdent animID) {

    playerAnimationID = animID;

    playerAnimation = playerAnimationLoop = AnimationVector[animID];

    playerAnimationCount = 0;
    autoMoveDeltaX = 0;
    autoMoveDeltaY = 0;
    frameAdjustX = 0;
    frameAdjustY = 0;

    processAnimationCommand();
}


// EOF