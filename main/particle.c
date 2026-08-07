#include <stdbool.h>

#include "board.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "T1TC.h"
#include "animations.h"
#include "attribute.h"
#include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "draw.h"
#include "drawScreen.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "random.h"
#include "scroll.h"
#include "sound.h"

static unsigned int weaponLength = 0;

#define TOOL_MAX 60

struct TOOL {

    unsigned char age;
    unsigned char dir;
    int x;
    int y;
    unsigned short speed;
};


struct TOOL tool[TOOL_MAX];


int weapon;


const short sin_cos[32] = {
    // clang-format off
    // Combined sin/cos table
       0,   50,   98,  142,  181,  213,  237,  251,      //   0° (D) to <  90° (R)
     256,  251,  237,  213,  181,  142,   98,   50,      //  90° (R) to < 180° (U)
       0,  -50,  -98, -142, -181, -213, -237, -251,      // 180° (U) to < 270° (L)
    -256, -251, -237, -213, -181, -142,  -98,  -50,      // 270° (L) to <   0° (D)
    // clang-format on
};


void initTool() {
    weapon = 0;
    for (int i = 0; i < TOOL_MAX; i++)
        tool[i].age = 0;
}


void modifyCharAtTip(int x, int y) {

    int xchar = (x * (256 / CHAR_TRIX_X)) >> 16;
    int ychar = (y * (256 / CHAR_TRIX_Y)) >> 16;

    if (xchar < 0 || xchar >= _BOARD_COLS || ychar < 0 || ychar >= _BOARD_ROWS)
        return;

    unsigned char *b = RAM + _BOARD + ychar * _BOARD_COLS + xchar;
    unsigned char ch = *b;

    if (ch < FLAG_THISFRAME) {

        enum ObjectType type = CharToType[ch];
        if (Attribute[type] & ATT_EXPLODABLE) {

            unsigned char colour = 0;

            switch (type) {

            case TYPE_DOGE:
                doges--;
                *b = FLAG(CH_BLANK);
                colour = rangeRandom(7) + 1;
                break;

            case TYPE_ROCK:
                *b = FLAG(CH_GEODOGE);
                colour = 1;
                break;

            case TYPE_ROCK_BONUS:
                *b = FLAG(CH_STAR);
                colour = 7;
                break;

            case TYPE_GEODOGE:
                *b = FLAG(CH_DOGE_00);
                colour = 3;
                break;

            case TYPE_DIRT:
                *b = FLAG(CH_DUST_0);
                colour = 2;
                break;

            default:
                break;
            }

            if (*b >= FLAG_THISFRAME)
                nDotsAtTrixel(5, (x >> 8) + CHAR_CENTER_Y, (y >> 8) + CHAR_CENTER_Y, 30, PT_SPIRAL, 50, colour);
        }
    }
}

static const int PIXEL_ASPECT = 110;

unsigned char turn_toward(unsigned char current, unsigned char target, unsigned char speed) {
    if (current == target)
        return target;    // fix: early exit
    signed char diff = (signed char)(target - current);
    // speed must be <= 127; caller's responsibility or clamp here
    signed char s = (signed char)speed;    // safe only if speed <= 127
    if (diff > 0)
        return (diff <= s) ? target : (unsigned char)(current + s);
    else
        return (diff >= -s) ? target : (unsigned char)(current - s);
}


