#include "animations.h"
#include "attribute.h"


const unsigned char *Animate[TYPE_MAX];
static char AnimCount[TYPE_MAX];

// clang-format off


const unsigned char AnimateCrackedBrick[] = {

    CH_CRACKED_BRICK, 0,
    CH_CRACKED_BRICK_1, 0,
    CH_CRACKED_BRICK_2, 0,
    CH_CRACKED_BRICK_3, 0,
    CH_CRACKED_BRICK_4, 0,
    CH_CRACKED_BRICK_5, 0,
    CH_CRACKED_BRICK_6, 0,
    CH_CRACKED_BRICK_7, 0,
};



const unsigned char AnimateBomb[] = {

    CH_BOMB, 0,

    CH_BOMB, 10,
    CH_BOMB_FLASH, 10,
    CH_BOMB, 9,
    CH_BOMB_FLASH, 9,
    CH_BOMB, 8,
    CH_BOMB_FLASH, 8,
    CH_BOMB, 7,
    CH_BOMB_FLASH, 7,
    CH_BOMB, 6,
    CH_BOMB_FLASH, 6,
    CH_BOMB, 5,
    CH_BOMB_FLASH, 5,
    CH_BOMB, 4,
    CH_BOMB_FLASH, 4,
    CH_BOMB, 3,
    CH_BOMB_FLASH, 3,
    CH_BOMB, 2,
    CH_BOMB_FLASH, 2,
    CH_BOMB, 1,
    CH_BOMB_FLASH, 1,

    CH_BLANK, 0,

    ANIM_LOOP,

};




const unsigned char AnimateStar[] = {

    CH_STAR, 21,
    CH_DOGE_04, 9,
    CH_BLANK, 6,
    ANIM_LOOP,

    CH_STAR,0,
};

const unsigned char AnimatePitL[] = {
    CH_PIT_L0,3,
    CH_PIT_L1,3,
    CH_PIT_L2,3,
    CH_PIT_L3,3,
    CH_PIT_L4,3,

    ANIM_LOOP
};

const unsigned char AnimatePitR[] = {
    CH_PIT_R0,3,
    CH_PIT_R1,3,
    CH_PIT_R2,3,
    CH_PIT_R3,3,
    CH_PIT_R4,3,

    ANIM_LOOP
};


const unsigned char AnimateStarExplode[] = {

    CH_STAR,12,
    CH_DOGE_04,6,
    CH_BLANK,3,
    CH_DOGE_04,3,

    CH_STAR,12,
    CH_DOGE_04,6,
    CH_BLANK,3,
    CH_DOGE_04,3,
    CH_STAR,12,
    CH_DOGE_04,6,
    CH_BLANK,3,
    CH_DOGE_04,3,


    CH_STAR,12,
    CH_DOGE_04,6,
    CH_BLANK,3,
    CH_DOGE_04,3,
    CH_STAR,12,
    CH_DOGE_04,3,
    CH_DUST_0,0,
};


const unsigned char AnimateRockBonus[] = {

    CH_ROCK_BONUS,0,
    ANIM_LOOP,
};


const unsigned char AnimateBelt[] = {
    CH_BELT_0, 12,
    CH_BELT_1, 12,
    ANIM_LOOP,
};

const unsigned char AnimatePebbleToGeoDoge[] = {

    CH_PEBBLE_ROCK, 3,
    CH_DIRT, 3,
    ANIM_LOOP
};

const unsigned char AnimateRockPebble[] = {

    CH_ROCK_PEBBLE, 6,
    CH_GEODOGE, 3,
    ANIM_LOOP
};




const unsigned char AnimateBelt1[] = {
    CH_BELT_1, 12,
    CH_BELT_0, 12,
    ANIM_LOOP,
};


#define TRICKLE 5

const unsigned char AnimateWaterFlow0[] = {

    CH_WATERFLOW_4, TRICKLE,
    CH_WATERFLOW_3, TRICKLE,
    CH_WATERFLOW_2, TRICKLE,
    CH_WATERFLOW_1, TRICKLE,
    CH_WATERFLOW_0, TRICKLE,
    ANIM_LOOP,
};

