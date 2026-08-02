#pragma once

struct caveHandler {
    const unsigned char *cave;
    void (*const handler)();

    // Where a teleport arriving in this cave lands -- board cell (col, row), same
    // coordinate space as CH_MELLON_HUSK_BIRTH/CH_TELEPORT placements in the cave data
    // itself. Hand-set per cave in caveList[] below. {0, 0} means this cave doesn't
    // support being a teleport destination at all -- (0,0) is always the border/corner
    // cell, never a legitimate landing spot, so it doubles safely as the "disabled"
    // sentinel. See startTeleportWarp()/relocatePlayerToTeleport().
    unsigned char teleportX, teleportY;
};


extern const struct caveHandler caveList[];
extern const int caveCount;

// caveList[] is laid out as 10 planets of CAVES_PER_PLANET levels each (see its "// PLANET n"
// comments) -- planet p occupies indices [p*CAVES_PER_PLANET, (p+1)*CAVES_PER_PLANET).
#define CAVES_PER_PLANET 10


#define CAVEDEF_LOCK_X 4
#define CAVEDEF_LOCK_Y 2
#define CAVEDEF_NOBORDER 1
#define CAVEDEF_STAR_STATIC 8
#define CAVEDEF_START_WITH_WEAPON 16
#define CAVEDEF_MASK_LEFT_PIXEL 32

#define CAVEDEF_BONUS (CAVEDEF_MASK_LEFT_PIXEL)


#define DRAW_EOF 255
#define DRAW_FILLED_RECT 254
#define DRAW_RECT 253
#define DRAW_LINE 252

#define DRAW_OBJ 251 /* "last" */


// EOF