void drawMace() {

    if (playerDead || !(weapon & WEAPON_MACE))
        return;

    if ((inpt4 & 0x80) && !weaponLength) {
        return;
    }

    if (inpt4 & 0x80)
        weaponLength--;

    else if (weaponLength < ROPE_PARTICLE_COUNT)
        weaponLength++;

    int baseX = (playerX * CHAR_TRIX_X + CHAR_CENTER_X + 1 + ((faceDirection * autoMoveX) >> 2)) << 8;
    int baseY = ((playerY * CHAR_TRIX_Y + CHAR_CENTER_Y) << 8) + (autoMoveY * (256 / 3));

    int x = 0, y = 0;

    for (unsigned int i = 0; i < weaponLength; i++) {
        x += (sin_cos[(tool[i].dir >> 3) & 0x1F] * PIXEL_ASPECT) >> 8;
        y += (sin_cos[((tool[i].dir >> 3) + 8) & 0x1F] * 256) >> 8;

        // Don't underwrite the player's body - looks better
        if (x < -0x100 || x > 0x200 || y < -0x300 || y > 0x000)
            drawBit((baseX + x) >> 8, ((baseY + y) >> 8), 1);
    }

    struct {

        unsigned char x;
        unsigned char y;

    } ball[] = {

        {1, 0},                    //
        {0, 1}, {1, 1}, {2, 1},    //
        {0, 2}, {1, 2}, {2, 2},    //
        {0, 3}, {1, 3}, {2, 3},    //
        {0, 4}, {1, 4}, {2, 4},    //
        {1, 5},                    //

    };


    for (int i = 0; i < (int)(sizeof(ball) / sizeof(ball[0])); i++)
        drawBit(((baseX + x) >> 8) + ball[i].x - 1, ((baseY + y) >> 8) + ball[i].y - 2, 7);

    nDots(1, 0, 0, PT_TWO, 50, (baseX + x) >> 8, (baseY + y) >> 8, 20, 6);


    modifyCharAtTip(baseX + x, baseY + y);

    static unsigned char wantedDirection = 96;


    // Weapon runs randomly if player not locked
    // Otherwise direction controls it
    int hard = 0;
    static const int xy[] = {1, -1, _BOARD_COLS, -_BOARD_COLS};

    unsigned char *man = RAM + _BOARD + playerY * _BOARD_COLS + playerX;
    for (int dir = 0; dir < 4; dir++) {
        unsigned char type = CharToType[GET(*(man + xy[dir]))];
        if (type == TYPE_OUTBOX || Attribute[CharToType[GET(*(man + xy[dir]))]] & ATT_HARD)
            hard++;
    }

    if (hard == 4) {


        if (!(swcha & (JOYSTICK_LEFT << 4)))
            wantedDirection -= 4;
        else if (!(swcha & (JOYSTICK_RIGHT << 4)))
            wantedDirection += 4;

        else
            wantedDirection += rangeRandom(15) - 7;

        tool[0].dir = wantedDirection;


    } else {
        if (tool[0].dir == wantedDirection)
            wantedDirection += rangeRandom(7) - 3;    // getRandom32();
        tool[0].dir = turn_toward(tool[0].dir, wantedDirection, 4);
    }


    for (int i = weaponLength - 1; i > 0; i--)
        tool[i].dir = tool[i - 1].dir;
}

int addTool(int x, int y, int age, unsigned char dir, unsigned short speed) {

    for (int i = 0; i < TOOL_MAX; i++)
        if (!tool[i].age) {

            tool[i].x = x;
            tool[i].y = y;
            tool[i].dir = dir;
            tool[i].speed = speed;

            tool[i].age = age;
            return i;
        }

    return -1;
}

unsigned char *getBoardAddress(int x, int y) {

    int xchar = (x * (0x10000 / CHAR_TRIX_X)) >> 16;
    int ychar = (y * (0x10000 / CHAR_TRIX_Y)) >> 16;

    if (xchar < 0 || xchar >= _BOARD_COLS || ychar < 0 || ychar >= _BOARD_ROWS)
        return 0;

    return (unsigned char *)(RAM + _BOARD + ychar * _BOARD_COLS + xchar);
}

