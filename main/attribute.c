#include "attribute.h"


const char BaseChar[] = {


};

const enum ObjectType CharToType[] = {

    // see ChName for corresponding character name/number
    // may need to update worstRequiredTime if that's in use

    TYPE_BLANK,                    // 0 CH_BLANK
    TYPE_PLACEHOLDER,              // 1 CH_PLACEHOLDER
    TYPE_DIRT,                     // 2 CH_DIRT
    TYPE_BRICKWALL,                // 3 CH_BRICKWALL
    TYPE_DOOR,                     // 4 CH_DOORCLOSED
    TYPE_OUTBOX,                   // 5 CH_DOOROPEN_0
    TYPE_OUTBOX,                   // 6 CH_EXITBLANK
    TYPE_STEELWALL,                // 7 CH_STEELWALL
    TYPE_PEBBLE1,                  // 8 CH_PEBBLE1
    TYPE_PEBBLE1,                  // 9 CH_PEBBLE2
    TYPE_ROCK,                     // 10 CH_ROCK
    TYPE_ROCK_FALLING,             // 11 CH_ROCK_FALLING
    TYPE_DOGE,                     // 12 CH_DOGE_00
    TYPE_DOGE_FALLING,             // 13 CH_DOGE_FALLING
    TYPE_MELLON_HUSK_PRE,          // 14 CH_MELLON_HUSK_BIRTH
    TYPE_LAVA,                     // 15 CH_LAVA_BLANK
    TYPE_LAVA,                     // 16 CH_LAVA_SMALL
    TYPE_LAVA,                     // 17 CH_LAVA_MEDIUM
    TYPE_LAVA,                     // 18 CH_LAVA_LARGE
    TYPE_MELLON_HUSK,              // 19 CH_MELLON_HUSK
    TYPE_DOGE,                     // 20 CH_DOGE_STATIC
    TYPE_PEBBLE_ROCK,              // 21 CH_PEBBLE_ROCK
    TYPE_ROCK_PEBBLE,              // 22 CH_ROCK_PEBBLE
    TYPE_ROCK_PEBBLE,              // 23 CH_ROCK_PEBBLE_1
    TYPE_DUST_0,                   // 24 CH_DUST_0
    TYPE_DUST_0,                   // 25 CH_DUST_1
    TYPE_DUST_0,                   // 26 CH_DUST_2
    TYPE_GEODOGE,                  // 27 CH_CONGLOMERATE
    TYPE_DUST_ROCK,                // 28 CH_DUST_ROCK_0
    TYPE_DUST_ROCK,                // 29 CH_DUST_ROCK_1
    TYPE_DUST_ROCK,                // 30 CH_DUST_ROCK_2
    TYPE_CONVERT_GEODE_TO_DOGE,    // 31 CH_CONVERT_GEODE_TO_DOGE
    TYPE_PUSHER,                   // 32 CH_HORIZONTAL_BAR
    TYPE_PUSHER,                   // 33 CH_PUSH_LEFT
    TYPE_PUSHER,                   // 34 CH_PUSH_LEFT_REVERSE
    TYPE_PUSHER,                   // 35 CH_PUSH_RIGHT
    TYPE_PUSHER,                   // 36 CH_PUSH_RIGHT_REVERSE
    TYPE_PUSHER,                   // 37 CH_VERTICAL_BAR
    TYPE_PUSHER,                   // 38 CH_PUSH_UP
    TYPE_PUSHER,                   // 39 CH_PUSH_UP_REVERSE
    TYPE_PUSHER,                   // 40 CH_PUSH_DOWN
    TYPE_PUSHER,                   // 41 CH_PUSH_DOWN_REVERSE
    TYPE_WYRM,                     // 42 CH_WYRM_BODY
    TYPE_WYRM,                     // 43 CH_WYRM_VERT_BODY
    TYPE_WYRM,                     // 44 CH_WYRM_CORNER_LD
    TYPE_WYRM,                     // 45 CH_WYRM_CORNER_RD
    TYPE_WYRM,                     // 46 CH_WYRM_CORNER_LU
    TYPE_WYRM,                     // 47 CH_WYRM_CORNER_RU
    TYPE_WYRM,                     // 48 CH_WYRM_HEAD_U
    TYPE_WYRM,                     // 49 CH_WYRM_HEAD_R
    TYPE_WYRM,                     // 50 CH_WYRM_HEAD_D
    TYPE_WYRM,                     // 51 CH_WYRM_HEAD_L
    TYPE_GEODOGE_FALLING,          // 52 CH_GEODOGE_FALLING
    TYPE_FLIP_GRAVITY,             // 53 CH_FLIP_GRAVITY_0
    TYPE_FLIP_GRAVITY,             // 54 CH_FLIP_GRAVITY_1
    TYPE_FLIP_GRAVITY,             // 55 CH_FLIP_GRAVITY_2
    TYPE_BLOCK,                    // 56 CH_BLOCK
    TYPE_GRINDER,                  // 57 CH_GRINDER_0
    TYPE_GRINDER_1,                // 58 CH_GRINDER_1
    TYPE_HUB,                      // 59 CH_HUB
    TYPE_WATER,                    // 60 CH_WATER_0
    TYPE_WATERFLOW_0,              // 61 CH_WATERFLOW_0
    TYPE_WATERFLOW_1,              // 62 CH_WATERFLOW_1
    TYPE_WATERFLOW_2,              // 63 CH_WATERFLOW_2
    TYPE_WATERFLOW_3,              // 64 CH_WATERFLOW_3
    TYPE_WATERFLOW_4,              // 65 CH_WATERFLOW_4
    TYPE_HUB,                      // 66 CH_HUB_1
    TYPE_OUTLET,                   // 67 CH_OUTLET
    TYPE_BELT,                     // 68 CH_BELT_0
    TYPE_BELT_1,                   // 69 CH_BELT_1
    TYPE_PUSHER,                   // 70 CH_PUSH_DOWN2
    TYPE_CONVERT_GEODE_TO_DOGE,    // 71 CH_GEODOGE_CONVERT  (deprecated)
    TYPE_CONVERT_PIPE,             // 72 CH_CONVERT_PIPE
    TYPE_WYRM,                     // 73 CH_WYRM_TAIL_U
    TYPE_WYRM,                     // 74 CH_WYRM_HEAD_R
    TYPE_WYRM,                     // 75 CH_WYRM_HEAD_D
    TYPE_WYRM,                     // 76 CH_WYRM_HEAD_L,
    TYPE_DOGE_FALLING,             // 77 CH_DOGE_FALLING_TOP
    TYPE_DOGE_FALLING,             // 78 CH_DOGE_FALLING_BOTTOM
    TYPE_ROCK_FALLING,             // 79 CH_ROCK_FALLING_TOP
    TYPE_ROCK_FALLING,             // 80 CH_ROCK_FALLING_BOTTOM
    TYPE_GEODOGE_FALLING,          // 81 CH_GEODOGE_FALLING_TOP
    TYPE_GEODOGE_FALLING,          // 82 CH_GEODOGE_FALLING_BOTTOM
    TYPE_DOGE_FALLING2,            // 83 CH_DOGE_FALLING_TOP2
    TYPE_DOGE_FALLING2,            // 84 CH_DOGE_FALLING_BOTTOM2
    TYPE_DOGE_FALLING2,            // 85 CH_DOGE_SIDE_1
    TYPE_DOGE_FALLING2,            // 86 CH_DOGE_SIDE_3
    TYPE_DOGE_FALLING2,            // 87 CH_DOGE_SIDE_2
    TYPE_DOGE_FALLING2,            // 88 CH_DOGE_SIDE_4
    TYPE_ELECTRIC,                 // 89 CH_ELECTRIC_0
    TYPE_ELECTRIC,                 // 90 CH_ELECTRIC_1
    TYPE_ELECTRIC,                 // 91 CH_ELECTRIC_2
    TYPE_ELECTRIC,                 // 92 CH_ELECTRIC_3
    TYPE_DIRT,                     // 93 CH_BROKEN_DIRT  (not actually used on board)
    TYPE_INSULATOR,                // 94 CH_INSULATOR_TOP
    TYPE_INSULATOR,                // 95 CH_INSULATOR_BOTTOM
    TYPE_STAR,                     // 96 CH_STAR
    TYPE_STAR_FALLING,             // 97 CH_STAR_TOP
    TYPE_STAR_FALLING,             // 98 CH_STAR_BOTTOM
    TYPE_ROCK_BONUS,               // 99 CH_ROCK_BONUS
    TYPE_STAR_EXPLODE,             // 100 CH_STAR_EXPLODE
    TYPE_INSULATOR,                // 101 CH_INSULATOR_L
    TYPE_INSULATOR,                // 102 CH_INSULATOR_R
    TYPE_ELECTRIC,                 // 103 CH_ELECTRIC_H0
    TYPE_ELECTRIC,                 // 104 CH_ELECTRIC_H1
    TYPE_ELECTRIC,                 // 105 CH_ELECTRIC_H2
    TYPE_ELECTRIC,                 // 106 CH_ELECTRIC_H3
    TYPE_ELECTRIC,                 // 107 CH_CROSSED_STREAMS
    TYPE_BOMB,                     // 108 CH_BOMB
    TYPE_CRACKED_BRICK,            // 109 CH_CRACKED_BRICK
    TYPE_CONCRETE,                 // 110 CH_CONCRETE
    TYPE_TELEPORT,                 // 111 CH_TELEPORT
    TYPE_KEY,                      // 112 CH_KEY
    TYPE_DOOR_OPEN,                // 113 CH_DOOROPEN_STATIC
    TYPE_IMMOVABLE,                // 114 CH_IMMOVABLE
    TYPE_IMMOVABLE_FALLING,        // 115 CH_IMMOVABLE_FALLING
    TYPE_IMMOVABLE_FALLING,        // 116 CH_IMMOVABLE_FALLING_TOP
    TYPE_IMMOVABLE_FALLING,        // 117 CH_IMMOVABLE_FALLING_BOTTOM
    TYPE_ROCK_ROLLING,             // 118 CH_ROCK_SIDE_1
    TYPE_ROCK_ROLLING,             // 119 CH_ROCK_SIDE_2
    TYPE_ROCK_ROLLING,             // 120 CH_ROCK_SIDE_3
    TYPE_ROCK_ROLLING,             // 121 CH_ROCK_SIDE_4
    0,                             // 122 (unused)
    0,                             // 123 (unused)
    0,                             // 124 (unused)
    0,                             // 125 (unused)
    0,                             // 126 (unused)
    0,                             // 127 (unused)

    // chars 128 onwards are virtual
    // THESE SHOULD NOT BE CALLING CharToType !!!!

    // TYPE_DOGE,             // 128 CH_DOGE_01
    // TYPE_DOGE,             // 129 CH_DOGE_02
    // TYPE_DOGE,             // 130 CH_DOGE_03
    // TYPE_DOGE,             // 131 CH_DOGE_04
    // TYPE_DOGE,             // 132 CH_DOGE_05
    //                        //
    // TYPE_CRACKED_BRICK,    // 142 CH_CRACKED_BRICK_1
    // TYPE_CRACKED_BRICK,    // 143 CH_CRACKED_BRICK_1
    // TYPE_CRACKED_BRICK,    // 144 CH_CRACKED_BRICK_1
    // TYPE_CRACKED_BRICK,    // 145 CH_CRACKED_BRICK_1
    // TYPE_CRACKED_BRICK,    // 146 CH_CRACKED_BRICK_1
    // TYPE_CRACKED_BRICK,    // 147 CH_CRACKED_BRICK_1
    // TYPE_CRACKED_BRICK,    // 148 CH_CRACKED_BRICK_1

};