const unsigned char AnimateWaterFlow1[] = {

    CH_WATERFLOW_3, TRICKLE,
    CH_WATERFLOW_2, TRICKLE,
    CH_WATERFLOW_1, TRICKLE,
    CH_WATERFLOW_0, TRICKLE,
    CH_WATERFLOW_4, TRICKLE,
    ANIM_LOOP,
};

const unsigned char AnimateWaterFlow2[] = {

    CH_WATERFLOW_2, TRICKLE,
    CH_WATERFLOW_1, TRICKLE,
    CH_WATERFLOW_0, TRICKLE,
    CH_WATERFLOW_4, TRICKLE,
    CH_WATERFLOW_3, TRICKLE,
    ANIM_LOOP,
};

const unsigned char AnimateWaterFlow3[] = {

    CH_WATERFLOW_1, TRICKLE,
    CH_WATERFLOW_0, TRICKLE,
    CH_WATERFLOW_4, TRICKLE,
    CH_WATERFLOW_3, TRICKLE,
    CH_WATERFLOW_2, TRICKLE,
    ANIM_LOOP,
};

const unsigned char AnimateWaterFlow4[] = {

    CH_WATERFLOW_0, TRICKLE,
    CH_WATERFLOW_4, TRICKLE,
    CH_WATERFLOW_3, TRICKLE,
    CH_WATERFLOW_2, TRICKLE,
    CH_WATERFLOW_1, TRICKLE,
    ANIM_LOOP,
};


const unsigned char AnimateGrinder[] = {

    CH_GRINDER_0, 12,
    CH_GRINDER_1, 12,
    ANIM_LOOP,

    CH_GRINDER_0, 0
};

const unsigned char AnimateGrinder1[] = {

    CH_GRINDER_1, 12,
    CH_GRINDER_0, 12,
    ANIM_LOOP,

    CH_GRINDER_1, 0
};


const unsigned char AnimateGravity[] = {

    CH_FLIP_GRAVITY_2, 30,
    CH_FLIP_GRAVITY_1, 4,
    CH_FLIP_GRAVITY_0, 4,
    CH_FLIP_GRAVITY_1, 4,
    ANIM_LOOP,
};


const unsigned char AnimFlashOut[] = {

    CH_DOOROPEN_0,20,
    CH_BLANK,20,
    ANIM_LOOP
};

const unsigned char AnimPulseDoge[] = {

    CH_DOGE_00, 12, //ANIM_RNDSPEED,
    CH_DOGE_01, 8,
    CH_DOGE_02, 5,
    CH_DOGE_03, 4,
    CH_DOGE_04, 5,
    CH_DOGE_05, 8,

    CH_DOGE_04, 5,
    CH_DOGE_03, 4,
    CH_DOGE_02, 5,
    CH_DOGE_01, 8,

    ANIM_LOOP
};


const unsigned char AnimMellonHusk[] = {

    // Note that mellon.c indexes into this with an offset so this must be kept synched

    // CH_DUST_ROCK_0, 12,
    // CH_DUST_ROCK_1, 6,
    // CH_DUST_ROCK_2, 6,

    CH_DIRT, 5,
    CH_DUST_0,6,
    // CH_BROKEN_DIRT, 3,
    // CH_BROKEN_DIRT, 3,
    CH_DUST_1, 6,
    CH_DUST_2, 6,

    CH_BLANK, ANIM_HALT,

    // @+2
    // CH_DOGE_GRAB,8,
    // CH_MELLON_HUSK, ANIM_HALT,

    // @+2
};

// clang-format on

