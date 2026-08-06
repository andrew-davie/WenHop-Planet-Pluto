#include "defines_dasm.h"

#include "animations.h"
#include "attribute.h"
#include "board.h"
#include "caveData.h"
#include "colour.h"
#include "decodeCaves.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "random.h"
#include "score.h"
#include "sound.h"

int playerX;    // char pos 0-39 (use * CHAR_TRIX_X for trixel)
int playerY;    // char pos 0-21 (use *CHAR_TRIX_Y for trixel and then *3 for scanline)

// See Attachment/SLOT_CARRY/SLOT_ACTION's own comments (mellon.h) for the full picture. type == 0
// means a slot is empty; every setter keeps it clean of FLAG_THISFRAME -- every reader can
// compare/index with it directly, no GET() needed. The one place that DOES need the flag -- the
// deferred board write when a carried item is actually dropped -- carries it separately via
// dropSkipThisFrame instead of baking it into attachments[SLOT_CARRY].type (see
// dropSkipThisFrame's own comment).
Attachment attachments[NUM_ATTACHMENTS];

// Ticks down to 0 once set; drawAttachment() (draw.c) blinks the carried item off on some of
// those frames instead of drawing it. Set to ATTACHMENT_FLASH_TICKS whenever the player tries
// to pick something up while already carrying something else -- see the two PickupCharacter[]
// trigger sites below (fire-button pickup, walk-in key pickup) -- as feedback that the attempt
// did nothing, rather than just silently blocking the move like a wall. Decremented once per
// real frame in VB_Game() (gameState_Game.c), same as shakeTime, so it still counts down even
// on the frames drawAttachment() itself is skipped (hidden player, exit fade, etc).
int attachmentFlashTicks = 0;

#define SHAKE_TICKS 30    // half a second at 60fps -- player-triggered shove/pickup feedback

// See SLOT_HAZARD1/2's own comment (mellon.h). col/row are the candidate cell's own board
// position -- checked against both slots first (regardless of which, if either, is free) because
// gameState_Game.c's restore keeps a just-restored slot's type nonzero for one extra frame after
// the real object is already back on the board (see its own comment), and re-qualifying THAT
// same cell during that one frame must not be handed the OTHER free slot -- that would let two
// slots fight over a single cell instead of cleanly finishing the handoff.
int findFreeHazardSlot(int col, int row) {

    if (attachments[SLOT_HAZARD1].type && attachments[SLOT_HAZARD1].col == col && attachments[SLOT_HAZARD1].row == row)
        return -1;

    if (attachments[SLOT_HAZARD2].type && attachments[SLOT_HAZARD2].col == col && attachments[SLOT_HAZARD2].row == row)
        return -1;

    if (!attachments[SLOT_HAZARD1].type)
        return SLOT_HAZARD1;

    if (!attachments[SLOT_HAZARD2].type)
        return SLOT_HAZARD2;

    return -1;
}

// Pulls `cell` off the board into attachments[slot] and reserves its square with CH_PLACEHOLDER
// for `ticks` frames -- see ATTACH_SHAKE's own comment (mellon.h) for the full picture. col/row
// are the cell's own board position, needed so drawAttachment() (draw.c) and the eventual
// restore (gameState_Game.c) both know where it belongs.
void shakeObject(int slot, unsigned char *cell, int col, int row, int ticks) {

    attachments[slot].type = GET(*cell);
    attachments[slot].mode = ATTACH_SHAKE;
    attachments[slot].col = col;
    attachments[slot].row = row;
    attachments[slot].ticks = ticks;

    // Roll the first jitter immediately (drawAttachment(), draw.c) rather than waiting a full
    // SHAKE_ROLLER_TICKS_Y for a first re-roll.
    attachments[slot].rollerY = 0;
    attachments[slot].jitterY = 0;

    *cell = FLAG(CH_PLACEHOLDER);
}

bool teleportLocked;
bool teleportCountingDown;
int teleportSwirlTicks;
bool teleportRequested;

// Departure: the tile the player stood on just before stepping onto the teleport tile --
// checkHighPriorityMove() below records this the instant it locks teleportLocked, and
// drawAttachment() (draw.c) draws the carried-object "drop" relative to THIS fixed tile
// instead of the already-updated-to-destination playerX/playerY, so it renders exactly like
// an ordinary stationary drop (player standing still, dropping in the direction they just
// walked) rather than inheriting the walk-in glide's motion.
int teleportDepartOriginX, teleportDepartOriginY;

// Door-exit counterpart to teleportDepartOriginX/Y -- same role (checkHighPriorityMove()'s
// exit trigger below records it, drawAttachment() draws relative to it instead of playerX/Y),
// entirely separate storage. Needed for exactly the same reason teleport needs its own copy:
// playerX/playerY get updated onto the door tile a few lines after the trigger fires (the
// shared ATT_BLANK|PERMEABLE|GRAB|EXIT movement code just below it), so without a frozen
// pre-move copy the drop would keep sliding as playerX/Y (and the walk-in glide on top of it)
// changed under it instead of settling once, exactly the "jumping all over the place" bug this
// was added to fix.
int exitDepartOriginX, exitDepartOriginY;

// Door-exit counterpart to teleportCarryLift() below -- same shape (armed to
// DOOR_CARRY_LIFT_MAX, ticks down by one per visible frame), entirely separate counter, so it
// can't add to or interfere with teleport's own ramp even though both ultimately feed the same
// drawAttachment() offsetY pull (draw.c sums the two -- see its own comment). Armed by
// initPlayer() only when doorExitArmsCarryLift was left set by the exit trigger just below
// (checkHighPriorityMove()), never for a teleport arrival or a fresh level start, and cleared
// right back to false there every time regardless, so it can only ever fire for the one cave
// load it was actually set up for.
static int doorCarryLiftTicks;

// Not static -- initNewGame() (main.c) reads this too. Walking out through an exit door doesn't
// go straight to loadCave() the way a teleport does: it routes through GS_MENU then GS_GLOBE
// before GS_GAME is re-entered, and initGameState_Game() calls initNewGame() (which clears
// SLOT_CARRY -- see its own comment, main.c) unconditionally on every entry to GS_GAME, not just
// a genuine fresh game. Left true here for that whole detour (only initPlayer() below ever
// clears it, on the loadCave() at the far end), initNewGame() checks it to tell "just finished a
// level via the door" apart from "actually starting over", so the carried item survives the trip
// the same way it already does for a teleport's direct loadCave() call.
bool doorExitArmsCarryLift;

int doorCarryLift() {
    int lift = doorCarryLiftTicks;
    if (doorCarryLiftTicks > 0)
        doorCarryLiftTicks--;
    return lift;
}

// Departure swirl: batches start the instant teleportLocked goes true (see
// startTeleportDepartSwirl()'s call site below) and keep going for as long as
// teleportLocked stays true -- board.c drives updateTeleportDepartSwirl() every such
// frame, unconditionally, so no separate tick countdown is needed here.
static int departSwirlBatchCounter;

void startTeleportDepartSwirl() {
    departSwirlBatchCounter = 0;
}

void updateTeleportDepartSwirl(int x, int y) {

    // A small batch every TELEPORT_DEPART_SWIRL_BATCH_INTERVAL frames, not a full pool
    // refill every frame -- see TELEPORT_SWIRL_PARTICLE_AGE's comment (mellon.h). Countdown,
    // not "counter++ % INTERVAL" -- this coprocessor has no divide instruction (see swipe.c's
    // isqrt() comment), and a non-power-of-2 INTERVAL forces the compiler to call out to
    // libgcc's software __aeabi_idivmod for the modulo. departSwirlBatchCounter starts at 0
    // (startTeleportDepartSwirl()), so the first call here fires immediately.
    if (departSwirlBatchCounter <= 0) {

        departSwirlBatchCounter = TELEPORT_DEPART_SWIRL_BATCH_INTERVAL;

        zapNonSpiralParticles();

        // Speed 32 = the original 40, reduced 20% (matches the arrival swirl below).
        nDots(TELEPORT_DEPART_SWIRL_BATCH_SIZE, x, y, PT_SPIRAL2, TELEPORT_SWIRL_PARTICLE_AGE, CHAR_CENTER_X,
              CHAR_CENTER_Y, 32, 7);
    }

    departSwirlBatchCounter--;
}

