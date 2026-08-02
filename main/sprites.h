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

    FRAME_BLANK,            // 00
    FRAME_NEW_X004_Y004,    // 01
    FRAME_ARMS_IN_AIR,      // 02
    FRAME_NEW_X020_Y274,    // 03
    FRAME_NEW_X017_Y124,    // 04
    FRAME_NEW_X000_Y124,    // 05
    FRAME_NEW_X004_Y064,    // 06
    FRAME_WALK2,            // 07
    FRAME_WALK3,            // 08
    FRAME_WALK4,            // 09
    // FRAME_SKELETON1,  // currently unused
    // FRAME_SKELETON2,  // currently unused
    // FRAME_SKELETON3,  // currently unused
    // FRAME_SKELETON4,  // currently unused
    // FRAME_SKELETON5,  // currently unused
    FRAME_NEW_X004_Y034,    // 10
    FRAME_NEW_X020_Y033,    // 11
    FRAME_WALKUP2,          // 12
    FRAME_NEW_X052_Y033,    // 13
    FRAME_NEW_X004_Y094,    // 14
    FRAME_NEW_X020_Y093,    // 15
    FRAME_WALKDOWN2,        // 16
    FRAME_NEW_X052_Y093,    // 17
    FRAME_MINE_UP_0,        // 18
    FRAME_MINE_UP_1,        // 19
    FRAME_MINE_DOWN_0,      // 20
    FRAME_MINE_DOWN_1,      // 21
    // FRAME_15,  // currently unused
    // FRAME_16,  // currently unused
    // FRAME_20,  // currently unused
    // FRAME_21,  // currently unused
    // FRAME_24,  // currently unused
    // FRAME_25,  // currently unused
    // FRAME_26,  // currently unused
    // FRAME_27,  // currently unused
    // FRAME_29,  // currently unused
    // FRAME_30,  // currently unused
    // FRAME_32,  // currently unused
    // FRAME_35,  // currently unused
    // FRAME_36,  // currently unused
    // FRAME_37,  // currently unused
    // FRAME_38,  // currently unused
    // FRAME_40,  // currently unused
    // FRAME_41,  // currently unused
    // FRAME_42,  // currently unused
    // FRAME_44,  // currently unused
    // FRAME_DIE_0,  // currently unused
    // FRAME_DIE_1,  // currently unused
    // FRAME_DIE_2,  // currently unused
    // FRAME_DIE_3,  // currently unused
    // FRAME_DIE_4,  // currently unused
    FRAME_PICKUP,     // 22
    FRAME_PICKUP2,    // 23

    // FRAME_NEW_X019_Y063,  // currently unused
    // FRAME_NEW_X051_Y063,  // currently unused
    // FRAME_NEW_X020_Y150,  // currently unused
    // FRAME_NEW_X020_Y181,  // currently unused
    // FRAME_NEW_X004_Y183,  // currently unused
    // FRAME_NEW_X036_Y241,  // currently unused
    // FRAME_NEW_X018_Y303,  // currently unused
    // FRAME_NEW_X116_Y364,  // currently unused
    // FRAME_NEW_X115_Y410,  // currently unused
    // FRAME_NEW_X128_Y410,  // currently unused
    // FRAME_NEW_X101_Y412,  // currently unused
    // FRAME_NEW_X019_Y429,  // currently unused
    // FRAME_NEW_X023_Y454,  // currently unused
    // FRAME_NEW_X018_Y481,  // currently unused
    // FRAME_NEW_X002_Y510,  // currently unused
    // FRAME_NEW_X018_Y510,  // currently unused
    // FRAME_NEW_X019_Y546,  // currently unused
    // FRAME_NEW_X051_Y546,  // currently unused
    // FRAME_NEW_X003_Y547,  // currently unused
    // FRAME_NEW_X019_Y576,  // currently unused
    // FRAME_NEW_X051_Y576,  // currently unused
    // FRAME_NEW_X003_Y577,  // currently unused
    // FRAME_NEW_X019_Y606,  // currently unused
    // FRAME_NEW_X051_Y606,  // currently unused
    // FRAME_NEW_X004_Y607,  // currently unused
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
