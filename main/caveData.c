
#include "defines_dasm.h"

#include "attribute.h"
#include "caveData.h"
#include "decodeCaves.h"
#include "main.h"

#define DIRT CH_DIRT
#define STEEL CH_STEELWALL

#define R 2

#define LINER(char, x, y, length, direction) DRAW_LINE, char, x, y, direction, length,


const unsigned char caveUseWall[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    3,17,
    BOARD_TRIX_X-SCREEN_TRIX_X-3,17,

    20,                             // milling
    10, 15,                         // doge $
    0,                              // shake

     17,  11,  50,  56,  8,         // randomiser[level]
     30,  12,  12,  12,  12,        // doge req
    200, 200, 200, 200, 200,

    WEAPON_MACE,                    // 0
    WEAPON_MACE,                    // 1
    WEAPON_MACE,                    // 2
    WEAPON_MACE,                    // 3
    WEAPON_MACE,                    // 4

//    CAVEDEF_LOCK_Y,
    0, CH_STEELWALL, CH_BLANK,           // flags, border, fill

    // Random objects

    3,
    CH_DIRT,10,20,20,20,20,
    CH_GEODOGE, 80,40,40,40,40,
    CH_ROCK, 50,40,40,40,40,


    CH_INSULATOR_L, 4,4,
    CH_INSULATOR_R, 8,4,
    // Start of cave draw


    CH_ROCK_BONUS, 7,3,

    DRAW_RECT,CH_BRICKWALL, 0,1,40,8,

    CH_INSULATOR_TOP, 6,2,
    CH_INSULATOR_BOTTOM, 6,7,


    CH_INSULATOR_TOP, 11,2,
    CH_INSULATOR_BOTTOM, 11,7,

    CH_BRICKWALL, 14,2,
    CH_INSULATOR_TOP, 14,3,
    CH_INSULATOR_BOTTOM, 14,6,
    CH_BRICKWALL, 14,7,


    CH_INSULATOR_TOP, 17,2,
    CH_INSULATOR_BOTTOM, 17,7,

    CH_BRICKWALL, 20,2,
    CH_INSULATOR_TOP, 20,3,
    CH_INSULATOR_BOTTOM, 20,6,
    CH_BRICKWALL, 20,7,

    CH_INSULATOR_TOP, 23,2,
    CH_INSULATOR_BOTTOM, 23,7,

    CH_BRICKWALL, 26,2,
    CH_INSULATOR_TOP, 26,3,
    CH_INSULATOR_BOTTOM, 26,6,
    CH_BRICKWALL, 26,7,

    CH_INSULATOR_TOP, 29,2,
    CH_INSULATOR_BOTTOM, 29,7,
    
    CH_BRICKWALL, 32,2,
    CH_BRICKWALL, 32,3,
    CH_BRICKWALL, 32,4,
    CH_INSULATOR_TOP, 32,5,
    CH_INSULATOR_BOTTOM, 32,7,

    DRAW_FILLED_RECT, CH_GEODOGE, 30, 2, 10, 5, CH_GEODOGE,

    CH_DOORCLOSED, 38, 5,
    CH_MELLON_HUSK_BIRTH, 2, 3,

    CH_WYRM_HEAD_U, 38, 2,

    CH_INSULATOR_L, 9,5,
    CH_INSULATOR_R, 30,5,

    DRAW_EOF,

    // EXTRAS
    // LEVEL 0 HERE

    DRAW_EOF,

     // LEVEL 1
    DRAW_EOF,

    // LEVEL 2
    DRAW_EOF,

    // LEVEL 3
    DRAW_EOF, // LEVEL 4

    DRAW_EOF,

    'T', 'E', 'S', 'T', END_STRING

    // clang-format on
};

//------------------------------------------------------------------------------

