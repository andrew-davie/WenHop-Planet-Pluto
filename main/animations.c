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
    CH_BLANK, 10,
    CH_BOMB, 9,
    CH_BLANK, 9,
    CH_BOMB, 8,
    CH_BLANK, 8,
    CH_BOMB, 7,
    CH_BLANK, 7,
    CH_BOMB, 6,
    CH_BLANK, 6,
    CH_BOMB, 5,
    CH_BLANK, 5,
    CH_BOMB, 4,
    CH_BLANK, 4,
    CH_BOMB, 3,
    CH_BLANK, 3,
    CH_BOMB, 2,
    CH_BLANK, 2,
    CH_BOMB, 1,
    CH_BLANK, 1,

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

// The door splitting open down its centre seam, both halves retracting toward the outer edges
// (not sliding off to one side) -- CH_DOORSLIDE_1 opens just the centre column, CH_DOOROPEN_STATIC
// widens that to the two centre columns either side, leaving a door-post column on each edge so
// it never reads as blank. A symmetric split on a glyph only 5px wide has room for exactly this
// one step in between without running out of edge to keep: one more widening would consume the
// last two door-post columns and leave nothing recognisably door-shaped.
//
// Frame 0 (closed, delay 0 -- holds forever) is AnimateBase[TYPE_DOOR], so every TYPE_DOOR cell
// idles here by default from level load. The actual "open" trigger is
// startCharAnimation(TYPE_DOOR, AnimateDoor + 2), which skips straight past frame 0 into the
// split -- same pattern as AnimateBomb's idle-vs-triggered split above. No ANIM_LOOP: the final
// frame's delay-0 holds it at CH_DOOROPEN_STATIC -- deliberately NOT CH_DOOROPEN_0, which would
// pull in TYPE_OUTBOX's AnimFlashOut the moment the board write below lands (CharToType[] keys
// off the raw byte); CH_DOOROPEN_STATIC is the same graphic under TYPE_DOOR_OPEN, which has no
// animation, so it just stays put instead of flickering to blank every 20 frames forever. A real
// board write is still needed to actually make the tile walkable, see updateDoorUnlock() and
// board.c's CH_DOORCLOSED case, both of which time their commit to land after this finishes.
//
// Shared per-type like every other AnimateBase entry (see this table's own "animate in unison"
// warning) -- if a level has more than one closed door, they all split open together the moment
// any single one is triggered, not just the one actually interacted with.
const unsigned char AnimateDoor[] = {

    CH_DOORCLOSED, 0,

    CH_DOORSLIDE_1, 14,
    CH_DOOROPEN_STATIC, 0,
};