// 1.5 seconds at 60fps -- spirals building up around the arrival tile (see schedule.c's
// pendingTeleportArrival handling). Shorter than an earlier pass at this (was 180/3s,
// felt too slow) -- more particles now land in less time (see the batch size/interval in
// mellon.h) rather than spreading the same amount out even further.
#define TELEPORT_ARRIVAL_SWIRL_TICKS 90

static int arrivalSwirlTicks;
static int arrivalSwirlBatchCounter;
static int arrivalSwirlX, arrivalSwirlY;

void startTeleportArrivalSwirl(int x, int y) {

    arrivalSwirlX = x;
    arrivalSwirlY = y;
    arrivalSwirlTicks = TELEPORT_ARRIVAL_SWIRL_TICKS;
    arrivalSwirlBatchCounter = 0;
}

// 0.5 seconds at 60fps -- the player now appears this much before the swirl itself actually
// finishes (drawPlayer.c uses this instead of isTeleportArrivalSwirlActive() for its hidden
// check), so they materialise while the last of the spiral dots are still landing rather than
// waiting for arrivalSwirlTicks to hit 0. The swirl's own duration/spawn behaviour
// (TELEPORT_ARRIVAL_SWIRL_TICKS, updateTeleportArrivalSwirl() below) is untouched -- this only
// changes when the PLAYER stops being hidden, not how long the particles themselves run.
// Moved above updateTeleportArrivalSwirl() (was below) so that function can use it directly.
#define TELEPORT_ARRIVAL_PLAYER_REVEAL_EARLY_TICKS 60

void updateTeleportArrivalSwirl() {

    if (arrivalSwirlTicks) {

        if (arrivalSwirlBatchCounter <= 0) {

            arrivalSwirlBatchCounter = TELEPORT_ARRIVAL_SWIRL_BATCH_INTERVAL;

            zapNonSpiralParticles();

            // Speed 32 = the original 40, reduced 20%.
            nDots(TELEPORT_ARRIVAL_SWIRL_BATCH_SIZE, arrivalSwirlX, arrivalSwirlY, PT_SPIRAL2,
                  TELEPORT_SWIRL_PARTICLE_AGE, CHAR_CENTER_X, CHAR_CENTER_Y, 32, 7);
        }

        arrivalSwirlBatchCounter--;
        arrivalSwirlTicks--;
    }
}

bool isTeleportArrivalSwirlActive() {
    return arrivalSwirlTicks != 0;
}

bool isTeleportArrivalPlayerHidden() {
    return arrivalSwirlTicks > TELEPORT_ARRIVAL_PLAYER_REVEAL_EARLY_TICKS;
}

// True exactly when drawPlayerSprite() (drawPlayer.c) is suppressing the player's own sprite
// for a teleport in progress -- departure (once the walk-in glide onto the tile has settled)
// or arrival (isTeleportArrivalPlayerHidden(), above). Shared so anything else drawn "on" the
// player -- currently just the carried-object icon, gameState_Game.c's drawAttachment()
// call -- can be suppressed in lockstep instead of being left floating in place with no player
// underneath it once the sprite itself vanishes into/out of the tile.
bool isPlayerHidden() {
    return (teleportLocked && !autoMoveFrameCount) || isTeleportArrivalPlayerHidden();
}

// Arrival only (departure uses a different mechanism entirely -- checkHighPriorityMove()'s
// dropOffset[] trigger and drawAttachment()'s teleportLocked correction, draw.c). How far
// drawAttachment() should pull the carried-object icon toward the player's own default draw
// position -- 0 = normal carry height, TELEPORT_CARRY_LIFT_MAX (mellon.h) = fully merged with
// the player. The instant the player stops being hidden on arrival
// (isTeleportArrivalPlayerHidden() clearing), this starts at MAX and falls to 0 over the next
// TELEPORT_CARRY_LIFT_MAX ticks, so the object visibly rises up out of the player instead of
// popping straight to fully-carried.
int teleportCarryLift() {

    if (isTeleportArrivalSwirlActive() && !isTeleportArrivalPlayerHidden()) {

        int ticksVisible = TELEPORT_ARRIVAL_PLAYER_REVEAL_EARLY_TICKS - arrivalSwirlTicks;
        if (ticksVisible < TELEPORT_CARRY_LIFT_MAX)
            return TELEPORT_CARRY_LIFT_MAX - ticksVisible;    // MAX right as they reappear, falling to 0
    }

    return 0;
}

int frameAdjustX;
int frameAdjustY;

// Exit sequence only (updatePlayerAnimation(), playerAnimation.c): how many luminance units to
// darken the player sprite by, on top of whatever the level's own luminance fade already
// contributes (drawPlayerSprite(), drawPlayer.c) -- 0 = normal, 15 = fully black. The level-wide
// fade (lumTarget, board.c's exitMode case) doesn't start until exitMode drops below 20, so most
// of the walk-off drift would otherwise play out at full brightness with the fade only catching
// up in the last ~15 real frames; this fades the player out smoothly across the whole walk
// instead, independent of that.
int playerExitFade;

// One-shot latch for the exit door's slide-shut trigger (updatePlayerAnimation(),
// playerAnimation.c) -- set true right there the instant it fires, reset false here whenever a
// fresh exit sequence starts (checkHighPriorityMove()'s exit trigger below). Comparing
// Animate[TYPE_OUTBOX] against AnimateDoorClose + 2 directly doesn't work as a one-shot guard:
// that pointer value is only true WHILE the slide is mid-flight -- once it finishes advancing to
// AnimateDoorClose + 4 (CH_DOORCLOSED), the comparison starts matching "not yet triggered" again
// and the trigger re-fires every single frame from then on, permanently restarting the slide
// from scratch (visible as the door never actually settling, cycling closed for one frame then
// snapping back open).
bool doorClosing;
static unsigned int pushCounter;

// True from the moment the fire-button dirt-dig (checkHighPriorityMove(), below) commits until
// the fire button is released -- see movePlayer()'s own check for the release detection. Holds
// the dig/grab animation and blocks all movement for as long as it's true, same idea as kdelay
// but keyed to the button rather than a fixed tick count.
static bool digging;
enum FaceDirectionX faceDirection;
bool playerDead;
static int waitForNothing;
bool handled;
static bool gearsActive;
static bool gearsWaitRelease;
// Ticks down once per movePlayer() call (i.e. once per board scan, which only restarts every
// gameSpeed real frames -- SPEED_BASE=5, ~12 scans/sec at NTSC's 60fps, see board.c's
// setupBoardScanner()) -- NOT once per real frame. Set on a successful pickup (fire-button or
// walk-in key, both below) to freeze the player for a beat: movePlayer()'s dispatch loops are
// skipped entirely while this is nonzero, so no new move of ANY kind can start, not just a
// fire-button re-trigger. Needed because without it, a player who just picked up a rock/boulder
// could immediately step into the square it vacated the same tick another falling rock lands
// there, getting crushed by something that hadn't had a chance to resolve yet. Deliberately NOT
// set by the walk-in shove trigger below -- pushing an immovable block has no equivalent hazard
// and should stay fully responsive.
#define PICKUP_DELAY_TICKS 6    // ~0.5s at ~12 game-loop ticks/sec (see comment above)
static int kdelay = 0;

static unsigned char *meAtt;
static bool drop = false;

// attachments[SLOT_CARRY].type is always kept clean of FLAG_THISFRAME (every setter strips it,
// every reader can compare/index with it directly -- see Attachment's own declaration, mellon.h)
// -- but the deferred board write in movePlayer()'s "if (drop)" block (one frame after this is
// set) still needs to know whether ITS write should carry the flag, same reasoning as
// FLAG(CH_DUST_0) elsewhere: a cell dropped into a board position the scanner hasn't reached
// yet this pass (right/down of the player, given scan order) must skip processing until next
// frame, or it can fall/roll the instant it lands. dir isn't available in movePlayer(), so
// this carries that one bit forward the one frame checkHighPriorityMove() -> movePlayer().
static bool dropSkipThisFrame;

