#include <stdbool.h>

#include "board.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "T1TC.h"
#include "attribute.h"
#include "colour.h"
#include "decodeCaves.h"
#include "draw.h"
#include "drawScreen.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "random.h"
#include "score.h"
#include "scroll.h"
#include "sound.h"

// TEMPORARY -- see makeRain() below. Prints the raw CH_* index of whatever
// the DRIP check read directly above each rain spawn. Confirmed spawning
// itself is fine (2026-07-29), turned off now that the actual bug was
// found to be the rock roll-off relocating drops, not the spawn check --
// flip back to 1 if spawn placement is ever in doubt again.
#define RAIN_SPAWN_DEBUG 0

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
       0,   50,   98,  142,  181,  213,  237,  251,     //   0° (D) to <  90° (R)
     256,  251,  237,  213,  181,  142,   98,   50,      //  90° (R) to < 180° (U)
       0,  -50,  -98, -142, -181, -213, -237, -251,    // 180° (U) to < 270° (L)
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


void drawParticles() {

    TIMED(DRAWPARTICLE, 0x4B8);    // 14/7/2026

    for (int i = 0; i < PARTICLE_COUNT && TIME_OK(DRAWPARTICLE); i++)
        if (particle[i].age && particle[i].type != PT_CHARACTER) {

            int xOffset = (sin_cos[particle[i].dir >> 3] * particle[i].distance) >> 8;
            int yOffset = (sin_cos[((particle[i].dir + 64) & 0xFF) >> 3] * particle[i].distance * 3) >> 8;

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
                if (y < lavaSurfaceTrixel) {
                    particle[i].age = 0;
                    continue;
                }

                particle[i].trixY_8 -= particle[i].speed;
                x += rangeRandom(3) - 1;    // wobble
                break;
            }

            case PT_RAIN: {

                // Rain doesn't use the dir/distance polar-offset dance above --
                // distance is pinned at 0 (see makeRain()) so xOffset/yOffset are
                // always 0 for this type, and dir is repurposed here as an 8-bit
                // fall-velocity accumulator instead of an angle. Keeps a straight
                // vertical drop without fighting the generic "distance +=
                // speed" advance every other particle type relies on below.
                if (particle[i].dir < 240)
                    particle[i].dir += 8;

                // dir maxes out at 240 here (fixed-point trix*256 units), so
                // adding it straight to trixY_8 tops out just under 1 trix/frame
                // -- crosses a 10-trix character cell in ~11 frames at terminal.
                // The >>2 this replaced divided that by another 4, so drops were
                // taking the better part of a second per cell -- looked more like
                // drizzle sliding down glass than falling rain.
                particle[i].trixY_8 += particle[i].dir;

                // No hardware divide on this target (ARMv4T/Thumb) and nothing
                // links a soft-divide routine here -- CHAR_TRIX_X/Y (5/10) aren't
                // powers of two, so a plain '/' silently hard-faults instead of
                // erroring at build time. Same reciprocal-multiply-shift trick
                // getBoardAddress() above uses for this conversion, but with the
                // reciprocal rounded UP (ceiling), not truncated down: rain spawns
                // sitting exactly on a cell boundary (row*CHAR_TRIX_Y, see
                // makeRain()), and a truncated reciprocal underestimates exactly
                // at those multiples -- e.g. 0x10000/10 floors to 6553, and
                // 50*6553>>16 comes out to 4, not 5. That misread the drop's own
                // spawn cell as the drip source one row up (solid), so it
                // splashed against itself the instant it was evaluated. Rounding
                // the reciprocal up instead (6554) keeps exact multiples correct
                // while still landing in the right cell everywhere in between.
                int cellCol = ((particle[i].trixX_8 >> 8) * ((0x10000 + CHAR_TRIX_X - 1) / CHAR_TRIX_X)) >> 16;
                int cellRow = ((particle[i].trixY_8 >> 8) * ((0x10000 + CHAR_TRIX_Y - 1) / CHAR_TRIX_Y)) >> 16;

                if (cellRow < 0 || cellRow >= _BOARD_ROWS || cellCol < 0 || cellCol >= _BOARD_COLS) {
                    pushParticle(i);
                    continue;
                }

                unsigned char *cell = RAM + _BOARD + cellRow * _BOARD_COLS + cellCol;
                enum ObjectType hitType = CharToType[GET(*cell)];

                if (!(Attribute[hitType] & ATT_BLANK)) {

                    // Used to roll rock/geodoge hits off sideways, same feel doRoll()
                    // gives real boulders. Dropped it -- rolling relocates the drop a
                    // full character sideways with no visible drip source above the
                    // new column, and it then keeps falling from there. That's what
                    // every one of the "spawns in the wrong place" reports turned out
                    // to be: not a spawn bug, this teleport-and-continue reading as
                    // one. A drop hitting a rock just splashes now, same as hitting
                    // anything else solid -- it always terminates exactly where it
                    // was visibly falling.
                    ADDAUDIO(SFX_DRIP2);
                    nDotsAtTrixel(3, cellCol * CHAR_TRIX_X + CHAR_CENTER_X, cellRow * CHAR_TRIX_Y, 12, PT_TWO, 30, 7);
                    pushParticle(i);
                    continue;
                }

                break;
            }

            default:
                break;
            }

            particle[i].distance += particle[i].speed;

            if (!--particle[i].age || !drawBit(x, y, particle[i].colour))
                pushParticle(i);
        }
}

