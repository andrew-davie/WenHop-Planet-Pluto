#pragma once

#include <stdbool.h>

void movePlayer(unsigned char *mes);
void initPlayer();
void grabDoge();
void bubbles(int count, int dripX, int dripY, int age, int /*speed*/);

extern int frameAdjustX;
extern int frameAdjustY;

extern unsigned int pushCounter;

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