const unsigned char *const AnimateBase[] = {

    // indexed by object TYPE (def: ObjectType in attribute.h)
    // =0 if object does not auto-animate

    // SUPER CRITICAL:  *ALL* characters of the given type will animate in unison.
    //  You *CANNOT* use this to animate a just single character onscreen.

    // Note that the type number is an ID, not ordinal. That's because the continuity may
    // be compromised by the conditional compilation. Beware.

    0,                      // 00 TYPE_BLANK
    0,                      // 01 TYPE_PLACEHOLDER
    0,                      // 02 TYPE_DIRT
    0,                      // 03 TYPE_BRICKWALL
    0,                      // 04 TYPE_OUTBOX_PRE
    AnimFlashOut,           // 05 TYPE_OUTBOX
    0,                      // 06 TYPE_STEELWALL
    0,                      // 07 TYPE_ROCK
    AnimPulseDoge,          // 08 TYPE_DOGE
    0,                      // 09 TYPE_MELLON_HUSK_PRE
    AnimMellonHusk,         // 10 TYPE_MELLON_HUSK
    0,                      // 11 TYPE_PEBBLE1
    0,                      // 12 TYPE_DUST_0
    0,                      // 13 TYPE_DOGE_FALLING
    0,                      // 14 TYPE_ROCK_FALLING
    0,                      // 15 TYPE_DUST_ROCK
    0,                      // 16 TYPE_CONVERT_GEODE_TO_DOGE
    0,                      // 17 TYPE_PUSHER
    0,                      // 18 TYPE_PUSHER_VERT
    0,                      // 19 TYPE_WYRM
    0,                      // 20 TYPE_GEODOGE
    0,                      // 21 TYPE_GEODOGE_FALLING
    0,                      // 22 TYPE_LAVA
    0,                      // 23 TYPE_PEBBLE_ROCK    (pebble to geodoge)
    AnimateGravity,         // 24 TYPE_FLIP_GRAVITY
    0,                      // 25 TYPE_BLOCK
    AnimateGrinder,         // 26 TYPE_GRINDER
    0,                      // 27 TYPE_HUB
    0,                      // 28 TYPE_WATER
    AnimateWaterFlow0,      // 29 TYPE_WATERFLOW0
    AnimateWaterFlow1,      // 30 TYPE_WATERFLOW1
    AnimateWaterFlow2,      // 31 TYPE_WATERFLOW2
    AnimateWaterFlow3,      // 32 TYPE_WATERFLOW3
    AnimateWaterFlow4,      // 33 TYPE_WATERFLOW4
    0,                      // 34 TYPE_TAP
    0,                      // 35 TYPE_OUTLET
    AnimateGrinder1,        // 36 TYPE_GRINDER1
    AnimateBelt,            // 37 TYPE_BELT
    AnimateBelt1,           // 38 TYPE_BELT1
    0,                      // 39 TYPE_CONVERT_PIPE
    0,                      // 40 TYPE_DOGE_FALLING2
    AnimateRockPebble,      // 41 TYPE_ROCK_PEBBLE (geodoge disintegrating)
    0,                      // 42 TYPE_ELECTRIC_0
    0,                      // 43 TYPE_INSULATOR
    AnimateStar,            // 44 TYPE_STAR
    0,                      // 45 TYPE_STAR_FALLING
    AnimateStarExplode,     // 46 TYPE_STAR_EXPLODE
    AnimateRockBonus,       // 47 TYPE_ROCK_BONUS
    0,                      // 48 TYPE_MOUNT
    AnimatePitL,            // 49 TYPE_PIT_L
    AnimatePitR,            // 50 TYPE_PIT_R
    AnimateBomb,            // 51 TYPE_BOMB
    AnimateCrackedBrick,    // 52 TYPE_CRACKED_BRICK
    0,                      // 53 TYPE_CONCRETE
};

_Static_assert(sizeof(AnimateBase) / sizeof(AnimateBase[0]) == TYPE_MAX, "AnimateBase table wrong size");

