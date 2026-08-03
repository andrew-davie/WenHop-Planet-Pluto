
#include "defines_dasm.h"

#include "attribute.h"
#include "caveData.h"
#include "decodeCaves.h"
#include "main.h"
#include "random.h"

#define DIRT CH_DIRT
#define STEEL CH_STEELWALL

#define R 2

#define LINER(char, x, y, length, direction) DRAW_LINE, char, x, y, direction, length,


const unsigned char P1_caveUseWall[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    3,17,
    BOARD_TRIX_X-3,17,
    0x46, 0xE8, 0xEA,               // palette

    20, 4,4,                            // milling
    10, 15,                         // doge $
    0,                              // shake
    0,                              // water

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
    CH_INSULATOR_R, 18,5,


    DRAW_RECT, CH_CONCRETE, 1,1,1,12,

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


const unsigned char caveraintest[] = {
    // clang-format off

    7,17,7,17,

    0x98, 0x26, 0xC6,               // palette

    60,  4,4,                           // milling
    10, 15,                         // doge $
    255,                              // weather (255 = storms)
    17 + SCREEN_TRIX_Y,                              // water

     17,  11,  50,  56,  8,         // randomiser[level]
     30,  12,  12,  12,  12,        // doge req
    200, 200, 200, 200, 200,

    0, //WEAPON_MACE,                    // 0
    0, //WEAPON_MACE,                    // 1
    0, //WEAPON_MACE,                    // 2
    0, //WEAPON_MACE,                    // 3
    0, //WEAPON_MACE,                    // 4

//    CAVEDEF_LOCK_Y,
    0, CH_BRICKWALL, CH_DIRT,           // flags, border, fill

    // Random objects

    1,
    // CH_DIRT,10,20,20,20,20,
    // CH_GEODOGE, 80,40,40,40,40,
    CH_ROCK, 50,40,40,40,40,

    DRAW_RECT, CH_STEELWALL, 1,1,9,8,
    CH_TELEPORT, 6, 4,

    // CH_BOMB, 2, 6,

    CH_DOOROPEN_0, 5, 6,
    CH_MELLON_HUSK_BIRTH, 2, 4,
    CH_TELEPORT, 8, 3,
    CH_IMMOVABLE, 6,2,

    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,

    'T', 'E', 'S', 'T', END_STRING

    // clang-format on
};


const unsigned char cavetest[] = {
    // clang-format off

    7,17,7,17,                      // scroll bounds -- locked single screen (see P0_caveNew), so the whole
                                     // cave is visible at once instead of needing to scroll to find things

    0x98, 0x26, 0xC6,               // palette

    255,  22, 50,                           // milling
    10, 15,                         // doge $
    4,                              // weather (rain -- see particle.c's makeRain()) -- was 1 under the old hardcoded-frequency scheme; 4 keeps the same intensity now that this value IS the frequency divisor
    0,                              // water

     0,  0,  0,  0,  0,         // randomiser[level]
     30,  12,  12,  12,  12,        // doge req
    200, 200, 200, 200, 200,

    WEAPON_MACE,                    // 0
    WEAPON_MACE,                    // 1
    WEAPON_MACE,                    // 2
    WEAPON_MACE,                    // 3
    WEAPON_MACE,                    // 4

    0, CH_BRICKWALL, CH_GEODOGE,           // flags, border, fill

    // Random objects

    3,
    CH_DIRT,10,20,20,20,20,
    CH_GEODOGE, 80,40,40,40,40,
    CH_ROCK, 50,40,40,40,40,


    CH_DOOROPEN_0, 5, 6,
    CH_MELLON_HUSK_BIRTH, 5, 2,
    CH_TELEPORT, 2, 4,
    CH_TELEPORT, 8, 7,


    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,

    'T', 'E', 'S', 'T', END_STRING

    // clang-format on
};


// Blank/empty filler cave -- same small (locked single-screen) format as caveraintest, but
// stripped down to just a border and an empty interior, no random fill, no hazards, no
// CH_TELEPORT of its own. Used to pad caveList[] out to a full 10 planets x 10 levels
// (100 entries) below, standing in for every level slot that doesn't have real content
// authored yet -- every one of those slots points at this same array.
const unsigned char caveBlank[] = {
    // clang-format off

    7,17,7,17,                      // scroll bounds -- locked single screen, same small format as caveraintest

    0x98, 0x26, 0xC6,               // palette

    60,  4,4,                           // milling
    10, 15,                         // doge $
    0,                              // weather (off)
    0,                              // water (off)

     0,  0,  0,  0,  0,             // randomiser[level]
     10,  0,  0,  0,  0,             // doge req -- nothing to collect in an empty room
    200, 200, 200, 200, 200,        // time to complete

    0, //WEAPON_MACE,                    // 0
    0, //WEAPON_MACE,                    // 1
    0, //WEAPON_MACE,                    // 2
    0, //WEAPON_MACE,                    // 3
    0, //WEAPON_MACE,                    // 4

    0, CH_BRICKWALL, CH_BLANK,           // flags, border, fill

    // Random objects

    0,

    DRAW_RECT, CH_STEELWALL, 1,1,9,8,
    CH_MELLON_HUSK_BIRTH, 5, 2,
    CH_TELEPORT, 4, 4,
    CH_DOORCLOSED, 5, 6,           // own door, inside the steel-walled interior -- must be
                                    // authored explicitly: decodeCave()/StoreObject() (decodeCaves.c)
                                    // only ever update doorX/doorY when a cave places a door of its
                                    // own, so a door-less cave otherwise inherits whatever door
                                    // position the PREVIOUS cave left behind and DECODE_FLASH stamps
                                    // a phantom door there. caveraintest/P0_caveNew's doors both
                                    // happen to land at (5,6) too, which is inside this cave's own
                                    // interior -- that's why planets A/B's caveBlank levels looked
                                    // fine while planet C's (whose real cave's door is at (38,5),
                                    // well outside this interior) didn't.


    CH_KEY, 7,6,

    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,
    DRAW_EOF,

    'T', 'E', 'S', 'T', END_STRING

    // clang-format on
};

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

const unsigned char P0_caveWater[] = {
    // clang-format off

    7,17,7,17,

    0x98, 0x26, 0xC6,               // palette

    60,  4,4,                           // milling
    10, 15,                         // doge $
    255,                              // weather (255 = storms)
    17+SCREEN_TRIX_Y,                // water -- this cave is locked to bounds_t == 17 (see resetTracking() in scroll.c), so just offscreen is bounds_t + SCREEN_TRIX_Y

     17,  11,  50,  56,  8,         // randomiser[level]
     30,  12,  12,  12,  12,        // doge req
    200, 200, 200, 200, 200,

    0, //WEAPON_MACE,                    // 0
    0, //WEAPON_MACE,                    // 1
    0, //WEAPON_MACE,                    // 2
    0, //WEAPON_MACE,                    // 3
    0, //WEAPON_MACE,                    // 4

//    CAVEDEF_LOCK_Y,
    0, CH_BRICKWALL, CH_BRICKWALL,           // flags, border, fill

    // Random objects

    3,
    CH_DIRT,10,20,20,20,20,
    CH_GEODOGE, 80,40,40,40,40,
    CH_ROCK, 50,40,40,40,40,

    DRAW_FILLED_RECT, CH_STEELWALL, 1,1,9,8, CH_ROCK,

    DRAW_FILLED_RECT, CH_GEODOGE, 30, 2, 10, 5, CH_GEODOGE,


    DRAW_FILLED_RECT, CH_BLANK, 3, 2, 5, 4, CH_BLANK,
    DRAW_FILLED_RECT, CH_BRICKWALL, 3, 5, 5, 4, CH_STAR,

    CH_CRACKED_BRICK, 4,5,
    CH_CRACKED_BRICK, 5,5,
    CH_CRACKED_BRICK, 6,5,

    CH_DOOROPEN_0, 5, 6,
    CH_MELLON_HUSK_BIRTH, 5, 2,

   CH_BOMB,2,7,


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
// DEV RIG: exercises every CH_* that (a) is still _untimed_ in board.c's budget[]
// table and (b) actually has case-statement handling in board.c (via processTypes'
// switch(type) or processCreatures' switch(creature)) -- so DEBUG_TIMES gets a real
// sample instead of the 12500 placeholder. Objects with a case but no ATT_PHASE*
// bit in Attribute[] (CH_DOOROPEN_0/CH_EXITBLANK via TYPE_OUTBOX, CH_ROCK_BONUS via
// TYPE_ROCK_BONUS) are never selected by processBoardSquares()'s phase gate, so
// their case is unreachable there regardless -- deliberately left off this rig.
// Installed as caveList[0] in place of P0_caveWater (still defined above, untouched
// -- swap the caveList[] entry back to restore it).
const unsigned char P0_caveChTiming[] = {
    // clang-format off

    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,   // scroll bounds -- full board, free-scrolling

    0x98, 0x26, 0xC6,               // palette

    20, 4,4,                        // milling
    10, 15,                         // doge $
    0,                              // weather -- off, keep the board quiet
    0,                              // water -- off; also keeps liquidTrixel_8 at its
                                     // off-board sentinel, which is what lets the
                                     // CH_WATERFLOW_0..4 cascade below keep extending
                                     // (processWaterFlow only advances while
                                     // line < liquidTrixel_8>>8)

    0, 0, 0, 0, 0,                  // randomiser[level] -- non-deterministic
    0, 0, 0, 0, 0,                  // doge req -- 0 so the door opens immediately
    200, 200, 200, 200, 200,        // time -- generous, this cave isn't meant to be "won"

    WEAPON_MACE,                    // 0
    WEAPON_MACE,                    // 1
    WEAPON_MACE,                    // 2
    WEAPON_MACE,                    // 3
    WEAPON_MACE,                    // 4

    CAVEDEF_START_WITH_WEAPON, CH_STEELWALL, CH_BLANK,    // flags, border, fill

    0,    // no random per-cell objects -- every placement below is explicit/deterministic

    CH_MELLON_HUSK_BIRTH, 2, 2,
    CH_DOOROPEN_0, 2, 20,

    // row 3 -- CH_PEBBLE1, CH_PEBBLE2 (TYPE_PEBBLE1), CH_DOGE_STATIC (TYPE_DOGE),
    //          CH_PEBBLE_ROCK, CH_ROCK_PEBBLE
    CH_PEBBLE1, 3, 3,
    CH_PEBBLE2, 10, 3,
    CH_DOGE_STATIC, 17, 3,
    CH_PEBBLE_ROCK, 24, 3,
    CH_ROCK_PEBBLE, 31, 3,

    // row 6 -- CH_ROCK_PEBBLE_1, and the four CH_PUSH_LEFT/RIGHT(+REVERSE) pushers
    // (blank neighbour in the push direction, from interiorCharacter, is enough to
    // exercise genericPush()'s/genericPushReverse()'s live branch)
    CH_ROCK_PEBBLE_1, 3, 6,
    CH_PUSH_LEFT, 10, 6,
    CH_PUSH_LEFT_REVERSE, 17, 6,
    CH_PUSH_RIGHT, 24, 6,
    CH_PUSH_RIGHT_REVERSE, 31, 6,

    // row 9 -- CH_PUSH_UP/DOWN(+REVERSE), CH_BLOCK (needs blank below -> falls once)
    CH_PUSH_UP, 3, 9,
    CH_PUSH_UP_REVERSE, 10, 9,
    CH_PUSH_DOWN, 17, 9,
    CH_PUSH_DOWN_REVERSE, 24, 9,
    CH_BLOCK, 31, 9,

    // row 11 -- CH_ROCK anchors (already timed; ATT_CONVEYOR, and stable since
    // TYPE_GRINDER/TYPE_BELT aren't ATT_BLANK) so the grinders/belts below actually
    // have something to feed instead of idling in their do-nothing branch
    CH_ROCK, 3, 11,
    CH_ROCK, 10, 11,
    CH_ROCK, 17, 11,
    CH_ROCK, 24, 11,

    // row 12 -- CH_GRINDER_0/1, CH_BELT_0/1, CH_CONVERT_PIPE
    // NOTE: StoreObject() force-parities a literal CH_GRINDER_0 placement to
    // CH_GRINDER_1 when (x+y) is even ("ensure parity on gears") -- (3+12)=15 is
    // odd, so this one lands as authored. The second slot is written directly as
    // CH_GRINDER_1, which isn't subject to that override.
    CH_GRINDER_0, 3, 12,
    CH_GRINDER_1, 10, 12,
    CH_BELT_0, 17, 12,
    CH_BELT_1, 24, 12,
    CH_CONVERT_PIPE, 31, 12,

    // row 15 -- CH_DOGE_FALLING_TOP2 (blank below it, from interiorCharacter, is all
    // it needs), CH_STAR_EXPLODE (self-paced by the shared Animate[] cycle)
    CH_DOGE_FALLING_TOP2, 3, 15,
    CH_STAR_EXPLODE, 10, 15,

    // col 36 -- CH_OUTLET seeds CH_WATERFLOW_0 (both carry ATT_WATERFLOW, satisfying
    // processWaterFlow()'s "above must be flow-carrying" check), which then rolls
    // itself downward through _1/_2/_3/_4 one step per advance as it falls down the
    // clear column beneath -- covers all five untimed CH_WATERFLOW_* from one seed
    CH_OUTLET, 36, 1,
    CH_WATERFLOW_0, 36, 2,

    DRAW_EOF,

    // no per-level extras -- everything above applies at every level (level is
    // always 0 in practice; see gameState_Menu.c)
    DRAW_EOF,    // LEVEL 0
    DRAW_EOF,    // LEVEL 1
    DRAW_EOF,    // LEVEL 2
    DRAW_EOF,    // LEVEL 3
    DRAW_EOF,    // LEVEL 4

    'C', 'H', 'T', 'E', 'S', 'T', END_STRING

    // clang-format on
};

//------------------------------------------------------------------------------
// "VAULT BREACH" -- purpose-built level, installed as caveList[0] in place of the
// CH_ timing rig above (still defined, untouched, easily restored).
//
// interiorCharacter is CH_DIRT, not CH_BLANK -- the visible window is only
// SCREEN_TRIX_X x SCREEN_TRIX_Y (40x66 trix, ~8x6.6 board cells) at a time, so an
// open BOARD_TRIX_X x BOARD_TRIX_Y (40x22-cell) board reads as long stretches of
// dead black if left blank. Dirt is ATT_PERMEABLE (walks/digs like open floor,
// mellon.c's moveHusk) but renders as ground, so the whole board stays visibly
// textured; CH_BLANK is used only where a room deliberately needs to read as
// open space (the vault interior). A light CH_ROCK/CH_GEODOGE scatter (randomiser
// block below) adds organic variation and a few extra optional mining targets on
// top of that base texture, same recipe the real planet caves use.
//
// Three rooms, west to east:
//
//  1. MINING YARD (cols 2-11) -- spawn, plus two CH_ROCK at (5,4)/(5,5) blocking
//     the CH_BOMB at (8,4). Contact-pushing into a rock is what mines it (not a
//     held action separate from movement -- checkLowPriorityMove's ATT_MINE path
//     counts consecutive frames of moving into it, mellon.c) -- rocks don't mine
//     themselves, the player mining them is what a joystick move into one does.
//     Three more CH_GEODOGE (4,7)/(9,3)/(10,8) are optional mining side-targets:
//     each becomes a doge in its own right, on top of the vault's cache below.
//
//  2. CHARGED CORRIDOR (cols 13-20) -- CH_BRICKWALL top/bottom framing (rows 2
//     and 10) around three CH_INSULATOR_TOP/BOTTOM pairs (cols 14/16/18,
//     rows 3/9), same idiom P1_caveUseWall uses. The gap between a pair arcs on
//     and off in a travelling wave (setInsulatorPattern, board.c); crossing means
//     timing the gaps, or ducking into one of the untouched lanes (13/15/17/19).
//     Carry the bomb through here -- the detour is the point.
//
//  3. THE VAULT (cols 22-37) -- CH_BRICKWALL partition at col 22, breached only
//     by bombing its 3-tall CH_CRACKED_BRICK weak point at (22, 5..7); plain
//     brickwall isn't ATT_EXPLODABLE (explode(), board.c), so there's no way
//     through except carrying the bomb the whole way from room 1. Drop it at
//     (21,6) facing right -> it lands at (22,6); the blast's 3x3 footprint
//     (cols 21-23, rows 5-7) covers all three cracked-brick cells, and the
//     player's own cell is ATT_EXPLODABLE too, so retreat before the fuse
//     (AnimateBomb) burns out. Rubble decays to CH_BLANK a couple of frames
//     later. Inside, cleared to open CH_BLANK floor: a CH_CONVERT_GEODE_TO_DOGE
//     seed at (27,5) has been chain-reacting through five adjacent CH_GEODOGE
//     (28-32,5) since the level started (see its case, board.c) -- 6 doges
//     waiting there alone. Door's at (34,5).
//
// dogeRequired is 5 -- covered by the vault cluster by itself with one spare, so
// the level is always finishable via the designed path; the three yard geodoges
// are pure bonus for a thorough player. water is set low and barely rising (see
// below) -- background tension, not a hard gate on any of the above.
const unsigned char P0_caveVaultBreach[] = {
    // clang-format off

    0,0,BOARD_TRIX_X,BOARD_TRIX_Y,   // scroll bounds -- full board, free-scrolling

    0x98, 0x26, 0xC6,               // palette

    20, 4,4,                        // milling
    10, 15,                         // doge $
    0,                              // weather -- clean, no distractions
    BOARD_TRIX_Y - 15,              // water -- starts almost at the very bottom of
                                     // the board and creeps up slowly; ambient
                                     // pressure, not a gate on the route above

    0, 0, 0, 0, 0,                  // randomiser[level] -- non-deterministic
    5, 5, 5, 5, 5,                  // doge req -- 5 of the 6 vault doges (3 more
                                     // available as yard bonus, uncounted)
    200, 200, 200, 200, 200,        // time

    WEAPON_MACE,                    // 0
    WEAPON_MACE,                    // 1
    WEAPON_MACE,                    // 2
    WEAPON_MACE,                    // 3
    WEAPON_MACE,                    // 4

    0, CH_STEELWALL, CH_DIRT,       // flags, border, fill -- dirt, see note above

    // light organic scatter on top of the dirt fill -- same recipe the real
    // planet caves use (e.g. P1_caveUseWall)
    2,
    CH_ROCK, 25,25,25,25,25,
    CH_GEODOGE, 15,15,15,15,15,

    CH_MELLON_HUSK_BIRTH, 2, 4,

    // room 1 -- mining yard
    CH_ROCK, 5, 4,
    CH_ROCK, 5, 5,
    CH_BOMB, 8, 4,
    CH_GEODOGE, 4, 7,
    CH_GEODOGE, 9, 3,
    CH_GEODOGE, 10, 8,

    // room 2 -- charged corridor
    DRAW_LINE, CH_BRICKWALL, 13, 2, 2, 8,
    DRAW_LINE, CH_BRICKWALL, 13, 10, 2, 8,
    CH_INSULATOR_TOP, 14, 3,
    CH_INSULATOR_BOTTOM, 14, 9,
    CH_INSULATOR_TOP, 16, 3,
    CH_INSULATOR_BOTTOM, 16, 9,
    CH_INSULATOR_TOP, 18, 3,
    CH_INSULATOR_BOTTOM, 18, 9,

    // room 3 -- the vault: partition, weak point, cleared interior, cache, door
    DRAW_LINE, CH_BRICKWALL, 22, 1, 4, 20,
    CH_CRACKED_BRICK, 22, 5,
    CH_CRACKED_BRICK, 22, 6,
    CH_CRACKED_BRICK, 22, 7,

    DRAW_FILLED_RECT, CH_BLANK, 24, 2, 13, 8, CH_BLANK,

    CH_CONVERT_GEODE_TO_DOGE, 27, 5,
    CH_GEODOGE, 28, 5,
    CH_GEODOGE, 29, 5,
    CH_GEODOGE, 30, 5,
    CH_GEODOGE, 31, 5,
    CH_GEODOGE, 32, 5,

    CH_DOOROPEN_0, 34, 5,

    DRAW_EOF,

    // no per-level extras -- everything above applies at every level (level is
    // always 0 in practice; see gameState_Menu.c)
    DRAW_EOF,    // LEVEL 0
    DRAW_EOF,    // LEVEL 1
    DRAW_EOF,    // LEVEL 2
    DRAW_EOF,    // LEVEL 3
    DRAW_EOF,    // LEVEL 4

    'V', 'A', 'U', 'L', 'T', END_STRING

    // clang-format on
};

//------------------------------------------------------------------------------


const unsigned char P0_caveNew[] = {
    // clang-format off

    7,17,7,17,

    0x98, 0x26, 0xC6,               // palette

    60,  4,4,                           // milling
    10, 15,                         // doge $
    255,                              // weather (255 = storms)
    17 + SCREEN_TRIX_Y,                              // water

     17,  11,  50,  56,  8,         // randomiser[level]
     30,  12,  12,  12,  12,        // doge req
    200, 200, 200, 200, 200,

    0, //WEAPON_MACE,                    // 0
    0, //WEAPON_MACE,                    // 1
    0, //WEAPON_MACE,                    // 2
    0, //WEAPON_MACE,                    // 3
    0, //WEAPON_MACE,                    // 4

//    CAVEDEF_LOCK_Y,
    0, CH_BRICKWALL, CH_BRICKWALL,           // flags, border, fill

    // Random objects

    3,
    CH_DIRT,10,20,20,20,20,
    CH_GEODOGE, 80,40,40,40,40,
    CH_ROCK, 50,40,40,40,40,

    DRAW_FILLED_RECT, CH_STEELWALL, 1,1,9,8, CH_ROCK,

    DRAW_FILLED_RECT, CH_GEODOGE, 30, 2, 10, 5, CH_GEODOGE,


    DRAW_FILLED_RECT, CH_BLANK, 3, 2, 5, 4, CH_BLANK,
    DRAW_FILLED_RECT, CH_BRICKWALL, 3, 5, 5, 4, CH_STAR,

    CH_CRACKED_BRICK, 4,5,
    CH_CRACKED_BRICK, 5,5,
    CH_CRACKED_BRICK, 6,5,

    CH_DOOROPEN_0, 5, 6,
    CH_MELLON_HUSK_BIRTH, 5, 2,

    CH_BOMB,2,7,


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


const unsigned char caveNew2[] = {
    // clang-format off

    // // scroll bounds (TL(x,y), BR(x,y) in trixels)
    // 0,0,
    // BOARD_TRIX_X-SCREEN_TRIX_X,BOARD_TRIX_Y - SCREEN_TRIX_Y,
    7,17,7,17,

    20,                             // milling
    10, 15,                         // doge $
    0,                              // shake
    0,                              // water

     17,  11,  50,  56,  8,         // randomiser[level]
     30,  12,  12,  12,  12,        // doge req
    200, 200, 200, 200, 200,

    WEAPON_MACE,                    // 0
    WEAPON_MACE,                    // 1
    WEAPON_MACE,                    // 2
    WEAPON_MACE,                    // 3
    WEAPON_MACE,                    // 4

//    CAVEDEF_LOCK_Y,
    0, CH_BRICKWALL, CH_DIRT,           // flags, border, fill

    // Random objects

    3,
    CH_DIRT,10,20,20,20,20,
    CH_GEODOGE, 80,40,40,40,40,
    CH_ROCK, 50,40,40,40,40,


    // CH_INSULATOR_L, 4,4,
    // CH_INSULATOR_R, 8,4,
    // Start of cave draw


    // CH_ROCK_BONUS, 7,3,

    // DRAW_RECT,CH_BRICKWALL, 0,1,40,8,

    // CH_INSULATOR_TOP, 6,2,
    // CH_INSULATOR_BOTTOM, 6,7,


    // CH_INSULATOR_TOP, 11,2,
    // CH_INSULATOR_BOTTOM, 11,7,

    // CH_BRICKWALL, 14,2,
    // CH_INSULATOR_TOP, 14,3,
    // CH_INSULATOR_BOTTOM, 14,6,
    // CH_BRICKWALL, 14,7,


    // CH_INSULATOR_TOP, 17,2,
    // CH_INSULATOR_BOTTOM, 17,7,

    // CH_BRICKWALL, 20,2,
    // CH_INSULATOR_TOP, 20,3,
    // CH_INSULATOR_BOTTOM, 20,6,
    // CH_BRICKWALL, 20,7,

    // CH_INSULATOR_TOP, 23,2,
    // CH_INSULATOR_BOTTOM, 23,7,

    // CH_BRICKWALL, 26,2,
    // CH_INSULATOR_TOP, 26,3,
    // CH_INSULATOR_BOTTOM, 26,6,
    // CH_BRICKWALL, 26,7,

    // CH_INSULATOR_TOP, 29,2,
    // CH_INSULATOR_BOTTOM, 29,7,
    
    // CH_BRICKWALL, 32,2,
    // CH_BRICKWALL, 32,3,
    // CH_BRICKWALL, 32,4,
    // CH_INSULATOR_TOP, 32,5,
    // CH_INSULATOR_BOTTOM, 32,7,


    DRAW_RECT, CH_STEELWALL, 1,1,9,8,

    CH_BOMB,5,5,
    
    CH_DOOROPEN_0, 5, 6,
    CH_MELLON_HUSK_BIRTH, 5, 2,

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

const unsigned char P2_caveWyrms[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    7,17,7,17,
    0xD6, 0x44, 0x76,               // palette


    20,  4,4,   // milling
    10, 15, // doge $
    0,          // weather
    0,          // water

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

    1,
    CH_BRICKWALL,110,110,110,110,110,

    DRAW_FILLED_RECT,CH_STEELWALL,1,1,9,8,CH_GEODOGE,
    DRAW_FILLED_RECT,CH_BRICKWALL,4,4,3,3,CH_ROCK,


    CH_DOORCLOSED, 5, 4,
    CH_MELLON_HUSK_BIRTH, 5, 5,

    CH_STAR, 5,7,

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
    0,      // water


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
    0,      // water


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
    CH_DOOROPEN_0, 20, 15,

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
    0,         // water


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
    BOARD_TRIX_Y+CHAR_TRIX_Y,  // water -- one character height under this cave's bottom bound


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
    0,      // water


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

    CH_DOOROPEN_0, 2, 2,

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
    0,      // water


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
    CH_DOOROPEN_0, 7, 11,


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
    0,     // water

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


const unsigned char P3_starsAndStripes[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    7,17,7,17,
    0x26, 0xB6, 0x54,               // palette

    20,    4,4,                         // milling
    1, 15,                          // doge $
    0,                              // rain
    0,                              // water

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


    CH_MELLON_HUSK_BIRTH, 5, 5,

    DRAW_EOF,


    'M', 'E', 'R', 'C', 'U', 'R', 'Y', END_STRING
    // clang-format on
};


const unsigned char P4_caveA4[] = {
    // clang-format off

    // scroll bounds (TL(x,y), BR(x,y) in trixels)
    43,17,BOARD_TRIX_X-53,17,
    0xA6, 0x16, 0xD6,               // palette

    20, 4,4,   // milling
    1, 15, // doge $
    0,     //              ,          // rain
    0,     // water

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


// 10 planets x 10 levels = 100 entries. One existing cave kept per planet (always at that
// planet's level 0 -- cavetest/P0_caveVaultBreach/P0_caveChTiming/caveNew2 stay retired,
// commented out above, not part of this); every other slot points at caveBlank (see its
// own comment) as a placeholder until real content is authored for it.
const struct caveHandler caveList[] = {

    // PLANET 0
    {caveraintest, none, 5, 2},    // level 0
    {caveBlank, none, 6, 5},       // level 1
    {caveBlank, none, 6, 5},       // level 2
    {caveBlank, none, 6, 5},       // level 3
    {caveBlank, none, 6, 5},       // level 4
    {caveBlank, none, 6, 5},       // level 5
    {caveBlank, none, 6, 5},       // level 6
    {caveBlank, none, 6, 5},       // level 7
    {caveBlank, none, 6, 5},       // level 8
    {caveBlank, none, 6, 5},       // level 9

    // PLANET 1
    {P0_caveNew, none, 5, 2},    // level 0
    {caveBlank, none, 6, 5},     // level 1
    {caveBlank, none, 6, 5},     // level 2
    {caveBlank, none, 6, 5},     // level 3
    {caveBlank, none, 6, 5},     // level 4
    {caveBlank, none, 6, 5},     // level 5
    {caveBlank, none, 6, 5},     // level 6
    {caveBlank, none, 6, 5},     // level 7
    {caveBlank, none, 6, 5},     // level 8
    {caveBlank, none, 6, 5},     // level 9

    // PLANET 2
    {P1_caveUseWall, none, 2, 3},    // level 0
    {caveBlank, none, 6, 5},         // level 1
    {caveBlank, none, 6, 5},         // level 2
    {caveBlank, none, 6, 5},         // level 3
    {caveBlank, none, 6, 5},         // level 4
    {caveBlank, none, 6, 5},         // level 5
    {caveBlank, none, 6, 5},         // level 6
    {caveBlank, none, 6, 5},         // level 7
    {caveBlank, none, 6, 5},         // level 8
    {caveBlank, none, 6, 5},         // level 9

    // PLANET 3
    {P2_caveWyrms, empty, 5, 5},    // level 0
    {caveBlank, none, 6, 5},        // level 1
    {caveBlank, none, 6, 5},        // level 2
    {caveBlank, none, 6, 5},        // level 3
    {caveBlank, none, 6, 5},        // level 4
    {caveBlank, none, 6, 5},        // level 5
    {caveBlank, none, 6, 5},        // level 6
    {caveBlank, none, 6, 5},        // level 7
    {caveBlank, none, 6, 5},        // level 8
    {caveBlank, none, 6, 5},        // level 9

    // PLANET 4
    {P3_starsAndStripes, spec, 2, 2},    // level 0
    {caveBlank, none, 6, 5},             // level 1
    {caveBlank, none, 6, 5},             // level 2
    {caveBlank, none, 6, 5},             // level 3
    {caveBlank, none, 6, 5},             // level 4
    {caveBlank, none, 6, 5},             // level 5
    {caveBlank, none, 6, 5},             // level 6
    {caveBlank, none, 6, 5},             // level 7
    {caveBlank, none, 6, 5},             // level 8
    {caveBlank, none, 6, 5},             // level 9

    // PLANET 5
    {P4_caveA4, none, 0, 0},    // level 0
    {caveBlank, none, 6, 5},    // level 1
    {caveBlank, none, 6, 5},    // level 2
    {caveBlank, none, 6, 5},    // level 3
    {caveBlank, none, 6, 5},    // level 4
    {caveBlank, none, 6, 5},    // level 5
    {caveBlank, none, 6, 5},    // level 6
    {caveBlank, none, 6, 5},    // level 7
    {caveBlank, none, 6, 5},    // level 8
    {caveBlank, none, 6, 5},    // level 9

    // PLANET 6
    {caveBlank, none, 6, 5},    // level 0
    {caveBlank, none, 6, 5},    // level 1
    {caveBlank, none, 6, 5},    // level 2
    {caveBlank, none, 6, 5},    // level 3
    {caveBlank, none, 6, 5},    // level 4
    {caveBlank, none, 6, 5},    // level 5
    {caveBlank, none, 6, 5},    // level 6
    {caveBlank, none, 6, 5},    // level 7
    {caveBlank, none, 6, 5},    // level 8
    {caveBlank, none, 6, 5},    // level 9

    // PLANET 7
    {caveBlank, none, 6, 5},    // level 0
    {caveBlank, none, 6, 5},    // level 1
    {caveBlank, none, 6, 5},    // level 2
    {caveBlank, none, 6, 5},    // level 3
    {caveBlank, none, 6, 5},    // level 4
    {caveBlank, none, 6, 5},    // level 5
    {caveBlank, none, 6, 5},    // level 6
    {caveBlank, none, 6, 5},    // level 7
    {caveBlank, none, 6, 5},    // level 8
    {caveBlank, none, 6, 5},    // level 9

    // PLANET 8
    {caveBlank, none, 6, 5},    // level 0
    {caveBlank, none, 6, 5},    // level 1
    {caveBlank, none, 6, 5},    // level 2
    {caveBlank, none, 6, 5},    // level 3
    {caveBlank, none, 6, 5},    // level 4
    {caveBlank, none, 6, 5},    // level 5
    {caveBlank, none, 6, 5},    // level 6
    {caveBlank, none, 6, 5},    // level 7
    {caveBlank, none, 6, 5},    // level 8
    {caveBlank, none, 6, 5},    // level 9

    // PLANET 9
    {caveBlank, none, 6, 5},    // level 0
    {caveBlank, none, 6, 5},    // level 1
    {caveBlank, none, 6, 5},    // level 2
    {caveBlank, none, 6, 5},    // level 3
    {caveBlank, none, 6, 5},    // level 4
    {caveBlank, none, 6, 5},    // level 5
    {caveBlank, none, 6, 5},    // level 6
    {caveBlank, none, 6, 5},    // level 7
    {caveBlank, none, 6, 5},    // level 8
    {caveBlank, none, 6, 5},    // level 9

};

const int caveCount = sizeof(caveList) / sizeof(caveList[0]);


// EOF