// Carrying a key up to CH_DOORCLOSED (checkHighPriorityMove() below) starts a two-phase
// countdown, but -- unlike teleportLocked, which this used to mirror -- doesn't freeze the
// player for it: board.c's TYPE_MELLON_HUSK case calls movePlayer() every frame regardless,
// calling updateDoorUnlock() alongside it whenever doorLocked is set. First the key's normal
// drop arc (dropOffset[], same table every other drop uses) settles into the door's cell,
// then the door itself becomes CH_DOOROPEN_0 and a short pause follows before doorLocked
// clears. The player can walk off mid-animation -- drawAttachment() (draw.c) keeps drawing
// the settling key pinned to doorUnlockOriginX/Y (recorded below) instead of the player's own
// current position, exactly like teleportDepartOriginX/Y does for a teleport departure.
// checkHighPriorityMove() also guards its trigger and the fire-button drop/pickup block with
// !doorLocked, since SLOT_CARRY is still "spoken for" by the door until updateDoorUnlock()
// clears it -- without that, walking straight back into the door (or firing to drop/pick up
// elsewhere) mid-animation could re-trigger or double-consume the key.
bool doorLocked;
static unsigned char *doorUnlockCell;
static int doorUnlockTicks;
static bool doorOpened;
int doorUnlockOriginX, doorUnlockOriginY;

// This case only ticks once every SPEED_BASE (5) real render frames (see
// TELEPORT_SWIRL_TICKS's comment above, board.c) -- drawAttachment() (draw.c) advances the
// carry slot's offset once per render call regardless. dropOffset[]'s arc used to be 8 real
// keyframes deep and ran out well before a multi-tick put phase could commit, snapping the
// key back to the carry slot's *default* draw offset (the "carried above head" pose) for the
// remaining wait -- see REST_HOLD (above the dropOffset* tables) for the fix: 15 extra
// repeats of the resting keyframe, 23 real entries total, good for ~4.6 ticks (23 render
// calls / 5 per tick) before it would run out. Keep this at or below 4 so it stays inside
// that budget; raising it further means growing REST_HOLD's repeat count too.
#define DOOR_UNLOCK_PUT_TICKS 4
#define DOOR_UNLOCK_PAUSE_TICKS 4    // extra beat with the door visibly open before doorLocked clears


const enum JOYSTICK_DIRECTION joyDirectBit[] = {
    JOYSTICK_UP,
    JOYSTICK_RIGHT,
    JOYSTICK_DOWN,
    JOYSTICK_LEFT,
};

const enum FaceDirectionX faceDirectionDef[] = {
    FACE_UP,
    FACE_RIGHT,
    FACE_DOWN,
    FACE_LEFT,
};

static void startDoorUnlock(unsigned char *cell, int dir) {

    doorLocked = true;
    doorOpened = false;
    doorUnlockCell = cell;
    doorUnlockTicks = DOOR_UNLOCK_PUT_TICKS;

    // playerX/playerY haven't moved onto the door tile for this trigger (the key is dropped
    // forward into it, same as any other stationary drop) -- but the player is free to walk
    // off starting next frame while doorLocked, so pin the settling key's draw position to
    // wherever they were standing right now, same idea as teleportDepartOriginX/Y.
    doorUnlockOriginX = playerX;
    doorUnlockOriginY = playerY;

    // Kick the visual split off now, the instant the key touches the door, so it has the whole
    // DOOR_UNLOCK_PUT_TICKS wait (4 ticks * 5 real frames/tick = 20 real frames) to finish before
    // updateDoorUnlock() below commits the real board write -- AnimateDoor's own total (14 real
    // frames) comfortably fits inside that. If DOOR_UNLOCK_PUT_TICKS or AnimateDoor's per-frame
    // delays ever change, keep that inequality true, or the door will become walkable mid-slide.
    startCharAnimation(TYPE_DOOR, AnimateDoor + 2);

    faceDirection = faceDirectionDef[dir];

    // Give the stand pose immediately rather than waiting a frame for movePlayer()'s own
    // walk-to-stand transition to catch up -- harmless either way now that movePlayer() runs
    // every frame during doorLocked too (unlike teleportLocked/exitMode, which still skip it).
    int dir2 = (gravity < 0) ? dir ^ 2 : dir;
    startPlayerAnimation(dir2 == 0 ? ID_StandUp : dir2 == 2 ? ID_Stand : ID_StandLR);
}

void updateDoorUnlock() {

    if (--doorUnlockTicks > 0)
        return;

    if (!doorOpened) {

        *doorUnlockCell = CH_DOOROPEN_STATIC;    // not CH_DOOROPEN_0 -- see AnimateDoor's comment
        ADDAUDIO(SFX_DOOR);

        attachments[SLOT_CARRY].type = 0;
        attachments[SLOT_CARRY].offset = 0;

        doorOpened = true;
        doorUnlockTicks = DOOR_UNLOCK_PAUSE_TICKS;

    } else
        doorLocked = false;
}

const signed int animDeltaX[] = {
    0,
    -CHAR_X * 0x10000 / MOVE_SPEED,
    0,
    -CHAR_X * 0x10000 / MOVE_SPEED,
};

const signed int animDeltaY[] = {
    CHAR_Y * 0x10000 / MOVE_SPEED,     //
    0,                                 //
    -CHAR_Y * 0x10000 / MOVE_SPEED,    //
    0                                  //
};


const unsigned char mineAnimation[] = {
    ID_MineUp,
    ID_Mine,
    ID_MineDown,
    ID_Mine,
};

const unsigned char WalkAnimation[] = {
    ID_WalkUp,      // U
    ID_Walk,        // R
    ID_WalkDown,    // D
    ID_Walk,        // L
};

void initPlayer() {

    pushCounter = 0;
    playerDead = false;

    teleportLocked = false;
    teleportCountingDown = false;
    teleportSwirlTicks = 0;
    teleportRequested = false;

    faceDirection = FACE_RIGHT;

    gearsActive = false;
    gearsWaitRelease = false;

    drop = false;
    attachments[SLOT_CARRY].offset = 0;

    // Door-exit counterpart to teleport arrival's own carry-lift ramp (which needs no arming
    // here -- teleportCarryLift() drives itself purely off arrivalSwirlTicks, untouched).
    // doorExitArmsCarryLift is only ever left true by the door-exit trigger just below
    // (checkHighPriorityMove()) for the one cave load it set up; every other load through here
    // (a teleport arrival, a fresh level start) leaves it false, so doorCarryLiftTicks stays 0
    // and doorCarryLift() is a no-op for them, same as it always was pre-door.
    doorCarryLiftTicks = (doorExitArmsCarryLift && attachments[SLOT_CARRY].type) ? DOOR_CARRY_LIFT_MAX : 0;
    doorExitArmsCarryLift = false;

    startPlayerAnimation(ID_Stand);    // tmp
}

void grabDoge() {

    totalDogePossible--;

    addScore(100);    // theCave->dogeValue);

    --doges;
}

static int playerSlow = 0;


void moveHusk(int dir, unsigned char *me, unsigned char *meOffset) {

    unsigned char destType = CharToType[GET(*meOffset)];

    *me = FLAG(CH_BLANK);
    *meOffset = FLAG(CH_MELLON_HUSK);

    if (destType == TYPE_PEBBLE1)
        nDots(4, playerX, playerY, PT_ONE, 10, CHAR_CENTER_X, CHAR_CENTER_Y, 10, 7);


    if (Attribute[destType] & ATT_DIRT) {
        startCharAnimation(TYPE_MELLON_HUSK, AnimateBase[TYPE_MELLON_HUSK]);

        int xsize, ysize;
        int xoff, yoff;


        for (int i = 0; i < 12; i++) {

            if (ydir[dir]) {
                ysize = 4;
                xsize = CHAR_TRIX_X - 1;
                xoff = rangeRandom(xsize) + 1;    // rangeRandom(xsize);
                yoff = (ydir[dir] < 0) ? 1 + rangeRandom(ysize)
                                       : 9 - ysize + rangeRandom(ysize);    // + rangeRandom(ysize);
            } else {
                ysize = CHAR_TRIX_Y - 1;
                xsize = 0;
                yoff = rangeRandom(ysize) + 1;    // rangeRandom(xsize);
                xoff = (xdir[dir] < 0) ? 1 + rangeRandom(xsize)
                                       : 4 - xsize + rangeRandom(xsize);    // + rangeRandom(ysize);
            }
            nDots(1, playerX, playerY, PT_ONE, 30, xoff, yoff, 30, 2);    // OK
        }
    } else
        startCharAnimation(TYPE_MELLON_HUSK, AnimateBase[TYPE_MELLON_HUSK] + 8);
}


