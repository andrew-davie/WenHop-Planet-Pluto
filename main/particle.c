#include <stdbool.h>

#include "board.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "attribute.h"
#include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "drawScreen.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "player.h"
#include "random.h"
#include "scroll.h"
#include "sound.h"

unsigned int weaponLength = 0;

void nDotsAtTrixel2(int count, int dripX, int dripY, unsigned char age, enum ParticleType type, int speed,
                    unsigned char colour, unsigned char dmask);

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


const int sin_cos[32] = {
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

    unsigned char colour = 0;

    int xchar = (x * (256 / CHAR_TRIX_X)) >> 16;
    int ychar = (y * (256 / CHAR_TRIX_Y)) >> 16;

    if (xchar < 0 || xchar >= _BOARD_COLS || ychar < 0 || ychar >= _BOARD_ROWS)
        return;

    unsigned char *b = RAM + _BOARD + ychar * _BOARD_COLS + xchar;
    enum ObjectType type = CharToType[GET(*b)];


    if (Attribute[type] & ATT_EXPLODABLE) {

        if (type == TYPE_DOGE) {
            doges--;
            *b = CH_BLANK | FLAG_THISFRAME;
            colour = rangeRandom(7) + 1;
        }

        else if (type == TYPE_ROCK) {
            *b = CH_GEODOGE | FLAG_THISFRAME;
            colour = 1;
        }

        else if (type == TYPE_ROCK_BONUS) {
            *b = FLAG(CH_STAR);
            colour = 7;

        }


        else if (type == TYPE_GEODOGE) {
            *b = CH_DOGE_00 | FLAG_THISFRAME;
            colour = 3;
            surroundingConglomerate(xchar, ychar);
        }

        else if (type == TYPE_DIRT) {
            *b = CH_DUST_0 | FLAG_THISFRAME;
            colour = 2;
        }

        else {

            if (xchar > 0 && xchar < _BOARD_COLS && ychar > 0 && ychar < _BOARD_ROWS) {

                if (type == TYPE_BRICKWALL) {

                    *b = CH_DUST_0 | FLAG_THISFRAME;
                    colour = 7;
                }

                else if (type == TYPE_STEELWALL) {
                    *b = CH_DUST_0 | FLAG_THISFRAME;
                    colour = 7;
                }
            }
        }

        if (*b & FLAG_THISFRAME)
            nDotsAtTrixel(5, (x >> 8) + CHAR_CENTER_Y, (y >> 8) + CHAR_CENTER_Y, 30, PT_SPIRAL, 50, colour);
    }
}

// bool ropeEnabled = false;
const int PIXEL_ASPECT = 110;

unsigned char turn_toward(unsigned char current, unsigned char target, unsigned char speed) {
    if (current == target)
        return target;    // fix: early exit
    signed char diff = (signed char)(target - current);
    // speed must be <= 127; caller's responsibility or clamp here:
    // speed = (speed > 127) ? 127 : speed;
    signed char s = (signed char)speed;    // safe only if speed <= 127
    if (diff > 0)
        return (diff <= s) ? target : (unsigned char)(current + s);
    else
        return (diff >= -s) ? target : (unsigned char)(current - s);
}


