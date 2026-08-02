#pragma once

#define SMALL_SPRITE_OFFSET 94
#define FRAMEDELAY_RANDOM 254


enum AnimationIdent {

    // see (player.c) AnimationVector[] -> animation program

    ID_Stand,          // 00
    ID_StandUp,        // 01
    ID_StandLR,        // 02
    ID_Mine,           // 03
    ID_Turn,           // 04
    ID_Die,            // 05
    ID_Walk,           // 06
    ID_Locked,         // 07
    ID_WalkUp,         // 08
    ID_WalkDown,       // 09
    ID_MineUp,         // 10
    ID_MineDown,       // 11
    ID_TapUp,          // 12
    ID_TapDown,        // 13
    ID_TapPush,        // 14
    ID_Pickup,         // 15
    ID_StandArmsUp,    // 16

    // Same 4 frames as ID_WalkUp, just held far longer each -- board.c's exit sequence uses this
    // instead of ID_WalkUp so the leg cycle reads as an unhurried walk-off into the distance
    // (matched to the frameAdjustY drift rate, updatePlayerAnimation()) without touching
    // ID_WalkUp's own speed, which is still used for ordinary up-facing movement everywhere else.
    ID_WalkUpSlow,     // 17

    ID_MAX
};

extern int autoMoveX;
extern int autoMoveY;
extern int autoMoveDeltaX;
extern int autoMoveDeltaY;
extern int autoMoveFrameCount;

extern const signed char *const AnimationVector[];    // animJames[];

extern enum AnimationIdent playerAnimationID;
extern const signed char *playerAnimation;

void processAnimationCommand();
void updatePlayerAnimation();
void startPlayerAnimation(enum AnimationIdent animID);

extern const unsigned char redirect[];

// EOF