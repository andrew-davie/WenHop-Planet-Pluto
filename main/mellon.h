#pragma once

#include <stdbool.h>

#include "board.h"

void movePlayer(BoardCursor *cur);
void initPlayer();
void grabDoge();
void bubbles(int count, int dripX, int dripY, int age, int /*speed*/);

typedef struct {

    signed char x;
    signed char y;

} OFFSET;


extern const OFFSET *attachmentOffset;

extern int frameAdjustX;
extern int frameAdjustY;

enum JOYSTICK_DIRECTION {
    JOYSTICK_UP = 1,
    JOYSTICK_DOWN = 2,
    JOYSTICK_LEFT = 4,
    JOYSTICK_RIGHT = 8,
};

enum FaceDirectionX {
    FACE_LEFT = -1,
    FACE_RIGHT = 1,
    FACE_UP = -1,
    FACE_DOWN = 1,
};

extern enum FaceDirectionX faceDirection;
extern int playerX, playerY;

extern bool playerDead;
// extern bool playerDeadRelease;

// EOF