void drawMace() {

    // if (T1TC > availableIdleTime - 3000)
    //     return;

    if (playerDead || !(weapon & WEAPON_MACE))
        return;

    if ((inpt4 & 0x80) && !weaponLength) {
        //        weaponLength = 0;
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
    static const int xy[] = {1, -1, _1ROW, -_1ROW};

    unsigned char *man = RAM + _BOARD + playerY * _1ROW + playerX;
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
        FLASH(0x28, 2);
        if (tool[0].dir == wantedDirection)
            wantedDirection = getRandom32();
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

    // if (T1TC > availableIdleTime - 3000)
    //     return;


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

                            for (int i = 0; i < 12; i++)
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

    static char angle[16] = {

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
        wantedDirection = rangeRandom(256);

    else if (wantedDirection > tool[0].dir)
        tool[0].dir += ((wantedDirection - tool[0].dir) >> 2) + 1;

    else
        tool[0].dir -= ((tool[0].dir - wantedDirection) >> 2) + 1;

    for (int i = weaponLength - 1; i > 0; i--)
        tool[i].dir = (tool[i].dir + tool[i - 1].dir * 3) >> 2;
}

struct Particle particle[PARTICLE_COUNT];
int particleStack[PARTICLE_COUNT];
int particleStackPointer;

int pushParticle(int prt) {
    particleStack[particleStackPointer++] = prt;
}

int popParticle() {
    return particleStackPointer ? particleStack[--particleStackPointer] : -1;
}


void initParticles() {

    particleStackPointer = 0;
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        particle[i].age = 0;
        pushParticle(i);
    }
}


void drawParticles() {

    for (int i = 0; i < PARTICLE_COUNT; i++) {

        // if (T1TC > availableIdleTime - 500)
        //     return;

        if (particle[i].age) {

            int xOffset = (sin_cos[particle[i].dir >> 3] * particle[i].distance) >> 8;
            int yOffset = (sin_cos[((particle[i].dir + 64) & 0xFF) >> 3] * particle[i].distance * 3) >> 8;

            int y = (particle[i].y + yOffset) >> 8;
            int x = (particle[i].x + xOffset) >> 8;

            switch (particle[i].type) {

            case PT_TWO: {

                break;
            }

            case PT_SPIRAL2:
            case PT_SPIRAL: {

                particle[i].dir += PARTICLE_SPIRAL_ANGULAR_SPEED;

                if (!rangeRandom(250))
                    nDotsAtTrixel(4, x, y, 30, PT_SPIRAL, 0x20, particle[i].colour);
                break;
            }

            case PT_BUBBLE: {

                // FLASH(0xD2, 4);

                if (y < lavaSurfaceTrixel) {
                    particle[i].age = 0;
                    continue;
                }

                particle[i].y -= particle[i].speed;
                x += rangeRandom(3) - 1;    // wobble
                break;
            }

            default:
                break;
            }

            if (!drawBit(x, y, particle[i].colour)) {
                particle[i].age = 0;
                pushParticle(i);
            }

            else {

                if (!--particle[i].age)
                    pushParticle(i);
                else
                    particle[i].distance += particle[i].speed;
            }
        }
    }
}

int sphereDot(int dotX, int dotY, int type, unsigned char age, unsigned char colour) {

    int whichDrop = -1;

    int col = dotX - ((scrollX) >> 16);
    if (col >= 0 && col < 40 /*pixels*/) {

        int line = dotY - (scrollY >> 16);
        if (line >= 0 && line < (_SCANLINES / 3 - 1)) {


            // TODO: could add a priority here, and if we can't pop then a one-pass kill lower priority


            whichDrop = popParticle();
            if (whichDrop >= 0) {

                // int oldest = 0;
                // while (++whichDrop < PARTICLE_COUNT && particle[whichDrop].age)
                //     if (particle[whichDrop].age < particle[oldest].age)
                //         oldest = whichDrop;

                // if (whichDrop == PARTICLE_COUNT)
                //     whichDrop = oldest;

                particle[whichDrop].type = type;
                particle[whichDrop].x = dotX << 8;

                particle[whichDrop].y = dotY << 8;
                particle[whichDrop].speed = 0;
                particle[whichDrop].age = age;
                particle[whichDrop].colour = colour;

                particle[whichDrop].dir = getRandom32();    // 16.16 angle
                particle[whichDrop].distance = 96;          // 16.16 speed
            }
        }
    }

    return whichDrop;
}


void nDots(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed,
           unsigned char colour) {

    if (gravity < 0)
        offsetY = CHAR_TRIX_Y - offsetY;

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX * CHAR_TRIX_X + offsetX, dripY * CHAR_TRIX_Y + offsetY, type, age, colour);
        if (idx >= 0) {
            particle[idx].speed = rangeRandom(speed);
            if (type == PT_SPIRAL2) {
                particle[idx].distance = rangeRandom(200) + 50;
                particle[idx].dir = getRandom32();
            }
        }
    }
}

void nDotsBackwards(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY,
                    int /*speed*/) {

    if (gravity < 0)
        offsetY = CHAR_TRIX_Y - offsetY;

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX * CHAR_TRIX_X + offsetX, dripY * CHAR_TRIX_Y + offsetY, type, age, 3);
        if (idx >= 0) {
            // TODO  vector
            particle[idx].x += particle[idx].age * particle[idx].speed;
            particle[idx].y += particle[idx].age * particle[idx].speed;

            // particle.speedX[idx] = -particle.speedX[idx];
            // particle.speedY[idx] = -particle.speedY[idx];
        }
    }
}

void nDotsAtTrixel(int count, int dripX, int dripY, unsigned char age, enum ParticleType type, int speed,
                   unsigned char colour) {

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX, dripY, type, age, colour);
        if (idx >= 0)
            particle[idx].speed = speed;
    }
}

#define SPREAD 96

void nDotsAtTrixel2(int count, int dripX, int dripY, unsigned char age, enum ParticleType type, int speed,
                    unsigned char colour, unsigned char dmask) {

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX, dripY, type, age, colour);
        if (idx >= 0) {
            particle[idx].speed = rangeRandom(speed);


            int doff = rangeRandom(SPREAD) - (SPREAD >> 1);
            particle[idx].dir = (unsigned char)(128 + dmask + doff);
        }
    }
}


//-------------rain
#if 0

