#include <stdbool.h>

#include "main.h"

#include "animations.h"
#include "attribute.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "sound.h"
#include "sprites.h"

int autoMoveX;
int autoMoveY;
int autoMoveDeltaX;
int autoMoveDeltaY;
int autoMoveFrameCount;

// clang-format off

const signed char AnimationDefault[] = {

    FRAME_NEW_X004_Y004, 255,
};

const signed char AnimationStandUp[] = {

    FRAME_NEW_X004_Y034, 20,
    FRAME_WALK3, 6,
    FRAME_NEW_X004_Y004, 255,
};

const signed char AnimationStandArmsUp[] = {

    FRAME_ARMS_IN_AIR, 255,
};


const signed char AnimationStandLR[] = {
    //    FRAME_WALK3, ,
    FRAME_NEW_X004_Y004, 255,
};

const signed char AnimationMine[] = {

    ACTION_SFX, SFX_PICKAXE,
    ACTION_DOT, 4, 4,
    FRAME_NEW_X017_Y124, 8,
    FRAME_NEW_X000_Y124, 5,
    FRAME_NEW_X004_Y004, 5,
    ACTION_LOOP,
};

const signed char AnimationTapPush[] = {

    // ACTION_SFX, SFX_PICKAXE,
    ACTION_POSITION, 5, 0,
    FRAME_NEW_X017_Y124, 8,
    FRAME_NEW_X000_Y124, 5,
    FRAME_NEW_X004_Y004, 5,
    ACTION_STOP,
};


const signed char AnimationMineUp[] = {

    ACTION_SFX, SFX_PICKAXE,
    // ACTION_SFX, SFX_PICKAXE,
 //   ACTION_DOT, 0, 0,
    FRAME_MINE_UP_1, 12,
    FRAME_MINE_UP_0, 8,
    ACTION_LOOP,
};

const signed char AnimationMineDown[] = {

    ACTION_SFX, SFX_PICKAXE,
    //    FRAME_MINE_DOWN_0, 2,
    // ACTION_SFX, SFX_PICKAXE,
    ACTION_DOT, 1, 12,
    FRAME_MINE_DOWN_1, 8,
    FRAME_MINE_DOWN_0, 5,
    FRAME_NEW_X004_Y004, 5,
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
    FRAME_NEW_X020_Y274, 12,
    ACTION_STOP,
};

const signed char AnimationPickup[] = {
    FRAME_PICKUP, 6,
    FRAME_PICKUP2, 6,
    FRAME_NEW_X004_Y004, 10,
    ACTION_STOP,
};


// const signed char AnimationEndPush2[] = {
// //    ACTION_POSITION, 4,0,
// //    FRAME_NEW_X000_Y124, 15,
// //    ACTION_POSITION, 2,0,
//     FRAME_NEW_X020_Y274, 5,
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
//     FRAME_NEW_X020_Y274, 1,
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
    FRAME_NEW_X017_Y124,6,// _LOOK1, 4,
    FRAME_NEW_X000_Y124,6,//, 4,
//    ACTION_FLIP,
    ACTION_STOP,
};

// const signed char AnimationShades[] = {
//     FRAME_NEW_X004_Y004, 50,
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
    // FRAME_NEW_X020_Y274,100,
    ACTION_STOP,
};

const signed char AnimationWalk[] = {

    FRAME_NEW_X004_Y064, 6,
    FRAME_WALK2, 6,
    FRAME_WALK3, 6,
    FRAME_WALK4, 6,
    ACTION_LOOP,
    //    ACTION_STOP,
};

const signed char AnimationWalkUp[] = {

    FRAME_NEW_X004_Y034, 6,
    FRAME_NEW_X020_Y033, 6,
    FRAME_WALKUP2, 6,
    FRAME_NEW_X052_Y033, 6,
    ACTION_LOOP,
    //    ACTION_STOP,
};

// Exit sequence only (board.c's exitMode case) -- same 4 frames as AnimationWalkUp, held longer
// each so the leg cycle matches the slow walk-off-into-the-distance drift
// (updatePlayerAnimation()) instead of looking like they're jogging in place. AnimationWalkUp
// itself is untouched, still used for ordinary up-facing movement everywhere else.
const signed char AnimationWalkUpSlow[] = {

    FRAME_NEW_X004_Y034, 10,
    FRAME_NEW_X020_Y033, 10,
    FRAME_WALKUP2, 10,
    FRAME_NEW_X052_Y033, 10,
    ACTION_LOOP,
};