void drawGun() {

    for (int i = 0; i < TOOL_MAX; i++)
        if (tool[i].age && tool[i].age--) {

            int s = tool[i].dir >> 3;

            tool[i].x += (tool[i].speed * sin_cos[s]) >> 8;
            tool[i].y += (tool[i].speed * 2 * sin_cos[(s + 8) & 0x1f]) >> 8;

            int x = tool[i].x >> 8;
            int y = tool[i].y >> 8;

            drawBit(x, y, 7);
            drawBit(x, y + 1, 7);

            enum ChName *p = getBoardAddress(x, y);
            if (p) {
                enum ChName ch = GET(*p);
                enum ObjectType type = CharToType[ch];
                const unsigned int attribute = Attribute[type];

                if (type != TYPE_MELLON_HUSK) {
                    if (!(attribute & ATT_BLANK)) {

                        if ((Attribute[type] & (ATT_EXPLODABLE | ATT_HARD))) {
                            ADDAUDIO(SFX_ROCK);

                            for (int j = 0; j < 12; j++)
                                nDotsAtTrixel(1, x, y, 12, PT_ONE, 120, 7);
                            tool[i].age = 0;

                            if (!(Attribute[type] & ATT_HARD))
                                *p = CH_BLANK | FLAG_THISFRAME;
                        }
                    }
                }
            }
        }


    static int gunDelay = 0;

    if (playerDead || (gunDelay && (--gunDelay > 0)) || (inpt4 & 0x80) || theCave->weapon[level] != WEAPON_GUN)
        return;

    ADDAUDIO(SFX_EXPLODE);
    gunDelay = 20;

    int x = (playerX * CHAR_TRIX_X + CHAR_CENTER_X + ((faceDirection * autoMoveX) >> 2)) << 8;
    int y = ((playerY * CHAR_TRIX_Y + 6) << 8) + (autoMoveY * (256 / 3)) - (2 << 8);

    static const char angle[16] = {

        0,      // 00
        128,    // 01 UP
        0,      // 02 DOWN
        0,      // 03 U+D
        192,    // 04 LEFT
        160,    // 05 U+L
        224,    // 06 D+L
        0,      // 07
        64,     // 08 RIGHT
        96,     // 09 R+U
        32,     // 10 R+D
        0,      // 11
        0,      // 12
        0,      // 13
        0,      // 14
        0,      // 15
    };

    int joy = (swcha ^ 0xFF) >> 4;
    int fireDir = joy ? angle[joy] : faceDirection == 1 ? 64 : 192;

    int idx = addTool(x, y, 60, fireDir, 0xC0);

    // move away from body
    if (idx >= 0) {
        int s = tool[idx].dir >> 3;

        tool[idx].x += (1 * tool[idx].speed * sin_cos[s]) >> 8;
        tool[idx].y += (1 * tool[idx].speed * 2 * sin_cos[(s + 8) & 0x1f]) >> 8;
    }
}


void drawRope() {

    if (playerDead || (RAM[_INPT4] & 0x80) || theCave->weapon[level] != WEAPON_ROPE)
        return;

    if (--weaponLength > ROPE_PARTICLE_COUNT)    // relies on unsigned int arithmetic
        weaponLength = ROPE_PARTICLE_COUNT;

    int baseX = (playerX * 5 + 2 + ((faceDirection * autoMoveX) >> 2)) << 8;
    int baseY = ((playerY * 10 + 6) << 8) + (autoMoveY * (256 / 3));

    int x = 0, y = 0;

    for (unsigned int i = 0; i < weaponLength; i++) {
        x += (sin_cos[(tool[i].dir >> 3) & 0x1F] * PIXEL_ASPECT) >> 8;
        y += (sin_cos[((tool[i].dir >> 3) + 4) & 0x1F] * 256) >> 8;

        drawBit((baseX + x) >> 8, (baseY + y) >> 8, 6);
        drawBit((baseX - x) >> 8, (baseY - y) >> 8, 6);
    }

    modifyCharAtTip(baseX + x, baseY + y);
    modifyCharAtTip(baseX - x, baseY - y);

    static int wantedDirection = 0;

    if (tool[0].dir == wantedDirection)
        wantedDirection += rangeRandom(256);

    else if (wantedDirection > tool[0].dir)
        tool[0].dir += ((wantedDirection - tool[0].dir) >> 2) + 1;

    else
        tool[0].dir -= ((tool[0].dir - wantedDirection) >> 2) + 1;

    for (int i = weaponLength - 1; i > 0; i--)
        tool[i].dir = (tool[i].dir + tool[i - 1].dir * 3) >> 2;
}

