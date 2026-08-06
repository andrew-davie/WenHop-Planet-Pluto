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


// A player interaction with the board can leave something visibly "detached" from its normal
// place: carried above their head, shoved along beside them, or briefly vibrating in place after
// a failed lift/shove attempt. All three used to be three separate ad-hoc variable sets
// (attachment/attachmentOffset, attachmentIsShove/shoveDestCell, attachment2/Col/Row/Ticks) --
// they're now just Attachments distinguished by `mode`. type == 0 means the slot is empty; every
// setter keeps it clean of FLAG_THISFRAME, so every reader can compare/index with it directly, no
// GET() needed.
enum AttachMode {
    ATTACH_CARRY,    // above the player's head, drop/pickup arc (offset) -- commits into a board
                     // cell via the ordinary "if (drop)" path (movePlayer(), mellon.c)
    ATTACH_SHOVE,    // rigid beside the player, slides with the walk-in glide -- commits into
                     // destCell once the glide settles (movePlayer(), mellon.c)
    ATTACH_SHAKE,    // anchored at (col, row), jitters in place -- restores itself to the board
                     // once ticks reaches 0 (VB_Game(), gameState_Game.c)
};

typedef struct {

    int type;    // CH_* value, or 0 if this slot is empty
    enum AttachMode mode;

    const OFFSET *offset;        // ATTACH_CARRY only -- drop/pickup arc cursor (draw.c)
    unsigned char *destCell;     // ATTACH_SHOVE only -- board cell to commit into on settle
    int col, row;                // ATTACH_SHAKE only -- board anchor to draw/restore at
    int ticks;                   // ATTACH_SHAKE only -- countdown to auto-restore

    int rollerY;                 // ATTACH_SHAKE only -- countdown to next jitter re-roll (draw.c)
    int jitterY;                 // ATTACH_SHAKE only -- current vertical jitter, held between re-rolls

} Attachment;

// How often (in real frames) drawAttachment() (draw.c) re-rolls a shake's vertical jitter --
// held steady between re-rolls instead of re-rolling every single frame, so it reads as a
// jerky, discrete vibration rather than a smooth blur.
#define SHAKE_ROLLER_TICKS_Y 3

// Amplitude, in trixels, of a shake's vertical jitter -- drawAttachment() (draw.c) picks a new
// offset in [-SHAKE_AMPLITUDE_Y, +SHAKE_AMPLITUDE_Y] every SHAKE_ROLLER_TICKS_Y frames.
#define SHAKE_AMPLITUDE_Y 1

// SLOT_CARRY is always the player's own held item (or 0) -- everything that used to read/write
// the old `attachment` variable directly now goes through attachments[SLOT_CARRY]. SLOT_ACTION is
// whichever of SHOVE or SHAKE is currently in progress from a player ACTION, if either (never
// both at once -- see their trigger sites, mellon.c) -- entirely independent of SLOT_CARRY, which
// is what lets the player shove an immovable while still carrying something else.
//
// SLOT_HAZARD1/2 are separate again: ambient shakes driven by the board scan itself rather than
// player input -- specifically, a rock/geodoge that can't ROLL off its perch because the player
// is standing in the landing spot it needs (doRollRock()/doRollGeodoge(), board.c,
// rollEligibility()'s ROLL_LOOSE) -- so they can't collide with whatever the player is doing in
// the other two slots. Two of them, not one: a rock resting one row up can be blocked by the
// player on its LEFT and a completely different rock one row up can independently be blocked on
// its RIGHT at the same time (two rocks flanking the player diagonally) -- findFreeHazardSlot()
// (mellon.c) picks whichever of the two is free.
#define SLOT_CARRY    0
#define SLOT_ACTION   1
#define SLOT_HAZARD1  2
#define SLOT_HAZARD2  3
#define NUM_ATTACHMENTS 4
extern Attachment attachments[NUM_ATTACHMENTS];

// Returns SLOT_HAZARD1 or SLOT_HAZARD2 for the cell at (col, row), or -1 if neither is available
// (already shaking that exact cell mid-handoff, or both genuinely busy elsewhere) -- board.c's
// CH_ROCK/CH_GEODOGE cases skip the shake for that pass in the -1 case (harmless: it's
// re-evaluated again next scan while the condition persists).
int findFreeHazardSlot(int col, int row);

extern int attachmentFlashTicks;    // see its own comment, mellon.c

// 0.8 seconds at 60fps. Shared with draw.c's drawAttachment(), which measures elapsed time
// against this (ATTACHMENT_FLASH_TICKS - attachmentFlashTicks) rather than testing
// attachmentFlashTicks directly, so the blink always starts BLANK on the very first frame
// regardless of this value's own bit pattern. mellon.c's trigger sites all guard on
// !attachmentFlashTicks, so a flash already in progress never restarts/retriggers.
#define ATTACHMENT_FLASH_TICKS 48

