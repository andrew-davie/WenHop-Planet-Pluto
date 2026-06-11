#pragma once

#define SPRITE_DOUBLE 0x80
#define SPRITE_ABSCOLOUR 0x40

extern const unsigned char *const spriteShape[];


enum SPRITE_COLOURS {

    NONE = 0x00,
    BONE = 0x08,
    HMT0 = 0x09,
    HMT1 = 0x0A,
    HMT2 = 0x0B,
    HMT3 = 0x0C,
    BDY0 = 0x0D,
    BDY1 = 0x0E,
    BDY2 = 0x0F,
};

enum FRAME {

    // see (sprites.c) -> create a frame shape_* and add to spriteShape[] table

    FRAME_BLANK,          // 00
    FRAME_STAND,          // 01
    FRAME_ARMS_IN_AIR,    // 02
    FRAME_HUNCH,          // 03
    FRAME_PUSH,           // 04
    FRAME_PUSH2,          // 05
    FRAME_WALK1,          // 06
    FRAME_WALK2,          // 07
    FRAME_WALK3,          // 08
    FRAME_WALK4,          // 09
    FRAME_SKELETON1,      // 10
    FRAME_SKELETON2,      // 11
    FRAME_SKELETON3,      // 12
    FRAME_SKELETON4,      // 13
    FRAME_SKELETON5,      // 14
    FRAME_WALKUP0,        // 15
    FRAME_WALKUP1,        // 16
    FRAME_WALKUP2,        // 17
    FRAME_WALKUP3,        // 18
    FRAME_WALKDOWN0,      // 19
    FRAME_WALKDOWN1,      // 20
    FRAME_WALKDOWN2,      // 21
    FRAME_WALKDOWN3,      // 22
    FRAME_MINE_UP_0,      // 23
    FRAME_MINE_UP_1,      // 24
    FRAME_MINE_DOWN_0,    // 25
    FRAME_MINE_DOWN_1,    // 26
    FRAME_15,             // 27
    FRAME_16,             // 28
    FRAME_17,             // 29
    FRAME_18,             // 30
    FRAME_19,             // 31
    FRAME_20,             // 32
    FRAME_21,             // 33
    FRAME_24,             // 34
    FRAME_25,             // 35
    FRAME_26,             // 36
    FRAME_27,             // 37
    FRAME_28,             // 38
    FRAME_29,             // 39
    FRAME_30,             // 40
    FRAME_31,             // 41
    FRAME_32,             // 42
    FRAME_33,             // 43
    FRAME_34,             // 44
    FRAME_35,             // 45
    FRAME_36,             // 46
    FRAME_37,             // 47
    FRAME_38,             // 48
    FRAME_40,             // 49
    FRAME_41,             // 50
    FRAME_42,             // 51
    FRAME_44,             // 52
    FRAME_45,             // 53
    FRAME_47,             // 54
    FRAME_48,             // 55
    FRAME_49,             // 56
    FRAME_50,             // 57
    FRAME_51,             // 58

    FRAME_MAX,

    // actions start after frames

    ACTION_DOT,
    ACTION_SFX,
    ACTION_SAY,
    ACTION_POSITION,
    ACTION_FLIP,
    ACTION_LOOP,
    ACTION_STOP,

};


// EOF