struct Particle particle[PARTICLE_COUNT];
static unsigned char particleStack[PARTICLE_COUNT];    // PARTICLE_COUNT (42) < 256 -- holds particle indices only
static int particleStackPointer;

void pushParticle(int prt) {
    particle[prt].age = 0;
    particleStack[particleStackPointer++] = prt;
}

int popParticle() {
    return particleStackPointer ? particleStack[--particleStackPointer] : -1;
}


// Frees every live particle that isn't a spiral, handing its slot straight back to the
// shared pool -- used by the teleport swirl (board.c) to starve out rain/dust/etc. so the
// whole PARTICLE_COUNT budget is available for spirals instead of being split with whatever
// else happens to be spawning that frame (makeRain() etc. aren't gated on teleportLocked).
// PT_CHARACTER is spared too -- it's readable text (score popups, the level-start label),
// not debris; the arrival swirl's first batch fires the same frame a level's label is
// queued (schedule.c), so zapping it here would wipe it before it's ever drawn.
void zapNonSpiralParticles() {

    for (int i = 0; i < PARTICLE_COUNT; i++)
        if (particle[i].age && particle[i].type != PT_SPIRAL && particle[i].type != PT_SPIRAL2 &&
            particle[i].type != PT_CHARACTER)
            pushParticle(i);
}


void initParticles() {

    particleStackPointer = 0;
    for (int i = 0; i < PARTICLE_COUNT; i++)
        pushParticle(i);
}


// void removeFloatingChars() {
//     for (int i = 0; i < PARTICLE_COUNT; i++)
//         if (particle[i].type == PT_CHARACTER)
//             pushParticle(i);
// }


void drawFloatingChars() {

    for (int i = PARTICLE_COUNT - 1; i >= 0; i--) {
        if (particle[i].type == PT_CHARACTER && particle[i].age) {
            blitShape(particle[i].colour, particle[i].trixX_8 >> 8, (particle[i].trixY_8 >> 8) * 3, CHAR_Y,
                      _BUF_GAME_PF0_LEFT);
            if (particle[i].age < 255 && !--particle[i].age)
                pushParticle(i);
        }
    }
}


// Is the board pixel at these absolute trix coordinates actually drawn?
// Used by PT_RAIN's roll-off to test a candidate landing spot, which is
// often in a different cell than the one it just hit. Mirrors
// grabCharacters() (drawScreen.c) for glyph selection.
static bool rainPixelSolid(int trixX, int trixY) {

    int col = (trixX * ((0x10000 + CHAR_TRIX_X - 1) / CHAR_TRIX_X)) >> 16;
    int row = (trixY * ((0x10000 + CHAR_TRIX_Y - 1) / CHAR_TRIX_Y)) >> 16;

    if (row < 0 || row >= _BOARD_ROWS || col < 0 || col >= _BOARD_COLS)
        return true;    // off-board -- treat as blocked

    unsigned char *testCell = RAM + _BOARD + row * _BOARD_COLS + col;
    enum ObjectType testType = CharToType[GET(*testCell)];

    if (Attribute[testType] & ATT_BLANK)
        return false;

    const unsigned char *fp;

    if (Attribute[testType] & ATT_GEODOGE) {
        unsigned char udlr =
            ((Attribute[CharToType[GET(*(testCell - _BOARD_COLS))]] & ATT_GEODOGE) >> POS_GEODOGE) |
            ((Attribute[CharToType[GET(*(testCell + 1))]] & ATT_GEODOGE) >> (POS_GEODOGE - 1)) |
            ((Attribute[CharToType[GET(*(testCell + _BOARD_COLS))]] & ATT_GEODOGE) >> (POS_GEODOGE - 2)) |
            ((Attribute[CharToType[GET(*(testCell - 1))]] & ATT_GEODOGE) >> (POS_GEODOGE - 3));
        fp = conglomerate[udlr];
    } else if (Animate[testType]) {
        fp = charSet[*Animate[testType]];
    } else {
        fp = charSet[GET(*testCell)];
    }

    int subC = trixX - col * CHAR_TRIX_X;
    int subR = trixY - row * CHAR_TRIX_Y;

    const unsigned char *rowData = fp + subR * 3;
    unsigned char lit = rowData[0] | rowData[1] | rowData[2];
    return (lit >> (CHAR_TRIX_X - 1 - subC)) & 1;
}