//_Static_assert(sizeof(CharToType) / sizeof(CharToType[0]) == CH_MAX, "CharToType table wrong size");


enum AttributeAlias {

    ROL = ATT_ROLL,                    // doge should roll off this object
    CVT = ATT_CONVERT,                 // iff below surface, object converts to lava or water
    XPD = ATT_EXPLODABLE,              // object is destroyed in explosions
    PER = ATT_PERMEABLE,               // square is 'permeable' (not solid/not blank)
    SPC = ATT_BLANK,                   // object is blank
    DRT = ATT_DIRT,                    // object is dirt
    GRB = ATT_GRAB,                    // doge
    PSH = ATT_PUSH,                    //
    SQB = ATT_SQUASHABLE_TO_BLANKS,    //
    HRD = ATT_HARD,                    // Hard. rocks create dust when falling onto
    XIT = ATT_EXIT,                    //
    QUI = ATT_NOROCKNOISE,             //
    RKF = ATT_BLANK,                   //
    MIN = ATT_MINE,                    //
    WTF = ATT_WATERFLOW,               //
    CVY = ATT_CONVEYOR,                //
    GND = ATT_GRIND,                   //
    PIP = ATT_PIPE,                    //
    PH4 = ATT_PHASE4,                  // processed every 4 frames
    PH2 = ATT_PHASE2,                  // processed every 2 frames
    PH1 = ATT_PHASE1,                  // processed every 1 frame
    DIS = ATT_DISSOLVES,               //
    MLT = ATT_MELTS,                   //
    DGE = ATT_GEODOGE,                 // a geodoge
    PAD = ATT_PAD,                     // add dirt corners to this object square
    CRU = ATT_CRUSHES,                 // object above crushes player
    CNR = ATT_CORNER,                  // this object contributes to making a dirt corner
    PUP = ATT_PICKUP,                  // object can be picked up
    MAS = ATT_MASSIVE,                 // has weight, affects crackable
    DRP = ATT_DRIP,                    // rain drips can form on the cell below this one
    SHV = ATT_SHOVE,                   // can be pushed sideways by the player walking into it
};