// A genuine arc, not a constant-rate diagonal: the first half moves almost entirely in y (the
// item rising straight up off the ground beside the player, x barely shifting) and the second
// half moves almost entirely in x (sweeping sideways into the carried position, y already
// settled) -- mostly vertical near the side, mostly horizontal near the top, instead of x and y
// ticking down together the whole way (which reads as a straight diagonal line no matter how
// many keyframes it's split into).
const OFFSET sampleOffsetRight[] = {

    {-5, -2 * 3}, {-5, -5 * 3}, {-4, -7 * 3}, {-4, -8 * 3}, {-3, -8 * 3},
    {-2, -8 * 3}, {-1, -8 * 3}, {0, -8 * 3},  {0, 0},
};

// Mirror of sampleOffsetRight above (x negated, same y).
const OFFSET sampleOffsetLeft[] = {

    {5, -2 * 3}, {5, -5 * 3}, {4, -7 * 3}, {4, -8 * 3}, {3, -8 * 3}, {2, -8 * 3}, {1, -8 * 3}, {0, -8 * 3}, {0, 0},
};

const OFFSET sampleOffsetDown[] = {

    {0, 8 * 3}, {-1, 5 * 3}, {-2, 2 * 3}, {-2, -2 * 3}, {-2, -5 * 3}, {-1, -6 * 3}, {-1, -7 * 3}, {0, -8 * 3}, {0, 0},
};

const OFFSET sampleOffsetUp[] = {
    {1, -11 * 3}, {1, -12 * 3}, {1, -13 * 3}, {0, -14 * 3}, {0, -12 * 3},
    {1, -10 * 3}, {1, -8 * 3},  {0, -8 * 3},  {0, 0},
};


const OFFSET *pickupOffset[] = {
    sampleOffsetUp,       // up
    sampleOffsetRight,    // right
    sampleOffsetDown,     // down
    sampleOffsetLeft,     // left
};