void drawParticles() {

    TIMED(DRAWPARTICLE, 0x4B8);    // 14/7/2026

    for (int i = 0; i < PARTICLE_COUNT && TIME_OK(DRAWPARTICLE); i++)
        if (particle[i].age && particle[i].type != PT_CHARACTER) {

            int distance3x = particle[i].distance * 3;    // roller==0 gating above only advances
                                                          // distance 1 real frame in 3, so scale
                                                          // it back up here to keep the same
                                                          // effective speed
            int xOffset = (sin_cos[particle[i].dir >> 3] * distance3x) >> 8;
            int yOffset = (sin_cos[((particle[i].dir + 64) & 0xFF) >> 3] * distance3x * 3) >> 8;

            int y = (particle[i].trixY_8 + yOffset) >> 8;
            int x = (particle[i].trixX_8 + xOffset) >> 8;

            switch (particle[i].type) {

            case PT_SPIRAL2:
            case PT_SPIRAL: {
                particle[i].dir += PARTICLE_SPIRAL_ANGULAR_SPEED;

                if (!rangeRandom(250))
                    nDotsAtTrixel(4, x, y, 30, PT_SPIRAL, 0x20, particle[i].colour);
                break;
            }

            case PT_BUBBLE: {
                if (y < (liquidTrixel_8) >> 8) {
                    pushParticle(i);
                    continue;
                }

                particle[i].trixY_8 -= particle[i].speed;
                x += rangeRandom(3) - 1;    // wobble
                break;
            }

            case PT_RAIN: {

                // dir is repurposed as an 8-bit fall-velocity accumulator
                // (distance stays pinned at 0, so the polar offset above is
                // inert for this type).
                if (particle[i].dir < 240)
                    particle[i].dir += 8;
                particle[i].trixY_8 += particle[i].dir;

                if (showWater && (particle[i].trixY_8 >> 8) >= (liquidTrixel_8 >> 8)) {
                    if (!particle[i].silent) {
                        ADDAUDIO(SFX_DRIP2);
                        nDotsAtTrixel(3, particle[i].trixX_8 >> 8, liquidTrixel_8 >> 8, 12, PT_TWO, 30, 7);
                    }
                    pushParticle(i);
                    continue;
                }

                int cellCol = ((particle[i].trixX_8 >> 8) * ((0x10000 + CHAR_TRIX_X - 1) / CHAR_TRIX_X)) >> 16;
                int cellRow = ((particle[i].trixY_8 >> 8) * ((0x10000 + CHAR_TRIX_Y - 1) / CHAR_TRIX_Y)) >> 16;

                if (cellRow < 0 || cellRow >= _BOARD_ROWS || cellCol < 0 || cellCol >= _BOARD_COLS) {
                    if (!particle[i].silent) {
                        ADDAUDIO(SFX_DRIP2);
                        nDotsAtTrixel(3, particle[i].trixX_8 >> 8, particle[i].trixY_8 >> 8, 12, PT_TWO, 30, 7);
                    }
                    pushParticle(i);
                    continue;
                }

                unsigned char *cell = RAM + _BOARD + cellRow * _BOARD_COLS + cellCol;
                enum ObjectType hitType = CharToType[GET(*cell)];

                int subCol = (particle[i].trixX_8 >> 8) - cellCol * CHAR_TRIX_X;

                // Player isn't in the board array -- treat its cell as a
                // person-shaped obstacle: outer columns pass through, inner
                // three splash at the top of the cell.
                if (cellCol == playerX && cellRow == playerY && subCol > 0 && subCol < CHAR_TRIX_X - 1) {
                    if (!particle[i].silent) {
                        ADDAUDIO(SFX_DRIP2);
                        nDotsAtTrixel(3, particle[i].trixX_8 >> 8, cellRow * CHAR_TRIX_Y, 12, PT_TWO, 30, 7);
                    }
                    pushParticle(i);
                    continue;
                }

                // Per-pixel glyph test, mirroring grabCharacters()
                // (drawScreen.c): Animate[]/ATT_GEODOGE glyph selection,
                // ATT_BLANK short-circuited as always passable.
                bool pixelSolid = false;
                unsigned char litMask = 0;

                if (!(Attribute[hitType] & ATT_BLANK)) {

                    const unsigned char *fp;

                    if (Attribute[hitType] & ATT_GEODOGE) {
                        unsigned char udlr =
                            ((Attribute[CharToType[GET(*(cell - _BOARD_COLS))]] & ATT_GEODOGE) >> POS_GEODOGE) |
                            ((Attribute[CharToType[GET(*(cell + 1))]] & ATT_GEODOGE) >> (POS_GEODOGE - 1)) |
                            ((Attribute[CharToType[GET(*(cell + _BOARD_COLS))]] & ATT_GEODOGE) >> (POS_GEODOGE - 2)) |
                            ((Attribute[CharToType[GET(*(cell - 1))]] & ATT_GEODOGE) >> (POS_GEODOGE - 3));
                        fp = conglomerate[udlr];
                    } else if (Animate[hitType]) {
                        fp = charSet[*Animate[hitType]];
                    } else {
                        fp = charSet[GET(*cell)];
                    }

                    int subRow = (particle[i].trixY_8 >> 8) - cellRow * CHAR_TRIX_Y;

                    // tools/cset.py pack_bits(): one bit per column across 3
                    // planes, column 0 in the highest bit.
                    const unsigned char *rowData = fp + subRow * 3;
                    litMask = rowData[0] | rowData[1] | rowData[2];
                    pixelSolid = (litMask >> (CHAR_TRIX_X - 1 - subCol)) & 1;
                }

                if (pixelSolid && (hitType == TYPE_ROCK || hitType == TYPE_GEODOGE)) {

                    // One-shot roll: which half of the glyph picks a side
                    // (dead centre splashes immediately); test the board
                    // (not this glyph) one column over and one pixel down.
                    int half = CHAR_TRIX_X >> 1;

                    if (subCol == half) {
                        // silent: no clear side to roll toward -- keep falling straight through
                        // rather than splashing and dying (see the roll-blocked case below for
                        // the same reasoning).
                        if (particle[i].silent)
                            break;
                        ADDAUDIO(SFX_DRIP2);
                        nDotsAtTrixel(3, particle[i].trixX_8 >> 8, particle[i].trixY_8 >> 8, 12, PT_TWO, 30, 7);
                        pushParticle(i);
                        continue;
                    }

                    int rollDir = subCol < half ? -1 : 1;
                    int newTrixX = (particle[i].trixX_8 >> 8) + rollDir;
                    int newTrixY = (particle[i].trixY_8 >> 8) + 1;

                    if (!rainPixelSolid(newTrixX, newTrixY)) {
                        // Horizontal only -- undo this frame's gravity fall
                        // (already applied above, before this check ran) so
                        // a roll is a pure sideways step.
                        particle[i].trixX_8 = newTrixX << 8;
                        particle[i].trixY_8 -= particle[i].dir;
                        particle[i].dir >>= 2;    // sheds most of its speed onto the rock
                    } else if (particle[i].silent) {
                        // silent: can't find a blank pixel to roll onto -- rather than splashing
                        // and dying like real rain would, just keep falling straight down
                        // through the obstruction; it re-tests from its new (already-fallen)
                        // position next frame.
                    } else {
                        ADDAUDIO(SFX_DRIP2);
                        nDotsAtTrixel(3, particle[i].trixX_8 >> 8, particle[i].trixY_8 >> 8, 12, PT_TWO, 30, 7);
                        pushParticle(i);
                        continue;
                    }

                } else if (pixelSolid && !particle[i].silent) {

                    ADDAUDIO(SFX_DRIP2);
                    nDotsAtTrixel(3, particle[i].trixX_8 >> 8, particle[i].trixY_8 >> 8, 12, PT_TWO, 30, 7);
                    pushParticle(i);
                    continue;
                }

                break;
            }

            default:
                break;
            }

            if (roller == 0)
                particle[i].distance += particle[i].speed;

            if (!--particle[i].age || !drawBit(x, y, particle[i].colour)) {

                // Age/off-screen safety net -- splash here too so a rain
                // drop never silently vanishes without hitting anything.
                if (particle[i].type == PT_RAIN && !particle[i].silent) {
                    ADDAUDIO(SFX_DRIP2);
                    nDotsAtTrixel(3, particle[i].trixX_8 >> 8, particle[i].trixY_8 >> 8, 12, PT_TWO, 30, 7);
                }

                pushParticle(i);
            }
        }
}