const unsigned char caveWyrms[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    7,17,7,17,


    20,     // milling
    10, 15, // doge $
    0,          // weather

    10, 11, 50, 56, 8, // randomiser[level]
    20, 12, 12, 12, 12,
    200, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_MACE,                    //0
    WEAPON_MACE,                    //1
    WEAPON_MACE,                    //2
    WEAPON_MACE,                    //3
    WEAPON_MACE,                    //4

    //CAVEDEF_LOCK_X|CAVEDEF_LOCK_Y|
    CAVEDEF_BONUS|CAVEDEF_START_WITH_WEAPON, CH_BLANK, CH_BRICKWALL,

    0,

    DRAW_FILLED_RECT,CH_STEELWALL,1,1,9,8,CH_GEODOGE,
    DRAW_FILLED_RECT,CH_BRICKWALL,4,4,3,3,CH_ROCK,


    CH_DOORCLOSED, 5, 4,
    CH_MELLON_HUSK_BIRTH, 5, 5,

    DRAW_EOF,

    // EXTRAS

    // LEVEL 0...
    DRAW_EOF,
    // LEVEL 1...
    
    DRAW_EOF,
    // LEVEL 2...

    DRAW_EOF,
    // LEVEL 3...

    DRAW_EOF,
    // LEVEL 4...
    
    DRAW_EOF,

    'T', 'E', 'S', 'T', END_STRING
    // clang-format on
};

//------------------------------------------------------------------------------

const unsigned char caveMace[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,

    20,     // milling
    10, 15, // doge $
    5,      //              ,          // rain

    
    10, 11, 50, 56, 8, // randomiser[level]
    1, 12, 12, 12, 12,
    200, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_GUN,                    //0
    WEAPON_MACE,                    //1
    WEAPON_MACE,                    //2
    WEAPON_MACE,                    //3
    WEAPON_MACE,                    //4

    0, CH_BRICKWALL, CH_DIRT,

    1,
    CH_BLANK, 10,100,100,100,100,

//    DRAW_FILLED_RECT,CH_STEELWALL,1,1,9,8,CH_DIRT,
    // DRAW_FILLED_RECT,CH_BRICKWALL,4,4,3,3,CH_ROCK,


    CH_DOORCLOSED, 5, 4,
    CH_MELLON_HUSK_BIRTH, 5, 5,


    //DRAW_LINE,CH_WATERFLOW_0,1,1,38,2,
    
    // DRAW_LINE,CH_TAP_0,1,1,2,10,
    DRAW_LINE,CH_HUB,1,2,2,10,
    DRAW_LINE,CH_OUTLET,1,3,2,10,
    CH_INSULATOR_TOP,12,1,


    DRAW_EOF,

    // EXTRAS
    // LEVEL 0
    DRAW_EOF, // LEVEL 1
    DRAW_EOF, // LEVEL 2
    DRAW_EOF, // LEVEL 3
    DRAW_EOF, // LEVEL 4
    DRAW_EOF,

    'T', 'E', 'S', 'T', END_STRING
    // clang-format on
};

const unsigned char caveTest[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,
    
    0,     // milling
    10, 15, // doge $
    0,      //              ,          // rain

    
    10, 11, 50, 56, 8, // randomiser[level]
    25, 12, 12, 12, 12, 200, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_MACE,                    //0
    WEAPON_NONE,                    //1
    WEAPON_NONE,                    //2
    WEAPON_NONE,                    //3
    WEAPON_NONE,                    //4

    0, STEEL, CH_DIRT,

    1,
 CH_BLANK, 60, 255, 0, 255, 10,


    CH_MELLON_HUSK_BIRTH, 20, 10,

    DRAW_EOF,

    // EXTRAS
    // LEVEL 0
    DRAW_EOF, // LEVEL 1
    DRAW_EOF, // LEVEL 2
    DRAW_EOF, // LEVEL 3
    DRAW_EOF, // LEVEL 4
    DRAW_EOF,

    'T', 'E', 'S', 'T', END_STRING
    // clang-format on
};