// The last real keyframe in each table below is where the dropped object comes to rest --
// repeated an extra REST_HOLD times before the {0,0} terminator so drawAttachment() (draw.c)
// keeps showing it sitting there instead of its own per-render auto-advance running off the
// end and falling back to the carry slot's default "carried above head" draw offset. Only
// actually exercised by callers that hold the slot/drop deliberately for several ticks after
// landing (see updateDoorUnlock(), which does exactly that) -- an ordinary drop still commits
// (writes the board cell, clears the slot) on the very next tick regardless, long before these
// extra entries would ever be reached.
#define REST_HOLD(x, y)                                                                                                \
    {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {x, y}, {  \
        x, y                                                                                                           \
    }

const OFFSET dropOffsetRight[] = {

    {0, -8 * 3},                      //
    {-1, -7 * 3},                     //
    {-2, -7 * 3},                     //
    {-3, -7 * 3},                     //
    {-4, -6 * 3},                     //
    {-4, -6 * 3},                     //
    {-5, -4 * 3},                     //
    {-5, -2 * 3},                     //
    REST_HOLD(-5, -2 * 3), {0, 0},    //
};

const OFFSET dropOffsetLeft[] = {

    {0, -8 * 3},                     //
    {1, -7 * 3},                     //
    {2, -7 * 3},                     //
    {3, -7 * 3},                     //
    {4, -6 * 3},                     //
    {4, -6 * 3},                     //
    {5, -4 * 3},                     //
    {5, -2 * 3},                     //
    REST_HOLD(5, -2 * 3), {0, 0},    //
};

const OFFSET dropOffsetDown[] = {

    {0, -8 * 3},                    //
    {-1, -7 * 3},                   //
    {-1, -6 * 3},                   //
    {-2, -5 * 3},                   //
    {-2, -2 * 3},                   //
    {-2, 2 * 3},                    //
    {-1, 5 * 3},                    //
    {0, 8 * 3},                     //
    REST_HOLD(0, 8 * 3), {0, 0},    //
};

const OFFSET dropOffsetUp[] = {
    {0, -8 * 3},                      //
    {1, -8 * 3},                      //
    {1, -10 * 3},                     //
    {0, -12 * 3},                     //
    {0, -14 * 3},                     //
    {1, -13 * 3},                     //
    {1, -12 * 3},                     //
    {1, -11 * 3},                     //
    REST_HOLD(1, -11 * 3), {0, 0},    //
};


const OFFSET *dropOffset[] = {
    dropOffsetUp,       // up
    dropOffsetRight,    // right
    dropOffsetDown,     // down
    dropOffsetLeft,     // left
};

bool checkHighPriorityMove(BoardCursor *cur, int dir) {

    static int zapDelay = 0;
    if (zapDelay)
        --zapDelay;


    unsigned char joyBit = joyDirectBit[dir] << 4;
    if (usableSWCHA & joyBit)
        return false;

    // !doorLocked: the carry slot is still CH_KEY (and still "spoken for" by the settling door,
    // startDoorUnlock() below) for as long as doorLocked holds -- since the player can now
    // walk around during that window, this block would otherwise let a fire-button press
    // drop/re-pick-up that same key elsewhere while it's also mid-way into the door,
    // double-consuming it. See doorLocked's own comment above for the full picture.
    //
    // Not guarded on the action slot being mid-shove any more -- SLOT_CARRY and SLOT_ACTION are
    // now independent (see their own comment, mellon.h), so a fire-button press while shoving
    // just acts on whatever's in SLOT_CARRY, if anything, same as it would with no shove going
    // on at all.
    if (!doorLocked && !kdelay && !waitRelease) {
        if (!(inpt4 & 0x80)) {

            meAtt = cur->me + dirOffset[dir];

            if (attachments[SLOT_CARRY].type) {

                int type2 = CharToType[GET(*meAtt)];
                if (Attribute[type2] & ATT_BLANK) {

                    // What's being converted here is what's actually being CARRIED
                    // (CharToType[attachments[SLOT_CARRY].type]), not the target cell (type2) --
                    // type2 is guaranteed blank by the check just above, and neither TYPE_ROCK
                    // nor TYPE_GEODOGE is ever ATT_BLANK, so comparing type2 against them here
                    // could never be true (this used to compare type2, dead code). Only
                    // convert to the falling variant if there's ALSO nothing solid directly
                    // below the target -- landing right on solid ground (dirt, wall, rock,
                    // brick, whatever) should commit as a settled item so the immediate
                    // landing check just below (movePlayer()'s "if (drop)" block) fires this
                    // same frame, not whenever board.c's case CH_ROCK_FALLING: next happens to
                    // get scanned. Only a drop into a genuine gap (open air under the target
                    // too) needs to actually fall first.
                    if (Attribute[CharToType[GET(*(meAtt + _BOARD_COLS))]] & ATT_BLANK) {

                        if (CharToType[attachments[SLOT_CARRY].type] == TYPE_GEODOGE)
                            attachments[SLOT_CARRY].type = CH_GEODOGE_FALLING;

                        else if (CharToType[attachments[SLOT_CARRY].type] == TYPE_ROCK)
                            attachments[SLOT_CARRY].type = CH_ROCK_FALLING;
                    }

                    dropSkipThisFrame = (dir == 1 || dir == 2);

                    drop = true;
                    attachments[SLOT_CARRY].offset = dropOffset[dir];
                    ADDAUDIO(SFX_DROP);
                    return true;
                }

                // Fire-button aimed at something liftable (PickupCharacter[] nonzero), but
                // already carrying something else -- same "did nothing" feedback as the walk-in
                // key pickup attempt below: flash the carried item instead of silently doing
                // nothing. Falls through unchanged otherwise -- not ATT_BLANK and not liftable
                // just means bump into it like a wall, same as before. Guarded on
                // !attachmentFlashTicks so holding the direction/button against it doesn't keep
                // restarting the flash from its (deliberately blank) first frame -- let an
                // already-running flash finish its cycle undisturbed.
                else if (PickupCharacter[type2] && !attachmentFlashTicks)
                    attachmentFlashTicks = ATTACHMENT_FLASH_TICKS;
            }

            else {

                unsigned char pickup = PickupCharacter[CharToType[GET(*meAtt)]];
                if (pickup) {

                    kdelay = PICKUP_DELAY_TICKS;

                    faceDirection = faceDirectionDef[dir];

                    attachments[SLOT_CARRY].type = pickup;
                    attachments[SLOT_CARRY].mode = ATTACH_CARRY;

                    ADDAUDIO(SFX_LIFT);

                    extern const unsigned char AnimateBomb[];
                    if (pickup == CH_BOMB)
                        startCharAnimation(TYPE_BOMB, AnimateBomb);

                    attachments[SLOT_CARRY].offset = pickupOffset[dir];

                    *meAtt = FLAG(CH_DUST_0);
                    waitRelease = true;
                    return true;
                }
            }

            // Fire-button aimed at something that can't be lifted at all (PickupCharacter[] is 0
            // for TYPE_IMMOVABLE) -- give the same "did nothing" feedback as the walk-in
            // shove-blocked case below, via the independent action slot (SLOT_ACTION, mellon.h)
            // instead of silently bumping into it. Checked regardless of what's in SLOT_CARRY --
            // shaking one object doesn't care whether the player is also carrying something else
            // -- and guarded on the action slot being free so this can't stomp an in-progress
            // shove/shake.
            if (!attachments[SLOT_ACTION].type && CharToType[GET(*meAtt)] == TYPE_IMMOVABLE)
                shakeObject(SLOT_ACTION, meAtt, playerX + xdir[dir], playerY + ydir[dir], SHAKE_TICKS);

            // Fire-button dig: instantly clears dirt in the aimed direction without having to
            // walk into it like the normal walk-through consumption does (moveHusk()'s ATT_DIRT
            // branch, below) -- works regardless of whether the player is currently carrying/
            // shoving something, since dirt is neither ATT_BLANK nor liftable and so falls
            // through both branches above untouched either way. Same SFX_DIRT/dust-cloud cue as
            // walking through it.
            if (CharToType[GET(*meAtt)] == TYPE_DIRT) {

                ADDAUDIO(SFX_DIRT);
                nDots(6, playerX + xdir[dir], playerY + ydir[dir], PT_ONE, 30, CHAR_CENTER_X, CHAR_CENTER_Y, 30, 2);

                *meAtt = FLAG(CH_DUST_0);

                // No dedicated grab animation -- reuse the directional mining pose
                // (mineAnimation[], same selection checkLowPriorityMove() uses for the
                // walk-and-hold mining mechanic) as a placeholder. digging (see its own
                // comment) holds it -- and blocks all movement -- until the fire button is
                // released; movePlayer()'s existing "switch back to standing" check already
                // covers ID_Mine/ID_MineUp/ID_MineDown, so releasing needs no special-case
                // cleanup here.
                int anim = mineAnimation[dir];
                if (!(dir & 1) && gravity < 0)
                    anim ^= ID_MineDown ^ ID_MineUp;
                startPlayerAnimation(anim);

                digging = true;
                return true;
            }
        }
    }


    if (faceDirectionDef[dir] && faceDirection != faceDirectionDef[dir]) {
        pushCounter = 0;    // so we get an animation during turn
        faceDirection = faceDirectionDef[dir];
    }

    unsigned char *meOffset = cur->me + dirOffset[dir];
    enum ObjectType destType = CharToType[GET(*meOffset)];

    {    // no fire button

        int type = CharToType[GET(*meOffset)];

        if (type == TYPE_GRINDER || type == TYPE_GRINDER_1) {
            ADDAUDIO(SFX_EXPLODE_QUIET);


            nDots(4, playerX, playerY, PT_TWO, 40,    //
                  ((xdir[dir] + 1) >> 1) * CHAR_TRIX_X + (ydir[dir] ? CHAR_CENTER_X : 0),
                  ((ydir[dir] + 1) >> 1) * CHAR_TRIX_Y + (xdir[dir] ? CHAR_CENTER_Y : 0), 50, rangeRandom(7) + 1);


            if (!gearsWaitRelease) {

                gearsWaitRelease = true;
                gearsActive = !gearsActive;

                if (gearsActive)
                    FLASH(0x28, 10);

                toggleGears(gearsActive);
            }

#if ENABLE_SHAKE

            if (!gearsActive)
                setShake(5);
#endif
        }


        else if (type == TYPE_ELECTRIC) {

            FLASH(0x28, 13);

            pulsePlayerColour = 100;

            if (!zapDelay || (!rangeRandom(30))) {
                zapDelay = 50;
            }

            if (rangeRandom(20)) {

                ADDAUDIO(SFX_ZAP);
                ADDAUDIO(SFX_ZAP2);
            } else {
                FLASH(0x0F, 1);
                ADDAUDIO(SFX_EXPLODE);
            }

            setShake(50);

            nDots(3, playerX, playerY, PT_SPIRAL, 25, 3 + ((xdir[dir] * CHAR_TRIX_X) >> 1) + rangeRandom(5) - 2,
                  4 + ((ydir[dir] * CHAR_TRIX_Y) >> 1) + rangeRandom(5) - 2, 100, 2);

            return true;
        }

        else if (destType == TYPE_STAR) {


            // int y = playerY * CHAR_TRIX_Y - (scrollY >> 16) - CHAR_TRIX_Y;
            // int x = playerX * CHAR_TRIX_X - (scrollX >> 16) + CHAR_TRIX_X;

            // if (y < 0)
            //     y = 0;
            // removeFloatingChars();
            // floatingCharacter(x, y, 30, CH_PLUS);


            // ADDAUDIO(SFX_ZAP2);
            // ADDAUDIO(SFX_WHOOSH);
            ADDAUDIO(SFX_BONUS);
            weapon = theCave->weapon[level];
            nDots(10, playerX + xdir[dir], playerY + ydir[dir], PT_SPIRAL2, 40, 3, 4, 50, 7);

            playerX += xdir[dir];
            playerY += ydir[dir];

            moveHusk(dir, cur->me, meOffset);

            int dir2 = (gravity < 0) ? dir ^ 2 : dir;

            if (playerAnimationID != WalkAnimation[dir2])
                startPlayerAnimation(WalkAnimation[dir2]);

            if (!autoMoveFrameCount) {

                autoMoveFrameCount = ((MOVE_SPEED) << playerSlow);

                autoMoveX = autoMoveDeltaX = animDeltaX[dir] >> playerSlow;
                autoMoveY = autoMoveDeltaY = animDeltaY[dir] >> playerSlow;
            }

            handled = true;
        }

        // Auto-pickup on walk-in, same as TYPE_STAR above -- but gated on the carry slot being
        // free, unlike STAR: a key is something you carry (SLOT_CARRY, same slot the fire-button
        // pickup path above uses), so if you're already carrying something this branch is
        // simply skipped -- destType == TYPE_KEY has no ATT_BLANK/PERMEABLE/GRAB/EXIT either
        // (attribute.c), so the generic walkable check below won't catch it and the move is
        // blocked, same as walking into a wall.
        else if (!attachments[SLOT_CARRY].type && destType == TYPE_KEY) {

            ADDAUDIO(SFX_LIFT);

            kdelay = PICKUP_DELAY_TICKS;

            attachments[SLOT_CARRY].type = CH_KEY;
            attachments[SLOT_CARRY].mode = ATTACH_CARRY;
            attachments[SLOT_CARRY].offset = pickupOffset[dir];

            playerX += xdir[dir];
            playerY += ydir[dir];

            moveHusk(dir, cur->me, meOffset);

            int dir2 = (gravity < 0) ? dir ^ 2 : dir;

            if (playerAnimationID != WalkAnimation[dir2])
                startPlayerAnimation(WalkAnimation[dir2]);

            if (!autoMoveFrameCount) {

                autoMoveFrameCount = ((MOVE_SPEED) << playerSlow);

                autoMoveX = autoMoveDeltaX = animDeltaX[dir] >> playerSlow;
                autoMoveY = autoMoveDeltaY = animDeltaY[dir] >> playerSlow;
            }

            handled = true;
        }

        // Same trigger as the fire-button pickup's PickupCharacter[] case above -- already
        // carrying something, so the auto-pickup just above is skipped and the move stays
        // blocked (see its own comment): flash the carried item AND shake the key itself, as
        // feedback that walking into it did something rather than nothing.
        else if (attachments[SLOT_CARRY].type && destType == TYPE_KEY) {

            // !attachmentFlashTicks guard: see the fire-button pickup case's own comment --
            // don't restart an already-running flash.
            if (!attachmentFlashTicks)
                attachmentFlashTicks = ATTACHMENT_FLASH_TICKS;

            // !attachments[SLOT_ACTION].type guard: see the fire-button immovable-shake case's
            // own comment -- shakeObject() immediately swaps the cell to CH_PLACEHOLDER, so this
            // naturally can't re-fire until the shake settles and the key is written back.
            if (!attachments[SLOT_ACTION].type)
                shakeObject(SLOT_ACTION, meOffset, playerX + xdir[dir], playerY + ydir[dir], SHAKE_TICKS);
        }

        // Carrying a key up to a still-locked door (CH_DOORCLOSED specifically, not just any
        // TYPE_DOOR -- board.c's ambient CH_DOORCLOSED case also opens these once
        // !doges, this is just an earlier/alternate trigger): a normal drop, using the exact
        // same offset arc (dropOffset[dir]) and SLOT_CARRY (still CH_KEY, still drawn by
        // drawAttachment()) as any other drop -- the slot doesn't get cleared until
        // updateDoorUnlock() actually commits. The key never becomes a real board character --
        // doorLocked (board.c's TYPE_MELLON_HUSK case) drives updateDoorUnlock() every frame
        // until it swaps the door open; see its own comment for the two-phase timing and for
        // why the player is NOT frozen while this plays out.
        //
        // !doorLocked here (as well as guarding the fire-button block above) stops this from
        // re-triggering every frame the player holds the direction into a door that's already
        // mid-open -- SLOT_CARRY stays CH_KEY and *meOffset stays CH_DOORCLOSED for the whole
        // window, so without the guard this would just keep restarting startDoorUnlock().
        //
        // attachments[SLOT_CARRY].type is never flagged with FLAG_THISFRAME (every setter
        // strips it), so it can be compared directly here; *meOffset is a real board cell and
        // still needs GET().
        else if (!doorLocked && attachments[SLOT_CARRY].type == CH_KEY && GET(*meOffset) == CH_DOORCLOSED) {

            attachments[SLOT_CARRY].offset = dropOffset[dir];
            startDoorUnlock(meOffset, dir);

            handled = true;
        }


        // Walking into an ATT_SHOVE object (TYPE_IMMOVABLE) from the left or right tries to
        // shove it one square further the same way -- distinct from ATT_PUSH/PSH (board.c's
        // genericPush(), a mechanical pusher-bar shoving something), this is the PLAYER doing
        // it by walking into it. No vertical case -- shoving is left/right only (dir 1 or 3).
        // Uses the action slot (SLOT_ACTION, mellon.h), entirely independent of SLOT_CARRY -- the
        // player can shove an immovable while still carrying something else in the other slot.
        // !attachments[SLOT_ACTION].type guards against shoving a second block while one's
        // already mid-shove/shake, and naturally also prevents re-triggering on the SAME block
        // before its previous shove has settled (the action slot stays CH_IMMOVABLE, non-zero,
        // for the whole walk-in glide).
        else if (!attachments[SLOT_ACTION].type && (dir == 1 || dir == 3) && (Attribute[destType] & ATT_SHOVE)) {

            unsigned char *behindCell = meOffset + dirOffset[dir];

            if (Attribute[CharToType[GET(*behindCell)]] & ATT_BLANK) {

                // Reserve the destination with CH_PLACEHOLDER -- looks blank (charSet[] maps
                // it to the same invisible glyph as CH_BLANK) but has no ATT_BLANK/PERMEABLE
                // etc of its own, so nothing else can walk into or spawn onto it while the
                // real object is still mid-carry. destCell remembers where to commit the real
                // CH_IMMOVABLE once the walk-in glide finishes (movePlayer(), below).
                *behindCell = CH_PLACEHOLDER;

                attachments[SLOT_ACTION].type = CH_IMMOVABLE;
                attachments[SLOT_ACTION].mode = ATTACH_SHOVE;
                attachments[SLOT_ACTION].destCell = behindCell;

                playerX += xdir[dir];
                playerY += ydir[dir];

                moveHusk(dir, cur->me, meOffset);

                if (playerAnimationID != ID_Push)
                    startPlayerAnimation(ID_Push);

                if (!autoMoveFrameCount) {

                    autoMoveFrameCount = (MOVE_SPEED << playerSlow);

                    autoMoveX = autoMoveDeltaX = animDeltaX[dir] >> playerSlow;
                    autoMoveY = autoMoveDeltaY = animDeltaY[dir] >> playerSlow;
                }

                handled = true;
            }

            // Shove blocked -- something solid immediately behind it -- same "did nothing"
            // feedback as the fire-button pickup-blocked case above, via the same action slot
            // instead of silently bumping into it. Shakes the immovable itself (meOffset), not
            // behindCell -- behindCell is whatever's blocking it and never moves.
            else
                shakeObject(SLOT_ACTION, meOffset, playerX + xdir[dir], playerY + ydir[dir], SHAKE_TICKS);
        }

        else if (Attribute[destType] & (ATT_BLANK | ATT_PERMEABLE | ATT_GRAB | ATT_EXIT)) {

            pushCounter = 0;

            if (Attribute[destType] & ATT_BLANK)
                ADDAUDIO(SFX_SPACE);

            // TYPE_DOOR_OPEN (an unlocked/opened door, mellon.c's updateDoorUnlock()/board.c's
            // CH_DOORCLOSED case) is deliberately a different type than TYPE_OUTBOX -- see its
            // own comment (attribute.h) -- but walking through one still has to finish the level
            // exactly like walking into the real TYPE_OUTBOX exit tile does, so both are checked
            // here.
            else if (destType == TYPE_OUTBOX || destType == TYPE_DOOR_OPEN) {

                *meOffset = CH_EXITBLANK;
                ADDAUDIO(SFX_WHOOSH);
                exitMode = EXIT_MODE_START;
                waitRelease = true;
                doorClosing = false;    // fresh exit sequence -- see its own comment

                // CH_EXITBLANK is TYPE_OUTBOX too, so from here it would otherwise keep rendering
                // via the shared Animate[TYPE_OUTBOX]/AnimFlashOut cycle -- flashing on/off every
                // 20 frames -- for the whole exit sequence, completely independent of exitMode.
                // Pin it to AnimFlashOut's first frame (the door glyph, not blank) and freeze it
                // there via haltCharAnimation() so it just sits still while the player walks off
                // and the screen fades. Same "animate in unison" caveat as everywhere else this
                // system is used -- any other TYPE_OUTBOX tile in the level freezes too, but the
                // level is ending regardless.
                startCharAnimation(TYPE_OUTBOX, AnimFlashOut);
                haltCharAnimation(TYPE_OUTBOX);

                // Same treatment as stepping onto a teleport tile (below): drop whatever's
                // carried on the tile the player is leaving, using the ordinary drop arc.
                // playerX/playerY are still the PRE-move position here (the shared
                // playerX/playerY += xdir/ydir[dir] below hasn't run yet for this branch), so
                // unlike teleport's version there's no subtraction needed to recover it -- just
                // record it directly, before it changes. drawAttachment() (draw.c) then draws
                // relative to this frozen exitDepartOriginX/Y, exactly like teleportLocked makes
                // it draw relative to teleportDepartOriginX/Y, instead of the live (and, for the
                // rest of the exit sequence, walk-in-gliding) playerX/playerY -- without that,
                // the drop arc's offsets were being added on top of a base position that kept
                // moving under it for the last few frames of the glide, reading as the item
                // jumping around instead of settling into a single dropped position.
                // SLOT_CARRY itself is untouched -- initPlayer() (gameState_Game.c's loadCave())
                // carries it into the next cave and arms doorCarryLiftTicks so it rises back
                // into carry pose there, same as a teleport arrival (see its own comment).
                if (attachments[SLOT_CARRY].type) {
                    exitDepartOriginX = playerX;
                    exitDepartOriginY = playerY;
                    attachments[SLOT_CARRY].offset = dropOffset[dir];
                    doorExitArmsCarryLift = true;
                }
            }

            //             else if (destType == TYPE_FLIP_GRAVITY) {
            //                 nextGravity = -gravity;
            //                 FLASH(0xC5, 3);
            // #if ENABLE_SHAKE
            //                 setShake(20);
            // #endif
            //             }

            if (Attribute[destType] & ATT_GRAB) {
                grabDoge();
                nDots(10, playerX + xdir[dir], playerY + ydir[dir], PT_SPIRAL2, 40, 3, 4, 50, 7);
            }


            playerX += xdir[dir];
            playerY += ydir[dir];

            frameAdjustX = frameAdjustY = 0;

            // Teleport tile: leave the destination square exactly as-is (don't drop a
            // CH_MELLON_HUSK on top of it) so it keeps showing its RAM-static glyph while
            // the player is standing on it -- see board.c's restartBoardScan(), which is
            // told to stop treating "not a husk square" as a death here via teleportLocked.
            if (!exitMode && destType != TYPE_TELEPORT) {

                moveHusk(dir, cur->me, meOffset);
            }

            // Fix bar stuff

            if (theCave->weapon[level] == WEAPON_PIPE) {
                if (!(inpt4 & 0x80)) {

                    int udlr = 0;

                    static const unsigned char udlrChar[] = {

                        // 1 = up
                        // 2 = right
                        // 4 = down
                        // 8 = left

                        CH_HORIZONTAL_BAR,    // 00
                        CH_VERTICAL_BAR,      // 01 U
                        CH_HORIZONTAL_BAR,    // 02 R
                        CH_HUB_1,             // 03 UR
                        CH_VERTICAL_BAR,      // 04 D
                        CH_VERTICAL_BAR,      // 05 UD
                        CH_HUB_1,             // 06 RD
                        CH_HUB_1,             // 07 URD
                        CH_HORIZONTAL_BAR,    // 08 L
                        CH_HUB_1,             // 09 UL
                        CH_HORIZONTAL_BAR,    // 10 RL
                        CH_HUB_1,             // 11 URL
                        CH_HUB_1,             // 12 LD
                        CH_HUB_1,             // 13 UDL
                        CH_HUB_1,             // 14 RDL
                        CH_HUB_1,             // 15 URDL
                    };

                    for (int d = 0; d < 4; d++) {
                        if ((ATTRIBUTE_BIT(*(cur->me + dirOffset[d]), ATT_PIPE)) ||
                            GET(*(cur->me + dirOffset[d])) == CH_MELLON_HUSK)
                            udlr |= 1 << d;
                    }

                    *cur->me = udlrChar[udlr];

                    showTool = true;
                }
            }

            playerSlow = 0;
            if (!autoMoveFrameCount && ((Attribute[destType] & (ATT_DIRT | ATT_WATERFLOW)) || destType == TYPE_LAVA)) {

                ADDAUDIO(SFX_DIRT);
                startCharAnimation(TYPE_MELLON_HUSK, AnimateBase[TYPE_MELLON_HUSK]);
            }

            int dir2 = (gravity < 0) ? dir ^ 2 : dir;

            if (playerAnimationID != WalkAnimation[dir2])
                startPlayerAnimation(WalkAnimation[dir2]);

            if (!autoMoveFrameCount) {

                autoMoveFrameCount = (MOVE_SPEED << playerSlow);

                autoMoveX = autoMoveDeltaX = animDeltaX[dir] >> playerSlow;
                autoMoveY = autoMoveDeltaY = animDeltaY[dir] >> playerSlow;
            }

            handled = true;
        }
    }

    // Stepping onto a teleport tile locks the player there -- board.c's TYPE_MELLON_HUSK
    // case stops calling movePlayer() the instant teleportLocked is set, so there's no
    // "cancel by walking away" to handle here: this function (and its caller) simply won't
    // run again for this player until the lock clears. Checked here rather than inline where
    // destType == TYPE_TELEPORT is handled above because both this move and the TYPE_STAR
    // grab above it reach this same point having actually moved the player onto meOffset.
    if (handled && destType == TYPE_TELEPORT) {
        teleportLocked = true;
        teleportCountingDown = false;
        startTeleportDepartSwirl();    // spirals start now, not once the walk-in glide settles

        // Any door this player unlocked with a key/doges in THIS cave gets locked again the
        // instant they step onto a teleport, same as decodeCave() would silently reset it on
        // any fresh load -- but doing it here too, visibly, on the way out, means a returning
        // player sees *why* it's shut instead of just finding it that way and wondering if they
        // imagined opening it. Harmless if there's no CH_DOOROPEN_STATIC cell in the cave at
        // all -- board.c's CH_DOOROPEN_STATIC case is the one that actually commits each such
        // cell once this finishes, this just kicks off the shared visual (and its own copy of
        // the same open-side cue, ADDAUDIO(SFX_DOOR)) unconditionally, same as the open
        // trigger does. Plenty of time to finish well before the fade-to-black completes --
        // this is a much shorter sequence than TELEPORT_SWIRL_TICKS.
        startCharAnimation(TYPE_DOOR_OPEN, AnimateDoorClose + 2);
        ADDAUDIO(SFX_DOOR);

        // Carried object (if any) "dropped" in the direction just walked, using the exact same
        // table a real fire-button drop uses -- SLOT_CARRY itself is left untouched (no board
        // write, nothing cleared). playerX/playerY are already updated to the destination
        // (teleport) tile by this point, so the origin tile is recovered by subtracting this
        // move back off -- drawAttachment() (draw.c) draws relative to that recorded origin,
        // not playerX/playerY, so the result is pixel-identical to a real stationary drop.
        // Arrival is unrelated -- initPlayer() (called by loadCave(), well before arrival's
        // reveal) already resets the offset to 0 on its own, and teleportCarryLift() (draw.c)
        // takes over from there.
        if (attachments[SLOT_CARRY].type) {
            teleportDepartOriginX = playerX - xdir[dir];
            teleportDepartOriginY = playerY - ydir[dir];
            attachments[SLOT_CARRY].offset = dropOffset[dir];
        }
    }

    if (handled) {
        bufferedSWCHA |= joyBit;
        usableSWCHA |= joyBit;
        inhibitSWCHA = joyBit;
    }

    return handled;
}

bool checkLowPriorityMove(BoardCursor *cur, int dir) {

    unsigned char joyBit = joyDirectBit[dir] << 4;
    if (usableSWCHA & joyBit) {
        return false;
    }

    int offset = dirOffset[dir];
    unsigned char *meOffset = cur->me + offset;
    enum ObjectType destType = CharToType[GET(*meOffset)];

#if 1    // disable push
    if ((!(theCave->flags & CAVEDEF_STAR_STATIC)) && (Attribute[destType] & (ATT_MINE | ATT_PIPE))) {

        if (++pushCounter > 1) {

            int anim = mineAnimation[dir];
            if (!(dir & 1) && gravity < 0)
                anim ^= ID_MineDown ^ ID_MineUp;

            if (playerAnimationID != anim)
                startPlayerAnimation(anim);

            if (destType == TYPE_INSULATOR)
                ADDAUDIO(SFX_ZAP2);


        } else {
            ADDAUDIO(SFX_SPACE);
        }

        if (pushCounter > 8) {


            static const signed char xOffset[] = {0, CHAR_TRIX_X, 0, -CHAR_TRIX_X};
            static const signed char yOffset[] = {-(CHAR_CENTER_Y >> 1), 0, CHAR_CENTER_Y >> 1, 0};

            if (Attribute[destType] & ATT_MINE) {

                addScore(VALUE_BREAK_GEODE);

                if (ATTRIBUTE_BIT(*meOffset, ATT_GEODOGE)) {
                    *meOffset = FLAG(CH_CONVERT_GEODE_TO_DOGE);
                    ADDAUDIO(SFX_UNCOVER);
                } else
                    *meOffset = CH_DUST_ROCK_0;


                if (destType == TYPE_ROCK) {


                    ADDAUDIO(SFX_ROCK);

                    nDots(10, playerX, playerY, PT_ONE, 30, xOffset[dir] + CHAR_CENTER_X, yOffset[dir] + CHAR_CENTER_Y,
                          40, 2);
                } else {
                    ADDAUDIO(SFX_DOGE);

                    nDots(6, playerX, playerY, PT_TWO, 30, xOffset[dir] + CHAR_CENTER_X, yOffset[dir] + CHAR_CENTER_Y,
                          40, 7);
                }
            }

            else {
                *meOffset = FLAG(CH_CONVERT_PIPE);
            }

            waitForNothing = 1;
            startPlayerAnimation(ID_StandUp);

            pushCounter = 0;

            if (faceDirection > 0) {
                cur->me += 2;
                cur->col += 2;    // SKIP processing it!
            }
        }

        handled = true;
    }

    else

#endif
        startPlayerAnimation(ID_Locked);

    return handled;
}

void bubbles(int count, int dripX, int dripY, int age, int /*speed*/) {
    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX, dripY, PT_BUBBLE, age, 7);
        if (idx >= 0) {
            particle[idx].speed = 10;
            particle[idx].dir = 128 + rangeRandom(64) - 32;
        }
    }
}