// weatherIntensity is the frequency divisor makeRain() rolls against: 1 in
// weatherIntensity calls attempts a spawn, lower = heavier rain.
// theCave->weather seeds it -- except 255, which means "rainstorms": dry
// gaps punctuated by a downpour that ramps up, holds at torrential, then
// tails off, cycling via weatherPhase.
#define WEATHER_WAIT_MAX_FRAMES (30 * 60)    // dry gap between storms: 0-30s, picked fresh each time
#define WEATHER_RISE_FRAMES (2 * 60)         // build from WEATHER_LIGHT to torrential
#define WEATHER_PEAK_FRAMES (12 * 60)        // hold at torrential
#define WEATHER_FALL_FRAMES (5 * 60)         // tail back off to nothing
#define WEATHER_LIGHT 12                     // intensity divisor at the start/end of a storm
#define WEATHER_TORRENTIAL 1                 // intensity divisor at the peak -- heaviest possible

enum WeatherPhase { WEATHER_WAIT, WEATHER_RISE, WEATHER_PEAK, WEATHER_FALL };

int weatherIntensity;
static enum WeatherPhase weatherPhase;
static int weatherPhaseTimer;    // counts down frames remaining in the current phase

void initWeather() {
    weatherPhase = WEATHER_WAIT;
    weatherPhaseTimer = rangeRandom(WEATHER_WAIT_MAX_FRAMES);
    weatherIntensity = theCave->weather;    // meaningful only for the non-255 fixed-rate case
}

