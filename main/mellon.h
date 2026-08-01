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

// Teleport: stepping onto a CH_TELEPORT tile (checkHighPriorityMove(), mellon.c) sets
// teleportLocked and locks the player in place -- board.c's TYPE_MELLON_HUSK case skips
// movePlayer() entirely while this is set, same as it does for exitMode, so joystick
// input is simply ignored. Once the walk-in glide (autoMoveFrameCount) settles, board.c
// flips on teleportCountingDown (forces the stand animation, flashes, one-shot) and lets
// the spiral dots spin alone for teleportSwirlTicks before fading to black exactly like
// exitMode does; once the fade completes it flashes again and sets teleportRequested for
// OS_Game() (gameState_Game.c) to act on -- see its comment for why the actual cave
// switch is deferred there rather than happening immediately, mid-board-scan. loadCave()
// then fades back up on its own, same as any other level start.
extern bool teleportLocked;
extern bool teleportCountingDown;
extern int teleportSwirlTicks;
extern bool teleportRequested;

// Departure swirl: starts the instant teleportLocked goes true (checkHighPriorityMove(),
// mellon.c) -- i.e. as soon as the player starts moving onto the tile, not gated on the
// walk-in glide (autoMoveFrameCount) settling like the rest of the teleportLocked sequence
// is. updateTeleportDepartSwirl() is called every such frame from board.c's teleportLocked
// case, unconditionally (outside its "!autoMoveFrameCount" gate).
void startTeleportDepartSwirl();
void updateTeleportDepartSwirl(int x, int y);

// Arrival-side counterpart: spiral particles building up around the tile the player lands
// on, kicked off once (startTeleportArrivalSwirl(), schedule.c) alongside loadCave()'s
// luminance fade back up, then ticked down every frame regardless of scan budgets
// (updateTeleportArrivalSwirl(), called from VB_Game()).
void startTeleportArrivalSwirl(int x, int y);
void updateTeleportArrivalSwirl();
bool isTeleportArrivalSwirlActive();    // true from startTeleportArrivalSwirl() until its ticks run out
bool isTeleportArrivalPlayerHidden();   // like isTeleportArrivalSwirlActive(), but clears 0.5s
                                         // early -- see its own comment (mellon.c)

// Both teleport swirls spawn a small batch of spirals every N frames instead of trying to
// zap-and-refill the whole particle pool every single frame -- that was real, sustained
// extra work on top of whatever board processing was also happening that frame, a
// plausible source of the occasional overtime flash (board.c's FLASH(0xD6, 12)) reported
// right around teleports. Spawning periodically in small groups, with a long particle age
// so each batch persists rather than expiring mid-swirl, still reads as the spiral count
// building up over the wait -- just accumulated over many cheap frames instead of
// committed all at once on frame 1. Same age both sides; batch size/cadence tuned
// separately per side (departure needed a bigger batch to read as noticeable at all;
// arrival needed more packed into less total time, not just a bigger batch).
#define TELEPORT_SWIRL_PARTICLE_AGE 90

#define TELEPORT_DEPART_SWIRL_BATCH_SIZE 8
#define TELEPORT_DEPART_SWIRL_BATCH_INTERVAL 8

#define TELEPORT_ARRIVAL_SWIRL_BATCH_SIZE 6
#define TELEPORT_ARRIVAL_SWIRL_BATCH_INTERVAL 6

// EOF