void movePlayer(BoardCursor *cur) {

    if (kdelay)
        --kdelay;

    // digging's own release check -- inpt4 & 0x80 is the fire button NOT pressed (see its
    // established sense throughout checkHighPriorityMove()'s own fire-button block). Cleared
    // before the movement-dispatch gate below is evaluated so movement (and the existing
    // ID_Mine*-to-standing reset already in the "switch back to standing" block, further down)
    // resumes the very same frame the button comes up, not one frame late.
    if (digging && (inpt4 & 0x80))
        digging = false;

    handled = false;


    if (drop) {

        if (attachments[SLOT_CARRY].type == CH_BOMB)
            startCharAnimation(TYPE_BOMB, AnimateBomb + 2);

        attachments[SLOT_CARRY].offset = 0;
        *meAtt = dropSkipThisFrame ? FLAG(attachments[SLOT_CARRY].type) : attachments[SLOT_CARRY].type;
        drop = false;

        if (Attribute[CharToType[attachments[SLOT_CARRY].type]] & ATT_MASSIVE) {

            int attBelow = Attribute[CharToType[GET(*(meAtt + _BOARD_COLS))]];

            // Sound plays for landing on ANY solid, non-squashable surface -- dirt/wall/steel
            // included, not just another massive object.
            if (!(attBelow & ATT_BLANK) && !(attBelow & ATT_SQUASHABLE_TO_BLANKS))
                ADDAUDIO(attBelow & ATT_HARD ? SFX_ROCK : SFX_ROCK2);

            // Shake is narrower than sound: a DELIBERATE drop gives a small, unconditional
            // thud as long as what's underneath is genuinely hard (dirt doesn't count, a
            // rock/wall/steel does) -- no chain-to-cracked-brick check here, that's only for
            // a rock that fell and landed on its own (board.c's CH_ROCK_FALLING case). The
            // player placed this on purpose; it shakes regardless of what it's sitting on top
            // of, same as it always has, just smaller now.
            if (attBelow & ATT_HARD)
                shakeTime += 2;
        }


        waitRelease = true;
        attachments[SLOT_CARRY].type = 0;

        return;
    }

    if (pulsePlayerColour) {
        nDots(2, playerX, playerY, PT_ONE, 25, CHAR_TRIX_X >> 1, CHAR_TRIX_Y >> 1, 100, 7);
        return;
    }


    // breath bubbles
    static int breath;
    if (showWater && playerY * CHAR_TRIX_Y > (liquidTrixel_8 >> 8)) {

        breath++;
        if (!(breath & 35) && (breath & 63) < 21) {
            int x = (playerX * 5) + 3;
            int y = (playerY * CHAR_TRIX_Y) + 4;
            bubbles(1, x - 1, y - 2, 400, 0x1000);
            ADDAUDIO(SFX_BUBBLER);
        }
    }

    else
        killAudio(SFX_BUBBLER);

    static unsigned char lastUsableSWCHA = 0;

    if (usableSWCHA != lastUsableSWCHA) {
        waitForNothing = 0;
    }

    if (autoMoveFrameCount)
        return;

    // The walk-in glide that started the shove (checkHighPriorityMove()'s ATT_SHOVE trigger,
    // below) has now fully settled -- commit the block for real into the square reserved with
    // CH_PLACEHOLDER back then, and let go of it. Must happen here, before checkHighPriorityMove
    // is given a chance to start a brand new move below: that call returns early on success
    // (skipping straight past the "switch back to standing" tail further down), so if a new
    // move started on the exact same frame the shove glide ended, the commit would otherwise be
    // skipped -- leaving the immovable attached and following the player through however many
    // subsequent moves it took before a frame finally passed with no new move beginning.
    if (attachments[SLOT_ACTION].mode == ATTACH_SHOVE && attachments[SLOT_ACTION].type) {

        // Check for support right here instead of just settling as CH_IMMOVABLE and waiting for
        // board.c's own ambient check (case CH_IMMOVABLE) to notice on its next scan pass: that
        // gap let the player push the block again before gravity ever got a look-in, since
        // checkHighPriorityMove() re-evaluates every single frame but the board scanner only
        // revisits this cell once per sweep -- a settled-looking, still-pushable CH_IMMOVABLE
        // could get shoved clean across a pit, one square at a time, without ever falling in.
        // Falling has to win that race, not input.
        unsigned char *destCell = attachments[SLOT_ACTION].destCell;
        unsigned char *below = destCell + _BOARD_COLS;
        if (Attribute[CharToType[GET(*below)]] & ATT_BLANK) {
            *below = FLAG(CH_IMMOVABLE_FALLING_BOTTOM);
            *destCell = FLAG(CH_IMMOVABLE_FALLING_TOP);
        } else
            *destCell = CH_IMMOVABLE;

        attachments[SLOT_ACTION].destCell = 0;
        attachments[SLOT_ACTION].type = 0;
    }

    lastUsableSWCHA = usableSWCHA;


    if (gearsWaitRelease && (usableSWCHA & 0xF0) == 0xF0)    // fixes gopher debug/exit 'lockup'
        gearsWaitRelease = false;


    // kdelay: see its own comment above -- freezes the player for a beat right after a pickup,
    // so no new move of any kind (including checkLowPriorityMove()'s ordinary walk) can start
    // until it expires. digging: same freeze, held for as long as the fire-button dig's grab
    // animation is up (see its own comment) rather than a fixed tick count.
    if (!kdelay && !digging) {

        for (int dir = 0; dir < 4; dir++)
            if (checkHighPriorityMove(cur, dir))
                return;

        for (int dir = 0; dir < 4 && !handled; dir++)
            if (checkLowPriorityMove(cur, dir))
                return;
    }

    // switch back to standing facing forward, turning if required

    if (!autoMoveFrameCount) {

        if (playerAnimationID == ID_WalkUp || playerAnimationID == ID_MineUp)
            startPlayerAnimation(ID_StandUp);

        else if (playerAnimationID == ID_Walk || playerAnimationID == ID_Mine || playerAnimationID == ID_Push)
            startPlayerAnimation(ID_StandLR);

        else if (playerAnimationID == ID_WalkDown || playerAnimationID == ID_MineDown)
            startPlayerAnimation(ID_Stand);
    }

    // after all movement checked, anything falling on player?
    // potential bug - if you're pushing and something falls on you

    if (Attribute[CharToType[*(cur->me - _BOARD_COLS * gravity)]] & ATT_CRUSHES) {
        startPlayerAnimation(ID_Die);
        return;
    }

    pushCounter = 0;
    idleTimer++;
}

// EOF