int isStormActive() {
    return theCave->weather == 255 && weatherPhase == WEATHER_PEAK;
}

void makeRain() {

    if (!theCave->weather)
        return;

    if (theCave->weather == 255) {

        switch (weatherPhase) {

        case WEATHER_WAIT:
            if (--weatherPhaseTimer <= 0) {
                weatherPhase = WEATHER_RISE;
                weatherPhaseTimer = WEATHER_RISE_FRAMES;
            }
            return;    // dry -- no spawn attempts at all between storms

        case WEATHER_RISE:

            weatherIntensity =
                WEATHER_TORRENTIAL +
                (((WEATHER_LIGHT - WEATHER_TORRENTIAL) * weatherPhaseTimer) * (0x10000 / WEATHER_RISE_FRAMES) >> 16);
            if (--weatherPhaseTimer <= 0) {
                weatherPhase = WEATHER_PEAK;
                weatherPhaseTimer = WEATHER_PEAK_FRAMES;
            }
            break;

        case WEATHER_PEAK:
            weatherIntensity = WEATHER_TORRENTIAL;
            if (--weatherPhaseTimer <= 0) {
                weatherPhase = WEATHER_FALL;
                weatherPhaseTimer = WEATHER_FALL_FRAMES;
            }
            break;

        case WEATHER_FALL:
            weatherIntensity = WEATHER_TORRENTIAL +
                               (((WEATHER_LIGHT - WEATHER_TORRENTIAL) * (WEATHER_FALL_FRAMES - weatherPhaseTimer)) *
                                    (0x10000 / WEATHER_FALL_FRAMES) >>
                                16);
            if (--weatherPhaseTimer <= 0) {
                weatherPhase = WEATHER_WAIT;
                weatherPhaseTimer = rangeRandom(WEATHER_WAIT_MAX_FRAMES);
            }
            break;
        }
    }

    if (rangeRandom(weatherIntensity))
        return;

    int col = (((scrollX >> 16) * ((0x10000 + CHAR_TRIX_X - 1) / CHAR_TRIX_X)) >> 16) +
              rangeRandom(SCREEN_TRIX_X / CHAR_TRIX_X);
    int row = (((scrollY >> 16) * ((0x10000 + CHAR_TRIX_Y - 1) / CHAR_TRIX_Y)) >> 16) +
              rangeRandom(SCREEN_TRIX_Y / CHAR_TRIX_Y);

    if (col < 0 || col >= _BOARD_COLS || row < 1 || row >= _BOARD_ROWS)
        return;

    unsigned char *cell = RAM + _BOARD + row * _BOARD_COLS + col;

    // Only drip from a blank cell directly under an ATT_DRIP-flagged one.
    if ((Attribute[CharToType[GET(*cell)]] & ATT_BLANK) &&
        (Attribute[CharToType[GET(*(cell - _BOARD_COLS))]] & ATT_DRIP)) {

        int idx = sphereDot(col * CHAR_TRIX_X + rangeRandom(CHAR_TRIX_X), row * CHAR_TRIX_Y, PT_RAIN, 200, 3);
        if (idx >= 0) {
            particle[idx].dir = 0;
            particle[idx].distance = 0;
            particle[idx].speed = 0;
        }
    }
}