const unsigned char caveFast[] = {
    // clang-format of

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    0, 0, BOARD_TRIX_X, BOARD_TRIX_Y,

    20,        // milling
    10, 15,    // doge $
    5,         //              ,          // rain


    10, 11, 50, 56, 8,    // randomiser[level]
    25, 12, 12, 12, 12, 200, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_NONE,    // 0
    WEAPON_NONE,    // 1
    WEAPON_NONE,    // 2
    WEAPON_NONE,    // 3
    WEAPON_NONE,    // 4

    0, STEEL, DIRT,

    0,


    CH_DOORCLOSED, 38, 16, CH_MELLON_HUSK_BIRTH, 10, 15,


    LINER(CH_BLANK, 0, 15, 10, 2) LINER(CH_BLANK, 10, 15, 6, 0)


        DRAW_EOF,

    // EXTRAS
    // LEVEL 0
    DRAW_EOF,    // LEVEL 1
    DRAW_EOF,    // LEVEL 2
    DRAW_EOF,    // LEVEL 3
    DRAW_EOF,    // LEVEL 4
    DRAW_EOF,

    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


const unsigned char caveA[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,

    80,     // milling
    10, 15, // doge $
    5,      //              ,          // rain

    
    10, 11, 50, 56, 8, // randomiser[level]
    25, 12, 12, 12, 12, 200, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_GUN,                    //0
    WEAPON_NONE,                    //1
    WEAPON_NONE,                    //2
    WEAPON_NONE,                    //3
    WEAPON_NONE,                    //4

    0, STEEL, DIRT,

    2,
    CH_BLANK, 60, 255, 0, 255, 10,
    CH_ROCK, 50, 0, 240, 0, 20,


    // //    0xFE,CH_PUSH_DOWN,7,1,
    //   //  0xFE,CH_PUSH_DOWN,9,1,
    //     //0xFE,CH_PUSH_DOWN,11,1,
    //     //0xFE,CH_PUSH_DOWN,13,1,


    CH_WYRM_BODY, 12, 8,

    CH_HORIZONTAL_BAR, 0,3,
    CH_HORIZONTAL_BAR, 1,3,
    CH_HUB, 2,3,
    CH_OUTLET, 2,4,


    // LINE + CH_LADDER_0, 4,2,4,10,

    // FILLRECT+CH_ROCK,5,5,21,10,CH_DIRT,

    // #define VS 3
    // #define HS 3

    // //    0xFE,CH_PUSH_DOWN,7,1,
    //   //  0xFE,CH_PUSH_DOWN,9,1,
    //     //0xFE,CH_PUSH_DOWN,11,1,
    //     //0xFE,CH_PUSH_DOWN,13,1,

    // //    0xFE,CH_PUSH_DOWN,7,1,
    //   //  0xFE,CH_PUSH_DOWN,9,1,
    //     //0xFE,CH_PUSH_DOWN,11,1,
    //     //0xFE,CH_PUSH_DOWN,13,1,

    // //    0xFE,CH_PUSH_DOWN,7,1,
    //   //  0xFE,CH_PUSH_DOWN,9,1,
    //     //0xFE,CH_PUSH_DOWN,11,1,
    //     //0xFE,CH_PUSH_DOWN,13,1,

    // //    0xFE, CH_HUB, 2+HS*3,5+VS*2,


    // LINE + CH_BRICKWALL,1,7,2,31,
    // LINE + CH_BRICKWALL,8,14,2,31,

    // LINE+CH_BLANK,30,1,4,6,

    // LINE+CH_BLANK,10,1,4,6,

    // LINE+CH_BLANK,5,8,4,7,

    // LINE+CH_BLANK,30,15,4,5,

    CH_DOORCLOSED, 38, 16,
    CH_MELLON_HUSK_BIRTH, 4, 2,


    CH_WATER, 1, 20,


    DRAW_EOF,

    // EXTRAS
    // LEVEL 0
    DRAW_EOF, // LEVEL 1
    DRAW_EOF, // LEVEL 2
    DRAW_EOF, // LEVEL 3
    DRAW_EOF, // LEVEL 4
    DRAW_EOF,

    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


const unsigned char caveA2[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,

    20,     // milling
    10, 15, // doge $
    5,      //              ,          // rain

    
    10, 11, 50, 56, 8, // randomiser[level]
    25, 12, 12, 12, 12, 200, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_MACE,                    //0
    WEAPON_MACE,                    //1
    WEAPON_NONE,                    //2
    WEAPON_NONE,                    //3
    WEAPON_NONE,                    //4

    0, STEEL, CH_DIRT,

    2,
    CH_BLANK, 100, 10, 5, 0, 20,
    CH_GEODOGE, 50, 10, 5, 0, 20,


    //0x80 + CH_STEELWALL, 10, 5, 20, 12, CH_DIRT,

    CH_PUSH_DOWN,6,9,
    CH_PUSH_RIGHT,7,8,
    CH_PUSH_LEFT,5,8,
    CH_PUSH_UP,6,7,
    CH_HUB, 6,8,

    CH_LAVA_BLANK, 1, 20,

    CH_MELLON_HUSK_BIRTH, 8, 6,

    CH_WYRM_HEAD_U, 11, 6,
    CH_WYRM_HEAD_U, 12, 6,
    CH_WYRM_HEAD_U, 13, 6,
    CH_WYRM_HEAD_U, 14, 6,
//    0xFF,

    CH_GRINDER_0, 4, 9,
    CH_GRINDER_0, 5, 8,
    CH_GRINDER_0, 6, 6,
    CH_GRINDER_0, 7, 6,
    CH_GRINDER_0, 8, 5,

    CH_GRINDER_0, 8, 9,
    CH_GRINDER_1, 9, 9,
    CH_GRINDER_0, 9, 8,
    CH_GRINDER_1, 10, 8,
    CH_GRINDER_0, 10, 7,

    CH_GRINDER_1, 11, 12,
    CH_BELT_0, 12, 12,
    CH_BELT_1, 13, 12,
    CH_GRINDER_1, 14, 12,
    CH_BELT_1, 15, 12,
    CH_BELT_0, 16, 12,
    CH_GRINDER_1, 17, 12,
    CH_BELT_1, 18, 12,
    CH_BELT_0, 19, 12,
    CH_GRINDER_1, 20, 12,

    CH_HUB_1, 20, 8,
    CH_PUSH_DOWN, 20, 9,

    CH_GRINDER_0, 15, 15,
    CH_BELT_0, 16, 15,
    CH_BELT_1, 17, 15,
    CH_BELT_0, 18, 15,
    CH_BELT_1, 19, 15,
    CH_BELT_0, 20, 15,
    CH_BELT_1, 21, 15,
    CH_GRINDER_0, 22, 15,


    // EXTRAS
    // LEVEL 0
    DRAW_EOF, // LEVEL 1
    DRAW_EOF, // LEVEL 2
    DRAW_EOF, // LEVEL 3
    DRAW_EOF, // LEVEL 4
    DRAW_EOF,

    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


const unsigned char caveA5[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,

    20,     // milling
    10, 15, // doge $
    5,      //              ,          // rain

    
    10, 11, 50, 56, 8, // randomiser[level]
    25, 12, 12, 12, 12, 200, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_NONE,                    //0
    WEAPON_NONE,                    //1
    WEAPON_NONE,                    //2
    WEAPON_NONE,                    //3
    WEAPON_NONE,                    //4

    0, STEEL, CH_BLANK,

    0,

    CH_MELLON_HUSK_BIRTH, 7, 6,


    // EXTRAS
    // LEVEL 0
    DRAW_EOF, // LEVEL 1
    DRAW_EOF, // LEVEL 2
    DRAW_EOF, // LEVEL 3
    DRAW_EOF, // LEVEL 4
    DRAW_EOF,

    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


const unsigned char caveA3[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,

    20,    // milling
    1, 15, // doge $
    5,     //              ,          // rain

    10, 11, 50, 56, 8, // randomiser[level]
    8, 8, 8, 8, 8, 20, 200, 200, 200, 200,
    // 70,65,60,55,50,

    WEAPON_NONE,                    //0
    WEAPON_NONE,                    //1
    WEAPON_NONE,                    //2
    WEAPON_NONE,                    //3
    WEAPON_NONE,                    //4

    0, STEEL, CH_BLANK,

    0,

    DRAW_FILLED_RECT, CH_STEELWALL, 10, 5, 20, 5, CH_BLANK,

    LINER(CH_DOGE_00, 13, 8, 9, R)
    CH_DOGE_00, 16, 8, 0xFE, CH_ROCK, 1, 6, 0xFE, CH_STEELWALL, 12, 7,


    CH_DOORCLOSED, 16, 5,
    CH_BLOCK, 16, 6,
    CH_BLOCK, 16, 7,
    CH_ROCK, 16, 9,

    CH_ROCK, 16, 9,
    CH_ROCK, 16, 10,
    CH_ROCK, 16, 11,
    CH_ROCK, 16, 12,
    CH_ROCK, 16, 13,
    CH_ROCK, 16, 14,
    CH_ROCK, 16, 15,
    CH_ROCK, 16, 16,
    CH_ROCK, 16, 17,
    CH_ROCK, 16, 18,
    
    CH_FLIP_GRAVITY_0, 16, 18,
    CH_FLIP_GRAVITY_0, 16, 19,
    CH_BLOCK, 16, 20,

    CH_FLIP_GRAVITY_0, 16, 8,

    /*
        0xFE, CH_PUSH_DOWN, 11, 6,
        0xFE, CH_PUSH_DOWN, 15, 6,
        0xFE, CH_PUSH_DOWN, 19, 6,
        0xFE, CH_PUSH_DOWN, 23, 6,


        0xFE, CH_PUSH_UP, 11, 12,
        0xFE, CH_PUSH_UP, 15, 12,
        0xFE, CH_PUSH_UP, 19, 12,
        0xFE, CH_PUSH_UP, 23, 12,
    */

    CH_MELLON_HUSK_BIRTH, 11, 7,


    DRAW_EOF,

    // EXTRAS
    // LEVEL 0
    DRAW_EOF, // LEVEL 1
    DRAW_EOF, // LEVEL 2
    DRAW_EOF, // LEVEL 3
    DRAW_EOF, // LEVEL 4
    DRAW_EOF,

    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


const unsigned char starsAndStripes[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    7,17,7,17,

    20,                             // milling
    1, 15,                          // doge $
    0,                              // rain
    
    10,                             //0
    11,                             //1
    50,                             //2
    56,                             //3
    8,                              //4 randomiser[level]

    15,                             //0
    15,                             //1
    10,                             //2
    10,                             //3
    15,                             //4
    
    20,                             //0
    200,                            //1
    200,                            //2
    200,                            //3
    200,                            //4

    WEAPON_NONE,                    //0
    WEAPON_NONE,                    //1
    WEAPON_MACE,                    //2
    WEAPON_NONE,                    //3
    WEAPON_NONE,                    //4

    CAVEDEF_START_WITH_WEAPON|CAVEDEF_STAR_STATIC|CAVEDEF_BONUS, CH_BRICKWALL, CH_DIRT,

    0,
    DRAW_FILLED_RECT, CH_BRICKWALL, 1,1,9,8, CH_DOGE_00,
    DRAW_FILLED_RECT, 0x80|CH_STAR, 2, 2, 7, 6, 8,


    DRAW_EOF,


    // EXTRAS

    // -------------------------------------------------------
    // LEVEL 0
    CH_MELLON_HUSK_BIRTH, 2, 2,
    DRAW_EOF,
    
    // -------------------------------------------------------
    // LEVEL 1

    DRAW_FILLED_RECT, 0x80|CH_ROCK, 2, 2, 7, 6, 8,
    CH_MELLON_HUSK_BIRTH, 4, 2,
    DRAW_EOF,
    
    // -------------------------------------------------------
    // LEVEL 2
    DRAW_FILLED_RECT, CH_BLANK, 2, 2, 7, 6,CH_BLANK,

    CH_ROCK,2,2,
    CH_ROCK,8,2,
    LINER(CH_STAR,3,2,5,2)
    CH_DIRT,2,3,CH_DIRT,8,3,

    CH_ROCK,3,3,
    CH_ROCK,7,3,
    LINER(CH_DOGE_00,4,3,3,2)
    CH_DIRT,3,4,CH_DIRT,7,4,

    CH_ROCK,4,4,
    CH_ROCK,6,4,
    CH_DOGE_00,5,4,
    CH_GEODOGE,4,5,CH_GEODOGE,6,5,
    
    CH_ROCK,5,5,

    //   DRAW_FILLED_RECT, CH_STAR, 2, 2, 7, 6,
//    DRAW_FILLED_RECT, CH_BLANK, 3, 3, 4, 3,
    CH_MELLON_HUSK_BIRTH, 5,6,
    LINER(CH_BLANK, 2,7,7,2)
    CH_DOORCLOSED, 5,7,
    DRAW_EOF,
    
    // -------------------------------------------------------
    // LEVEL 3

    LINER(CH_STAR,2,2,5,2)
    DRAW_FILLED_RECT,CH_ROCK, 2,3,7,2, CH_ROCK,
    CH_MELLON_HUSK_BIRTH, 2,6,
    CH_DOORCLOSED, 5,2,

    DRAW_EOF,
    
    // -------------------------------------------------------
    // LEVEL 4


    CH_MELLON_HUSK_BIRTH, 2, 2,

    DRAW_EOF,


    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


const unsigned char caveA4[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    43,17,BOARD_TRIX_X-SCREEN_TRIX_X-53,17,

    20,    // milling
    1, 15, // doge $
    0,     //              ,          // rain
    
    10,                             //0
    11,                             //1
    50,                             //2
    56,                             //3
    8,                              //4 randomiser[level]

    15,                             //0
    15,                             //1
    15,                             //2
    15,                             //3
    15,                             //4
    
    20,                             //0
    200,                            //1
    200,                            //2
    200,                            //3
    200,                            //4

    WEAPON_NONE,                    //0
    WEAPON_NONE,                    //1
    WEAPON_NONE,                    //2
    WEAPON_NONE,                    //3
    WEAPON_NONE,                    //4

    //CAVEDEF_LOCK_Y,
     0, CH_DIRT, CH_DIRT,

    2,
    CH_GEODOGE, 100, 100, 100, 100, 100,
    CH_BLANK, 50, 10, 5, 0, 20,
    CH_ROCK, 120, 10, 5, 0, 20,

    LINER(CH_STEELWALL, 0, 21, 40, 2)   // protective catch-all wall bottom

    DRAW_RECT, CH_BRICKWALL, 8, 1, 22, 8,
    DRAW_FILLED_RECT, CH_DIRT, 9, 2, 20, 6,CH_DIRT,
    DRAW_FILLED_RECT, 0x80|CH_BLANK, 9, 2, 18, 5, 2,
    DRAW_FILLED_RECT, 0x80|CH_ROCK, 9,2, 20, 6, 20,
    DRAW_FILLED_RECT, 0x80|CH_GEODOGE, 9, 2, 18, 5, 6,
    DRAW_FILLED_RECT, 0x80|CH_ROCK_BONUS, 9, 2, 18, 5, 26,

    // 0xFE,
    CH_INSULATOR_BOTTOM,12,7,
    CH_INSULATOR_TOP,12,2,

    CH_STAR, 13,6,

    CH_STEELWALL, 14, 5,
    CH_STEELWALL, 14, 6,


    CH_DOORCLOSED, 16, 3,

    CH_MELLON_HUSK_BIRTH, 11, 4,


    CH_INSULATOR_BOTTOM,18,7,
    CH_INSULATOR_TOP,18,2,
 
    CH_INSULATOR_BOTTOM,24,7,
    CH_INSULATOR_TOP,24,2,

    DRAW_EOF,

    // EXTRAS
    // LEVEL 0
    DRAW_EOF, // LEVEL 1
    DRAW_EOF, // LEVEL 2
    DRAW_EOF, // LEVEL 3
    DRAW_EOF, // LEVEL 4
    DRAW_EOF,

    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


void none() {
}

void spec() {
}


void empty() {
}


const struct caveHandler caveList[] = {

    // cave definition, condition handler
    {caveUseWall, none},
    {caveA4, none},
    {caveWyrms, empty},
    {starsAndStripes, spec},
};

const int caveCount = sizeof(caveList) / sizeof(caveList[0]);

// EOF