const signed char AnimationWalkDown[] = {

    FRAME_NEW_X004_Y094, 6,
    FRAME_NEW_X020_Y093, 6,
    FRAME_WALKDOWN2, 6,
    FRAME_NEW_X052_Y093, 6,
    ACTION_LOOP,
    //    ACTION_STOP,
};

// const signed char AnimationSnatch[] = {
//     ACTION_POSITION, 5,0,
//     FRAME_NEW_X017_Y124, 10,
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


// const signed char AnimationStartup[] = {
//      FRAME_SKELETON5, 8,
//      FRAME_SKELETON2, 8,
//      FRAME_SKELETON3, 8,
//      FRAME_SKELETON, 15,

// #define DX 4

//     FRAME_NEW_X004_Y004,DX,
//     FRAME_SKELETON, DX,
//     FRAME_NEW_X004_Y004,DX,
//     FRAME_SKELETON, DX,
//     FRAME_NEW_X004_Y004,DX,
//     FRAME_SKELETON, DX,
//     FRAME_NEW_X004_Y004,DX,
//     FRAME_SKELETON, DX,

// FRAME_BLANK, 90,
// FRAME_NEW_X004_Y004, DX,
// FRAME_BLANK, DX,
// FRAME_NEW_X004_Y004, DX,
// FRAME_BLANK, DX,
// FRAME_NEW_X004_Y004, DX,
// FRAME_BLANK, DX,
// FRAME_NEW_X004_Y004, DX,
// FRAME_BLANK, DX,

// FRAME_NEW_X004_Y004, 1,
// ACTION_STOP

// FRAME_NEW_X004_Y004,10,

// ACTION_POSITION, -1,0,
// FRAME_LOOK1, 5,
// ACTION_POSITION, -1,0,
// FRAME_LOOK2, 20,
// ACTION_POSITION, -1,0,
// FRAME_LOOK1, 5,
// ACTION_POSITION, 0,0,
//    FRAME_NEW_X004_Y004, 255,
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
//     FRAME_NEW_X004_Y004,3, //FRAMEDELAY_RANDOM,
//     FRAME_TALK,6, //FRAMEDELAY_RANDOM,
//     FRAME_NEW_X004_Y004,4, //FRAMEDELAY_RANDOM,
//     FRAME_TALK,10, //FRAMEDELAY_RANDOM,
// #endif
//     FRAME_NEW_X004_Y004,10, //FRAMEDELAY_RANDOM,
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
//     FRAME_NEW_X004_Y004,10,
//     FRAME_HAIR2, 6,
//     FRAME_HAIR, 6,
//     FRAME_HAIR2, 6,
//     FRAME_HAIR, 6,
//     ACTION_STOP,
// };

// const signed char AnimationDrip2[] = {
// //    FRAME_IMPATIENT,25,
// //    FRAME_NEW_X004_Y004,10,
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

    AnimationDefault,        // 00 ID_Stand
    AnimationStandUp,        // 01 ID_StandUp
    AnimationStandLR,        // 02 ID_StandLR
    AnimationMine,           // 03 ID_Mine
    AnimationTurn,           // 04 ID_Turn
    AnimationDie,            // 05 ID_Die
    AnimationWalk,           // 06 ID_Walk
    AnimationLocked,         // 07 ID_Locked
    AnimationWalkUp,         // 08 ID_WalkUp
    AnimationWalkDown,       // 09 ID_WalkDown
    AnimationMineUp,         // 10 ID_MineUp
    AnimationMineDown,       // 11 ID_MineDown
    AnimationTapUp,          // 12 ID_TapUp
    AnimationTapDown,        // 13 ID_TapDown
    AnimationTapPush,        // 14 ID_TapPush
    AnimationPickup,         // 15 ID_Pickup
    AnimationStandArmsUp,    // 16 ID_StandArmsUp
    AnimationWalkUpSlow,     // 17 ID_WalkUpSlow
};

_Static_assert(sizeof(AnimationVector) / sizeof(AnimationVector[0]) == ID_MAX, "AnimationVector table wrong size");


enum AnimationIdent playerAnimationID = ID_Stand;
const signed char *playerAnimation = AnimationDefault;
static const signed char *playerAnimationLoop = AnimationDefault;
static unsigned int playerAnimationCount = 0;

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
            // if (playerDead)
            //     startPlayerAnimation(ID_Skeleton);

            // else {
            if (playerAnimationLoop)
                playerAnimation = playerAnimationLoop;
            else
                playerAnimation++;
            // }
            break;

        case ACTION_STOP:
            startPlayerAnimation(ID_Stand);
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
}


