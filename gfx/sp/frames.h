#pragma once
#include <stdint.h>

extern uint16_t frameLowTable[];
extern const unsigned char frameHeight[];
extern const unsigned char frameCenterX[];
extern const unsigned char frameCenterY[];

// LEM(r, g, b): expands to the combined byte followed by each plane byte.
// Plane encoding: r=red plane, g=green plane, b=blue plane (MSB=leftmost pixel).
#define LEM(r, g, b) ((r) | (g) | (b)), (r), (g), (b)

enum FRAME {
    FRAME_0, // 0
    ACTION_JOYSTICK, // 1
    ACTION_MOVE, // 2
    ACTION_POSITION, // 3
    ACTION_DELAY, // 4
    ACTION_FLIP, // 5
    ACTION_LOOP, // 6
    ACTION_PUSH, // 7
    ACTION_AUTOFRAME, // 8
    ACTION_NEXTSQUARE, // 9
    ACTION_STOP, // 10
};