// See ATTACH_SHAKE's own comment above -- fills in attachments[slot] and reserves `cell` with
// CH_PLACEHOLDER for `ticks` frames.
void shakeObject(int slot, unsigned char *cell, int col, int row, int ticks);

extern int frameAdjustX;
extern int frameAdjustY;

// Extra luminance darkening applied to just the player sprite during the exit walk-off -- see
// its own comment (mellon.c).
extern int playerExitFade;

// One-shot latch for the exit door's slide-shut trigger -- see its own comment (mellon.c).
extern bool doorClosing;

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

// The tile the player stood on just before stepping onto the teleport tile -- see its own
// comment (mellon.c). Only meaningful while teleportLocked is true.
extern int teleportDepartOriginX, teleportDepartOriginY;

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

// True whenever drawPlayerSprite() (drawPlayer.c) is suppressing the player sprite for a
// teleport departure or arrival in progress -- see its own comment (mellon.c). Anything else
// drawn relative to the player should check this too, so it disappears/reappears in lockstep
// with the sprite instead of being left visible with nothing under it. Teleport only -- the
// exit-door sequence's own equivalent (exitMode, once playerExitFade reaches full black) is
// checked separately, alongside this, wherever this is used (drawPlayer.c, gameState_Game.c),
// rather than being folded in here, so this stays exactly what it always was and the door
// logic can't accidentally perturb teleport's.
bool isPlayerHidden();

// Ties the carried-object arrival rise duration to the same MOVE_SPEED-frame span the
// departure walk-in glide takes (see teleportCarryLift()'s comment, mellon.c) -- not otherwise
// related to MOVE_SPEED, just a convenient existing constant of about the right size. Relies
// on MOVE_SPEED (main.h) already being visible wherever this expands -- true for both current
// users (mellon.c, draw.c), each of which includes main.h directly.
#define TELEPORT_CARRY_LIFT_MAX MOVE_SPEED

// 0 (normal carry height) .. TELEPORT_CARRY_LIFT_MAX (fully merged with the player) -- see its
// own comment (mellon.c). drawAttachment() (draw.c) pulls the carried-object icon's draw
// offset toward the player's own position by this much on arrival, so it visibly rises out of
// them instead of popping straight to fully-carried.
int teleportCarryLift();

// Door-exit counterpart to teleportDepartOriginX/Y and teleportCarryLift() above -- same shape,
// entirely separate state, so the exit door can mirror teleport's departure-drop/arrival-lift
// behaviour exactly without sharing any of teleport's own variables or touching its code path.
// See their own comments (mellon.c) for the two-sided arm/consume story.
#define DOOR_CARRY_LIFT_MAX MOVE_SPEED
int doorCarryLift();
extern int exitDepartOriginX, exitDepartOriginY;

// See its own comment (mellon.c) -- lets initNewGame() (main.c) tell a door-exit's detour
// through GS_MENU/GS_GLOBE back to GS_GAME apart from an actual fresh game, so it doesn't
// clear out whatever's attached just because that detour happens to pass through the same
// initNewGame() call a real new game uses.
extern bool doorExitArmsCarryLift;

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
#define TELEPORT_SWIRL_PARTICLE_AGE 72    // 90, reduced 20%

#define TELEPORT_DEPART_SWIRL_BATCH_SIZE 8
#define TELEPORT_DEPART_SWIRL_BATCH_INTERVAL 8

#define TELEPORT_ARRIVAL_SWIRL_BATCH_SIZE 6
#define TELEPORT_ARRIVAL_SWIRL_BATCH_INTERVAL 6

// Carrying a key up to a locked door (checkHighPriorityMove(), mellon.c) sets doorLocked and
// starts the two-phase put-down/open timer -- unlike teleportLocked, the player is NOT frozen
// for it: board.c's TYPE_MELLON_HUSK case keeps calling movePlayer() every frame regardless,
// alongside updateDoorUnlock(). See updateDoorUnlock()'s own comment (mellon.c) for the timing,
// and doorUnlockOriginX/Y below for how the settling key stays visually pinned to the door tile
// while the player is free to walk away mid-animation.
extern bool doorLocked;
void updateDoorUnlock();

// The tile the player was standing on when the key touched the door -- recorded by
// startDoorUnlock() (mellon.c), same role as teleportDepartOriginX/Y and exitDepartOriginX/Y
// above. Needed because doorLocked no longer freezes the player: drawAttachment() (draw.c)
// draws the settling key relative to THIS fixed position instead of the player's own
// (potentially now-moving) playerX/playerY, so it stays pinned to the door instead of
// following the player off across the board.
extern int doorUnlockOriginX, doorUnlockOriginY;

// EOF