void makeRain() {

    for (int drop = 0; drop < RAINHAILSHINE; drop++)
        if (rainX[drop] == UNUSED) {

            // carefully calculated to not overflow board bounds!

            int dripX = (scrollX[displayMode] >> (SHIFT_SCROLLX + 2)) + rangeRandom(10);
            int dripY = (((scrollY[displayMode] >> SHIFT_SCROLLY) * (0x300 / CHAR_HEIGHT)) >> 8) + rangeRandom(8);

            unsigned char *dripPos = ADDRESS_OF(dripY) + dripX;
            if ((Attribute[CharToType[GET(*dripPos)]] & ATT_BLANK) &&
                (Attribute[CharToType[GET(*(dripPos - _1ROW))]] & ATT_DRIP)) {

                rainX[drop] = (dripX << 2) + rangeRandom(4);
                rainY[drop] = (dripY * (CHAR_HEIGHT / 3)) << 16;    // embed in upper char
                rainSpeed[drop] = RAIN_FORMING_DRIP;
            }
        }
}


void rain() {

    for (int i = 0; i < RAINHAILSHINE; i++) {
        if (rainX[i] != 255) {

            rainSpeed[i] += RAIN_ACCEL;

            if (rainSpeed[i] > 0)
                rainY[i] += rainSpeed[i];

            int t = (rainY[i] >> 16);
            int row = 0;
            while (t >= CHAR_HEIGHT / 3) {
                t -= CHAR_HEIGHT / 3;
                row++;
            }

            if (row >= __BOARD_DEPTH) {
                FLASH(0xD8, 30);
                rainX[i] = 255;
                continue;
            }

            // int row = ((rainY[i] >> (16)) * (0x10000 / ((CHAR_HEIGHT / 3)) + 1)) >>
            //           16; // trick! adjusted / 7 + a bit

            // #else
            int row = ((rainY[i] >> (16)) * 0x2493) >> 16;    // trick! adjusted / 7 + a bit
                                                              // #endif

            // #if ENABLE_DRIBBLE
            int pix = (rainY[i] >> 16) - row * (CHAR_HEIGHT / 3);
            // #endif

            if (pix >= CHAR_HEIGHT / 3) {
                FLASH(0xF8, 30);
                rainX[i] = 255;
                continue;
            }
            unsigned char *cell = ADDRESS_OF(row) + (rainX[i] >> 2);
            unsigned char img = GET(*cell);
            int type = CharToType[img];

            if (type == TYPE_ROCKFORD) {
                if (playerAnimationID == ID_Stand) {
                    if ((rainX[i] + 1) & 2) {
                        startPlayerAnimation(ID_Drip);
                        rainX[i] = UNUSED;
                        continue;
                    } else
                        startPlayerAnimation(ID_Talk2);
                }
            }

            if (!(Attribute[type] & ATT_ROCKFORDYBLANK)) {

#if ENABLE_DRIBBLE

                // if (Animate[type])
                //     img = *Animate[type];
                const unsigned char *shape = charSet[img] + pix * 3;

                if (type == TYPE_DIAMOND_WITHOUT_DIRT)
                    shape = EXTERNAL(__CHAR_DIAMOND_FALLING) + pix * 3;

                unsigned char iccPix = *shape | *(shape + 1) | *(shape + 2);

                if (rainSpeed[i] >= 0 && iccPix & (1 << (3 - (rainX[i] & 3)))) {

                    if (type >= TYPE_DIAMOND_WITHOUT_DIRT && type <= TYPE_DIAMONDX_PULSE_4) {
                        rainX[i]--;
                        rainY[i] -= rainSpeed[i];
                        rainSpeed[i] = (rainSpeed[i] * 1) >> 2;
                        break;
                    }

                    else if (type == TYPE_BOULDER) {

                        //                        rainY[i] -= rainSpeed[i];
                        rainSpeed[i] >>= 1;

                        if (pix >= 3) {
                            rainX[i] = UNUSED;
                            ADDAUDIO(SFX_DRIP2);
                            break;
                        }

                        //                        rainY[i] -= rainSpeed[i];

                        int diff = rainX[i] & 3;

                        if (diff < 2)
                            rainX[i]--;
                        else
                            rainX[i]++;

                        // rainSpeed[i] >>= 1;
                        //                          rainY[i] -= 0x1000;
                        //   rainSpeed[i] = 0x10000;
                        // break;
                        //                         continue;
                    }

                    else if (rainSpeed[i] > 0) {
                        rainSpeed[i] = RAIN_RESET_AFTER_IMPACT;
                        ADDAUDIO(SFX_DRIP2);
                    }

                    else if (rainSpeed[i] > RAIN_DEAD) {
                        rainX[i] = UNUSED;
                        continue;
                    }
                }
#else

                if (rainSpeed[i] > 0) {
                    rainSpeed[i] = RAIN_RESET_AFTER_IMPACT;
                    ADDAUDIO(SFX_DRIP2);
                }

                else if (rainSpeed[i] > RAIN_DEAD) {
                    rainX[i] = UNUSED;
                    continue;
                }

#endif
            }

            // rainSpeed[i] += RAIN_ACCEL;

            // if (rainSpeed[i] > 0)
            //     rainY[i] += rainSpeed[i];

            if ((rainSpeed[i] & 0x2000) || rainSpeed[i] > 0) {
                if (!drawBit(rainX[i], rainY[i] >> 16))
                    rainX[i] = UNUSED;
            }
        }
    }
}


#endif


// EOF