void updatePlayerAnimation() {

    if (autoMoveFrameCount)
        autoMoveFrameCount--;

#define RECIPROCAL (0x4000 / (SPEED_BASE))

    autoMoveX = ((autoMoveFrameCount * autoMoveDeltaX) >> 16);
    autoMoveY = ((autoMoveFrameCount * autoMoveDeltaY) >> 16);

    // Exit sequence: board.c's exitMode case starts the walk-up cycle (ID_WalkUp) once the
    // walk-in glide onto the exit tile has settled (autoMoveFrameCount == 0) -- from there, ease
    // the player up the screen a little, so they read as walking away into the distance through
    // the doorway instead of marching on the spot. Tied to exitMode's own countdown (EXIT_MODE_START
    // down to 0) rather than incremented once per real frame -- exitMode only ticks down at
    // board.c's scan cadence, much slower than every real frame, so a flat per-frame increment
    // covered far more than intended (looked like sprinting off, not walking) by the time exitMode
    // actually reached 0. >>1 (/2) instead of a real divide (this coprocessor has none -- see
    // swipe.c's isqrt() comment) -- EXIT_MODE_START (40) divides out to a clean 20 scanlines of
    // total drift by the time exitMode hits 0. frameAdjustY (drawPlayerSprite(), drawPlayer.c:
    // subtracted from ypos, so increasing it moves the sprite up) is otherwise only ever set by
    // ACTION_POSITION (processAnimationCommand() below), which AnimationWalkUp never uses, so
    // nothing else touches it while this runs.
    //
    // playerExitFade rides the same countdown as frameAdjustY, but x1.125 its rate (75% of the
    // previous x1.5 -- (diff + diff/2) * 3/4, all as adds/shifts, not real divides -- this
    // coprocessor has none, see swipe.c's isqrt() comment) -- so the player visibly darkens
    // alongside the walk (see its own comment, mellon.c, for why this needs to be separate from
    // the level's own luminance fade) without waiting for most of the walk to play out first.
    // Reaches full black (drawPlayerSprite() snaps to true black once its own luminance clamp
    // bottoms out, not just this hue's darkest shade) once exitMode counts down to
    // EXIT_MODE_START - 14, well before the walk itself ends.
    if (exitMode && !autoMoveFrameCount) {
        int exitProgress = EXIT_MODE_START - exitMode;
        frameAdjustY = exitProgress >> 1;
        playerExitFade = ((exitProgress + (exitProgress >> 1)) * 3) >> 2;

        // Door starts sliding shut just after the player is fully faded -- 15 is where
        // drawPlayerSprite()'s own luminance clamp guarantees full black and the sprite itself
        // stops being drawn at all (see playerExitFade's comment above); 16 gives it one tick's
        // worth of margin past that, so the player is unambiguously gone before the door starts
        // moving. The slide
        // itself (AnimateDoorClose, animations.c) plays out after the player is already gone, not
        // before -- there's no player left to race against by this point, only the level's own
        // separate background fade (lumTarget, board.c's exitMode case) still running. CH_EXITBLANK
        // (what actually got written to the board, mellon.c's exit trigger) is TYPE_OUTBOX --
        // currently pinned to the open-door glyph via that same trigger's
        // startCharAnimation()+haltCharAnimation() -- so this just re-points it at
        // AnimateDoorClose's trigger offset instead (skipping its frame-0 landing pad, same
        // pattern AnimateDoor's own trigger uses), which visibly slides shut and then holds on
        // CH_DOORCLOSED (delay 0) on its own, no extra halt needed. doorClosing (mellon.c) is the
        // one-shot guard, not a pointer comparison against Animate[TYPE_OUTBOX] -- that pointer
        // only equals AnimateDoorClose + 2 WHILE the slide is mid-flight; once it finishes
        // advancing past that (to the held CH_DOORCLOSED frame), a pointer-equality guard starts
        // matching "not yet triggered" again and re-fires forever, restarting the slide from
        // scratch every frame (this actually happened -- the door never settled, visibly cycling
        // between closed for one frame and re-opening).
        if (playerExitFade >= 16 && !doorClosing) {
            doorClosing = true;
            startCharAnimation(TYPE_OUTBOX, AnimateDoorClose + 2);
        }
    }

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
    playerExitFade = 0;

    processAnimationCommand();
}


// EOF