const unsigned int Attribute[] = {

// see attribute.h "ObjectType"

#define _ 0

    // clang-format off

// CNR PAD DGE MLT DIS PH* PIP GND CVY WTF MIN RKF QUI XIT HRD SQB PSH GRB MAS DRT SPC PER XPD CVT CRU ROL PUP DRP SHV
// ---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    _ |PAD| _ | _ | _ | _ | _ | _ | _ | _ | _ |RKF|QUI| _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD|CVT| _ | _ | _ | _ | _ , // 0 TYPE_BLANK,
    _ |PAD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 1 TYPE_PLACEHOLDER,
   CNR| _ | _ | _ |DIS| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |DRT| _ |PER|XPD|CVT| _ | _ | _ |DRP| _ , // 2 TYPE_DIRT,
    _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |ROL| _ |DRP| _ , // 3 TYPE_BRICKWALL,
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 4 TYPE_DOOR,
    _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |MIN| _ | _ |XIT| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 5 TYPE_OUTBOX,
    _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |DRP| _ , // 6 TYPE_STEELWALL,
    _ |PAD| _ |MLT| _ |PH1| _ |GND|CVY| _ |MIN| _ | _ | _ |HRD| _ |PSH| _ |MAS| _ | _ | _ |XPD| _ | _ |ROL|PUP| _ | _ , // 7 TYPE_ROCK,
    _ |PAD| _ |MLT| _ |PH4| _ |GND|CVY| _ | _ | _ | _ | _ | _ | _ |PSH|GRB| _ | _ | _ | _ |XPD| _ | _ |ROL|PUP| _ | _ , // 8 TYPE_DOGE,
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 9 TYPE_MELLON_HUSK_PRE,
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ |QUI| _ | _ |SQB|PSH| _ | _ | _ | _ | _ |XPD| _ | _ | _ | _ | _ | _ , // 10 TYPE_MELLON_HUSK,
   CNR|PAD| _ | _ |DIS|PH4| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |DRT| _ |PER|XPD| _ | _ | _ | _ |DRP| _ , // 11 TYPE_PEBBLE1,
    _ |PAD| _ | _ | _ |PH2| _ | _ | _ | _ | _ |RKF|QUI| _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD| _ | _ | _ | _ | _ | _ , // 12 TYPE_DUST_0,
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |PSH| _ | _ | _ | _ | _ |XPD| _ | _ |ROL| _ | _ | _ , // 13 TYPE_DOGE_FALLING,
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ |PSH| _ | _ | _ | _ | _ |XPD| _ |CRU| _ | _ | _ | _ , // 14 TYPE_ROCK_FALLING,
    _ |PAD| _ | _ | _ |PH2| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD| _ | _ | _ | _ | _ | _ , // 15 TYPE_DUST_ROCK,
    _ |PAD| _ |MLT| _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |XPD| _ | _ |ROL| _ | _ | _ , // 16 TYPE_CONVERT_GEODE_TO_DOGE
    _ | _ | _ | _ | _ |PH4|PIP| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |ROL| _ | _ | _ , // 17 TYPE_PUSHER,
    _ | _ | _ | _ | _ |PH1|PIP| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 18 TYPE_PUSHER_VERT,
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 19 TYPE_WYRM,
    _ |PAD|DGE|MLT| _ |PH1| _ |GND|CVY| _ |MIN| _ | _ | _ |HRD| _ |PSH| _ |MAS| _ | _ | _ |XPD| _ | _ |ROL|PUP| _ | _ , // 20 TYPE_GEODOGE,
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ |PSH| _ | _ | _ | _ | _ |XPD| _ |CRU| _ | _ | _ | _ , // 21 TYPE_GEODOGE_FALLING,
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER| _ | _ | _ | _ | _ | _ | _ , // 22 TYPE_LAVA,
   CNR|PAD| _ | _ |DIS|PH4| _ | _ | _ | _ | _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ | _ |PER|XPD| _ | _ | _ | _ | _ | _ , // 23 TYPE_PEBBLE_ROCK,
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ |RKF| _ | _ | _ | _ | _ | _ | _ |DRT| _ |PER|XPD| _ | _ | _ | _ | _ | _ , // 24 TYPE_FLIP_GRAVITY
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 25 TYPE_BLOCK
    _ |PAD| _ | _ | _ |PH4| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |ROL| _ | _ | _ , // 26 TYPE_GRINDER
    _ | _ | _ | _ | _ |PH4|PIP| _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 27 TYPE_HUB
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD| _ | _ | _ | _ | _ | _ , // 28 TYPE_WATER
    _ | _ | _ | _ |DIS|PH1| _ | _ | _ |WTF| _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD|CVT| _ | _ | _ | _ | _ , // 29 TYPE_WATERFLOW0
    _ | _ | _ | _ |DIS|PH1| _ | _ | _ |WTF| _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD|CVT| _ | _ | _ | _ | _ , // 30 TYPE_WATERFLOW1
    _ | _ | _ | _ |DIS|PH1| _ | _ | _ |WTF| _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD|CVT| _ | _ | _ | _ | _ , // 31 TYPE_WATERFLOW2
    _ | _ | _ | _ |DIS|PH1| _ | _ | _ |WTF| _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD|CVT| _ | _ | _ | _ | _ , // 32 TYPE_WATERFLOW3
    _ | _ | _ | _ |DIS|PH1| _ | _ | _ |WTF| _ |RKF| _ | _ | _ | _ | _ | _ | _ | _ |SPC|PER|XPD|CVT| _ | _ | _ | _ | _ , // 33 TYPE_WATERFLOW4
    _ |PAD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 34 TYPE_TAP
    _ | _ | _ | _ | _ |PH1|PIP| _ | _ |WTF| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 35 TYPE_OUTLET
    _ |PAD| _ | _ | _ |PH4| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |ROL| _ | _ | _ , // 36 TYPE_GRINDER1
    _ |PAD| _ | _ | _ |PH2| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |ROL| _ | _ | _ , // 37 TYPE_BELT
    _ |PAD| _ | _ | _ |PH2| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |ROL| _ | _ | _ , // 38 TYPE_BELT_1
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 39 TYPE_CONVERT_PIPE
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |PSH| _ | _ | _ | _ | _ |XPD| _ | _ | _ | _ | _ | _ , // 40 TYPE_DOGE_FALLING2
    _ |PAD| _ | _ |DIS|PH4| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |XPD| _ | _ | _ | _ | _ | _ , // 41 TYPE_ROCK_PEBBLE
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |SPC| _ | _ | _ | _ |ROL| _ | _ | _ , // 42 TYPE_ELECTRIC
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |ROL| _ | _ | _ , // 43 TYPE_INSULATOR
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |GRB| _ | _ | _ |PER| _ | _ | _ | _ |PUP| _ | _ , // 44 TYPE_STAR
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ |PSH| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 45 TYPE_STAR_FALLING
    _ |PAD| _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 46 TYPE_STAR_EXPLODING
    _ |PAD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ |PSH| _ | _ | _ | _ | _ | _ | _ | _ |ROL|PUP| _ | _ , // 47 TYPE_ROCK_BONUS
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 48 TYPE_BOMB
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ |XPD| _ | _ | _ | _ | _ | _ , // 49 TYPE_CRACKED_BRICK
    _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ |XPD| _ | _ |ROL| _ |DRP| _ , // 50 TYPE_CONCRETE
    _ |PAD| _ | _ | _ |PH4| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |PER| _ | _ | _ | _ | _ | _ | _ | _ , // 51 TYPE_TELEPORT (walkable, solid -- like TYPE_STAR's ground; PH4 drives its idle sparkle, see board.c)
    _ |PAD| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 52 TYPE_KEY (solid ground pickup -- auto-grabbed by walking onto it, see mellon.c's checkHighPriorityMove; blocked while already carrying something)
    _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |MIN| _ | _ |XIT| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ | _ , // 53 TYPE_DOOR_OPEN (same walkable/exit bits as TYPE_OUTBOX, deliberately not TYPE_OUTBOX itself -- see its own comment)
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ |MAS| _ | _ | _ | _ | _ | _ |ROL| _ | _ |SHV, // 54 TYPE_IMMOVABLE
    _ | _ | _ | _ | _ |PH1| _ | _ | _ | _ | _ | _ | _ | _ |HRD| _ | _ | _ | _ | _ | _ | _ | _ | _ |CRU| _ | _ | _ | _ , // 55 TYPE_IMMOVABLE_FALLING
    _ |PAD| _ | _ | _ |PH2| _ | _ | _ | _ | _ | _ | _ | _ | _ | _ |PSH| _ | _ | _ | _ | _ |XPD| _ | _ | _ | _ | _ | _ , // 56 TYPE_ROCK_ROLLING -- PH2 (half speed) vs TYPE_ROCK_FALLING's PH1, so the roll's midpoint visibly lingers instead of snapping through in one scan pass
// ---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---|---+---+---+

    // clang-format on
};

_Static_assert(sizeof(Attribute) / sizeof(Attribute[0]) == TYPE_MAX, "Attribute table wrong size");

// EOF
