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
    ID_Skeleton,       // 07
    ID_Locked,         // 08
    ID_WalkUp,         // 09
    ID_WalkDown,       // 10
    ID_MineUp,         // 11
    ID_MineDown,       // 12
    ID_TapUp,          // 13
    ID_TapDown,        // 14
    ID_TapPush,        // 15
    ID_Xray,           // 16
    ID_Pickup,         // 17
    ID_StandArmsUp,    // 18

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
extern const unsigned char *const spriteShape[];

// EOF