const unsigned char PickupCharacter[] = {

    0,                // 00 TYPE_BLANK
    0,                // 01 TYPE_PLACEHOLDER
    0,                // 02 TYPE_DIRT
    0,                // 03 TYPE_BRICKWALL
    CH_DOORCLOSED,    // 04 TYPE_OUTBOX_PRE
    CH_DOOROPEN_0,    // 05 TYPE_OUTBOX
    0,                // 06 TYPE_STEELWALL
    CH_ROCK,          // 07 TYPE_ROCK
    CH_DOGE_00,       // 08 TYPE_DOGE
    0,                // 09 TYPE_MELLON_HUSK_PRE
    0,                // 10 TYPE_MELLON_HUSK
    0,                // 11 TYPE_PEBBLE1
    0,                // 12 TYPE_DUST_0
    0,                // 13 TYPE_DOGE_FALLING
    0,                // 14 TYPE_ROCK_FALLING
    0,                // 15 TYPE_DUST_ROCK
    0,                // 16 TYPE_CONVERT_GEODE_TO_DOGE
    0,                // 17 TYPE_PUSHER
    0,                // 18 TYPE_PUSHER_VERT
    0,                // 19 TYPE_WYRM
    CH_GEODOGE,       // 20 TYPE_GEODOGE
    0,                // 21 TYPE_GEODOGE_FALLING
    0,                // 22 TYPE_LAVA
    0,                // 23 TYPE_PEBBLE_ROCK    (pebble to geodoge)
    0,                // 24 TYPE_FLIP_GRAVITY
    0,                // 25 TYPE_BLOCK
    0,                // 26 TYPE_GRINDER
    0,                // 27 TYPE_HUB
    0,                // 28 TYPE_WATER
    0,                // 29 TYPE_WATERFLOW0
    0,                // 30 TYPE_WATERFLOW1
    0,                // 31 TYPE_WATERFLOW2
    0,                // 32 TYPE_WATERFLOW3
    0,                // 33 TYPE_WATERFLOW4
    0,                // 34 TYPE_TAP
    0,                // 35 TYPE_OUTLET
    CH_GRINDER_0,     // 36 TYPE_GRINDER1
    0,                // 37 TYPE_BELT
    0,                // 38 TYPE_BELT1
    0,                // 39 TYPE_CONVERT_PIPE
    0,                // 40 TYPE_DOGE_FALLING2
    0,                // 41 TYPE_ROCK_PEBBLE (geodoge disintegrating)
    0,                // 42 TYPE_ELECTRIC_0
    0,                // 43 TYPE_INSULATOR
    CH_STAR,          // 44 TYPE_STAR
    0,                // 45 TYPE_STAR_FALLING
    0,                // 46 TYPE_STAR_EXPLODE
    CH_ROCK_BONUS,    // 47 TYPE_ROCK_BONUS
    0,                // 48 TYPE_MOUNT
    0,                // 49 TYPE_PIT_L
    0,                // 50 TYPE_PIT_R
    CH_BOMB,          // 51 TYPE_BOMB
    0,                // 52 TYPE_CRACKED_BRICK
    0,                // 53 TYPE_CONCRETE

};

_Static_assert(sizeof(PickupCharacter) / sizeof(PickupCharacter[0]) == TYPE_MAX, "PickupCharacter table wrong size");


void initCharAnimations() {

    for (int type = 0; type < TYPE_MAX; type++)
        startCharAnimation(type, AnimateBase[type]);
}

void startCharAnimation(int type, const unsigned char *idx) {

    if (idx) {

        if ((int)*idx == ANIM_LOOP)
            idx = AnimateBase[type];

        Animate[type] = idx++;
        AnimCount[type] = *idx;

        // extern int lastRockCount;
        // int speed = 8 - lastRockCount + 1;
        // if (speed < 1)
        //     speed = 1;
        // if (type == TYPE_PIT_L || type == TYPE_PIT_R)
        //     AnimCount[type] = speed;
    }
}


void processCharAnimations() {

    for (int type = 0; type < TYPE_MAX; type++)
        if (AnimateBase[type] && AnimCount[type] != ANIM_HALT)
            if (!--AnimCount[type])
                startCharAnimation(type, Animate[type] + 2);
}


void toggleGears(bool active) {

    startCharAnimation(TYPE_GRINDER, AnimateGrinder + (active ? 5 : 0));
    startCharAnimation(TYPE_GRINDER_1, AnimateGrinder1 + (active ? 5 : 0));
}

// EOF