int sphereDot(int trixX, int trixY, int type, unsigned char age, unsigned char colour) {


    int whichDrop = -1;

    int col = trixX - ((scrollX) >> 16);
    if (col >= 0 && col < 40 /*pixels*/) {

        int line = trixY - (scrollY >> 16);
        if (line >= 0 && line < (_SCANLINES / 3 - 1)) {


            // TODO: could add a priority here, and if we can't pop then a one-pass kill lower priority


            whichDrop = popParticle();
            if (whichDrop >= 0) {

                particle[whichDrop].type = type;
                particle[whichDrop].trixX_8 = trixX << 8;

                particle[whichDrop].trixY_8 = trixY << 8;
                particle[whichDrop].speed = 0;
                particle[whichDrop].age = age;
                particle[whichDrop].colour = colour;

                particle[whichDrop].dir = getRandom32();    // 16.16 angle
                particle[whichDrop].distance = 0;
                particle[whichDrop].silent = false;
            }
        }
    }

    return whichDrop;
}


int nDots(int count, int trixX, int trixY, int type, unsigned char age, int offsetX, int offsetY, int speed,
          unsigned char colour) {

    // Note: if type == PT_CHARACTER, then colour = CH_* name

    int lastIdx = -1;

    int pixelX = trixX * CHAR_TRIX_X + offsetX;
    int pixelY = trixY * CHAR_TRIX_Y + offsetY;

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(pixelX, pixelY, type, age, colour);
        if (idx >= 0) {
            particle[idx].speed = rangeRandom(speed);
            if (type == PT_SPIRAL2)
                // dir already set by sphereDot() -- every particle gets one, not just SPIRAL2.
                particle[idx].distance = rangeRandom(200) + 50;
            lastIdx = idx;
        } else
            break;
    }

    return lastIdx;
}


void floatingCharacter(int trixX, int trixY, int age, unsigned char ch) {

    int slot = popParticle();

    if (slot < 0)
        for (int i = 0; i < PARTICLE_COUNT; i++)
            if (particle[i].age && particle[i].type != PT_CHARACTER) {
                slot = i;
                break;
            }

    if (slot >= 0) {
        particle[slot].type = PT_CHARACTER;
        particle[slot].trixX_8 = trixX << 8;
        particle[slot].trixY_8 = trixY << 8;
        particle[slot].age = age;
        particle[slot].colour = ch;    // letter
    }
}


void nDotsAtTrixel(int count, int trixX, int trixY, unsigned char age,    //
                   enum ParticleType type, int speed, unsigned char colour) {

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(trixX, trixY, type, age, colour);
        if (idx >= 0)
            particle[idx].speed = speed;
    }
}

// EOF