// Ported from Boulder-Dash-DEMO2025's makeRain() (main.c/drawscreen.c there) --
// that version drove rain as its own dedicated pair of drops, hand-plotted
// pixel-by-pixel against the raw character bitmap. This project already has a
// general particle pool for exactly this kind of thing, so rain is just
// another particle type (PT_RAIN, see the switch in drawParticles() above)
// riding the same pool as dust/spirals/bubbles, gated on the per-cave
// theCave->weather byte (decodeCaves.h) that was sitting there unused.

// weatherIntensity is the frequency divisor makeRain() actually rolls
// against: 1 in weatherIntensity calls attempts a spawn, so lower = heavier
// rain, 1 is as heavy as it gets. theCave->weather seeds it (see
// initWeather()) and is normally just copied straight across -- except 255,
// which means "storm builds over the level": start slow and let
// weatherIntensity creep down toward WEATHER_RAMP_MIN on its own as play
// continues, rather than holding at one fixed rate for the whole cave.
#define WEATHER_RAMP_START 50       // drizzle: roughly 1 spawn attempt/sec at the start
#define WEATHER_RAMP_MIN 1          // heaviest -- a spawn attempt every call
#define WEATHER_RAMP_INTERVAL 300   // frames between each step down (~5s at 60fps)

int weatherIntensity;
static int weatherRampTimer;

void initWeather() {
    weatherRampTimer = 0;
    weatherIntensity = theCave->weather == 255 ? WEATHER_RAMP_START : theCave->weather;
}

void makeRain() {

    if (!theCave->weather)
        return;

    if (theCave->weather == 255 && weatherIntensity > WEATHER_RAMP_MIN && ++weatherRampTimer >= WEATHER_RAMP_INTERVAL) {
        weatherRampTimer = 0;
        weatherIntensity--;
    }

    if (rangeRandom(weatherIntensity))
        return;

    // SCREEN_TRIX_X/CHAR_TRIX_X and SCREEN_TRIX_Y/CHAR_TRIX_Y are both
    // compile-time constants (folded away, no runtime div) -- but scrollX/Y
    // are runtime values, so those divisions need the same reciprocal-
    // multiply-shift trick as the PT_RAIN case above (ceiling-rounded
    // reciprocal, same reasoning as there) rather than a plain '/'.
    int col = (((scrollX >> 16) * ((0x10000 + CHAR_TRIX_X - 1) / CHAR_TRIX_X)) >> 16) +
              rangeRandom(SCREEN_TRIX_X / CHAR_TRIX_X);
    int row = (((scrollY >> 16) * ((0x10000 + CHAR_TRIX_Y - 1) / CHAR_TRIX_Y)) >> 16) +
              rangeRandom(SCREEN_TRIX_Y / CHAR_TRIX_Y);

    if (col < 0 || col >= _BOARD_COLS || row < 1 || row >= _BOARD_ROWS)
        return;

    unsigned char *cell = RAM + _BOARD + row * _BOARD_COLS + col;

    // only drip from a blank cell directly under something ATT_DRIP-flagged --
    // same rule DEMO2025 used (its ATT_DRIP), now on WenHop's DIRT/BRICKWALL/
    // STEELWALL/PEBBLE1/CONCRETE (see attribute.c)
    if ((Attribute[CharToType[GET(*cell)]] & ATT_BLANK) &&
        (Attribute[CharToType[GET(*(cell - _BOARD_COLS))]] & ATT_DRIP)) {

#if RAIN_SPAWN_DEBUG
        // TEMPORARY -- prints the raw CH_* index (attribute.h) of the cell this
        // spawn believed was DRIP-flagged, right above the drop, so we can see
        // exactly what the check actually read instead of guessing from static
        // code review. Delete once the mislocated-spawn bug is confirmed/found.
        {
            char str[4];
            drawDecimalToString(str, '0', GET(*(cell - _BOARD_COLS)));
            displayFloatingString(col * CHAR_TRIX_X - (scrollX >> 16), (row - 1) * CHAR_TRIX_Y - (scrollY >> 16), 90,
                                  str);
        }
#endif

        int idx = sphereDot(col * CHAR_TRIX_X + CHAR_CENTER_X, row * CHAR_TRIX_Y, PT_RAIN, 200, 3);
        if (idx >= 0) {
            // sphereDot() defaults these for the radiating-burst types --
            // rain doesn't use them (see the PT_RAIN case), so pin them inert
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
                particle[whichDrop].distance = 96;          // 16.16 speed
            }

            // else {
            //     FLASH(0x42, 2);
            // }
        }
    }

    return whichDrop;
}


int nDots(int count, int trixX, int trixY, int type, unsigned char age, int offsetX, int offsetY, int speed,
          unsigned char colour) {

    // Note: if type == PT_CHARACTER, then colour = CH_* name

    int lastIdx = -1;
    // if (gravity < 0)
    //     offsetY = CHAR_TRIX_Y - offsetY;

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(trixX * CHAR_TRIX_X + offsetX, trixY * CHAR_TRIX_Y + offsetY, type, age, colour);
        if (idx >= 0) {
            particle[idx].speed = rangeRandom(speed);
            if (type == PT_SPIRAL2) {
                particle[idx].distance = rangeRandom(200) + 50;
                particle[idx].dir = getRandom32();
            }
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