// The reverse of AnimateDoor above -- used two ways: (1) the exit door (mellon.c's exit
// trigger, via TYPE_OUTBOX) sliding shut once the player has faded almost to black
// (updatePlayerAnimation(), playerAnimation.c), and (2) any key/doge-unlocked door
// (TYPE_DOOR_OPEN) sliding shut again the instant the player steps onto a teleport
// (mellon.c's TYPE_TELEPORT trigger, board.c's CH_DOOROPEN_STATIC case commits the board
// write once this finishes) -- so a returning player sees why it's locked again instead of
// just finding it that way. Same CH_DOORSLIDE_1 crack frame both directions -- it's
// symmetric, so it works for either the door opening or closing through it. Frame 0 here is
// just a landing pad matching each type's already-open idle state, never actually shown --
// TYPE_OUTBOX's own base is AnimFlashOut instead, so it's only reached via
// startCharAnimation(TYPE_OUTBOX, AnimateDoorClose + 2); TYPE_DOOR_OPEN's AnimateBase entry
// IS this table directly (see its own comment, above), since unlike TYPE_OUTBOX it has no
// other idle animation of its own to fall back to between triggers.
//
// CH_DOORSLIDE_1's hold here is intentionally short -- even shorter than AnimateDoor's own
// opening hold -- there's only this one static crack frame between open and closed (a symmetric
// split on a glyph this narrow doesn't have room for more without going fully blank, see
// CH_DOORSLIDE_1's own comment), so holding it for longer doesn't read as a slow slide, it reads
// as the animation freezing on a single unmoving frame for that whole stretch, then snapping
// straight to closed. A short hold at least keeps it feeling like one continuous motion.
const unsigned char AnimateDoorClose[] = {

    CH_DOOROPEN_STATIC, 0,

    CH_DOORSLIDE_1, 8,
    CH_DOORCLOSED, 0,
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


// A 3-spoke pinwheel, 8 rotations 15 degrees apart (characterset.c has the full geometry
// rationale). Entirely owned by driveTeleportSpin() below, NOT by processCharAnimations()'s
// usual auto-advance -- deliberately not wired into AnimateBase[] (see its own entry's comment).
// Durations here are ANIM_HALT and unused: driveTeleportSpin() paces itself, independent of
// whatever's in this table, and having AnimateBase[TYPE_TELEPORT] be 0 means
// processCharAnimations() skips this type outright, so there's no second driver left that could
// race it. (An earlier version wired this into AnimateBase[] so the idle case could ride the
// normal mechanism "for free" -- but ANIM_LOOP always resolves through AnimateBase[type], a
// single fixed pointer, so that auto-advance never actually stopped even while
// driveTeleportSpin() was separately forcing the fast cadence: both were writing
// Animate[TYPE_TELEPORT]/AnimCount[TYPE_TELEPORT] every frame, and the two drifted out of sync
// until the untouched auto-advance's slow 3-tick cadence became the one you could see.)
const unsigned char AnimTeleport[] = {
    CH_TELEPORT, ANIM_HALT,
    CH_TELEPORT_1, ANIM_HALT,
    CH_TELEPORT_2, ANIM_HALT,
    CH_TELEPORT_3, ANIM_HALT,
    CH_TELEPORT_4, ANIM_HALT,
    CH_TELEPORT_5, ANIM_HALT,
    CH_TELEPORT_6, ANIM_HALT,
    CH_TELEPORT_7, ANIM_HALT,
};

// clang-format on

// Sole driver of TYPE_TELEPORT's animation, every frame, both speeds -- see AnimTeleport's own
// comment for why processCharAnimations()/AnimateBase[] are deliberately kept out of this
// entirely. fast is gameState_Game.c's teleportLocked -- true from the instant the player steps
// onto the tile until initPlayer() clears it once the destination cave loads.
#define TELEPORT_IDLE_HOLD_FRAMES 4    // "3 frame delay" idle cadence
#define TELEPORT_FAST_HOLD_FRAMES 1    // one tick slower than every frame, which read as too fast

void driveTeleportSpin(bool fast) {

    static unsigned int frame;
    static int hold;

    if (hold) {
        hold--;
        return;
    }

    hold = (fast ? TELEPORT_FAST_HOLD_FRAMES : TELEPORT_IDLE_HOLD_FRAMES) - 1;

    startCharAnimation(TYPE_TELEPORT, AnimTeleport + 2 * frame);

    if (++frame >= 8)
        frame = 0;
}

const unsigned char *const AnimateBase[] = {

    // indexed by ObjectType (attribute.h); 0 = no auto-animation

    // SUPER CRITICAL:  *ALL* characters of the given type will animate in unison.
    //  You *CANNOT* use this to animate a just single character onscreen.

    // Type numbers are IDs, not sequential — conditional compilation can break continuity.

    0,                      // 00 TYPE_BLANK
    0,                      // 01 TYPE_PLACEHOLDER
    0,                      // 02 TYPE_DIRT
    0,                      // 03 TYPE_BRICKWALL
    AnimateDoor,            // 04 TYPE_DOOR
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
    AnimateBomb,            // 49 TYPE_BOMB
    AnimateCrackedBrick,    // 52 TYPE_CRACKED_BRICK
    0,                      // 53 TYPE_CONCRETE
    0,                      // 54 TYPE_TELEPORT -- driveTeleportSpin() owns this one entirely
                            // (below); deliberately not wired in here, see its own comment
    0,                      // 55 TYPE_KEY -- no animation
    AnimateDoorClose,       // 56 TYPE_DOOR_OPEN -- idle landing pad only (frame 0, holds forever,
                            // same as TYPE_OUTBOX uses it); needs a real AnimateBase entry (not 0)
                            // so processCharAnimations() actually services this type at all -- see
                            // board.c's teleport-departure trigger and CH_DOOROPEN_STATIC case
    0,                      // 57 TYPE_IMMOVABLE -- no animation, static like TYPE_ROCK
    0,                      // 58 TYPE_IMMOVABLE_FALLING
    0,                      // 59 TYPE_ROCK_ROLLING -- no animation, same as TYPE_DOGE_FALLING2
};

_Static_assert(sizeof(AnimateBase) / sizeof(AnimateBase[0]) == TYPE_MAX, "AnimateBase table wrong size");

const unsigned char PickupCharacter[] = {

    0,                // 00 TYPE_BLANK
    0,                // 01 TYPE_PLACEHOLDER
    0,                // 02 TYPE_DIRT
    0,                // 03 TYPE_BRICKWALL
    CH_DOORCLOSED,    // 04 TYPE_DOOR
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
    CH_BOMB,          // 49 TYPE_BOMB
    0,                // 52 TYPE_CRACKED_BRICK
    0,                // 53 TYPE_CONCRETE
    0,                // 54 TYPE_TELEPORT
    0,                // 55 TYPE_KEY -- auto-grabbed by walking onto it (mellon.c), not fire-button-yankable
    0,                // 56 TYPE_DOOR_OPEN
    0,                // 57 TYPE_IMMOVABLE -- can't be fire-button picked up, per its own name
    0,                // 58 TYPE_IMMOVABLE_FALLING
    0,                // 59 TYPE_ROCK_ROLLING -- transitional only, never fire-button-yankable

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

    }
}


void haltCharAnimation(int type) {
    AnimCount[type] = ANIM_HALT;
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