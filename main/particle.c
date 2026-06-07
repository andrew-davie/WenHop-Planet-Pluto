#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "attribute.h"
#include "characterset.h"
#include "decodeCaves.h"
#include "drawscreen.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "player.h"
#include "random.h"
#include "scroll.h"

unsigned int weaponLength = 0;
unsigned char ropeDirection[ROPE_PARTICLE_COUNT];

// const int xsin[32] = {
//     //    0, 97, 181, 236, 256, 236, 181, 97, 0, -97, -181, -236, -256, -236, -181, -97,
//     0, 49,  97,  142,  181,  212,  236,  251,  256,  251,  236,  212,  181,  142,  97,  49,
//     0, -49, -97, -142, -181, -212, -236, -251, -256, -251, -236, -212, -181, -142, -97, -49,
// };


const int xsin[32] = {0, 50,  98,  142,  181,  213,  237,  251,  256,  251,  237,  213,  181,  142,  98,  50,
                      0, -50, -98, -142, -181, -213, -237, -251, -256, -251, -237, -213, -181, -142, -98, -50};


void modifyCharAtTip(int x, int y) {

    unsigned char colour = 0;

    int xchar = (x * (256 / CHAR_TRIX_X)) >> 16;
    int ychar = (y * (256 / CHAR_TRIX_Y)) >> 16;

    if (xchar < 0 || xchar >= _BOARD_COLS || ychar < 0 || ychar >= _BOARD_ROWS)
        return;

    unsigned char *b = RAM + _BOARD + ychar * _BOARD_COLS + xchar;
    int ch = CharToType[GET(*b)];


    if (Attribute[ch] & ATT_EXPLODABLE) {

        if (ch == TYPE_DOGE) {
            *b = CH_BLANK | FLAG_THISFRAME;
            colour = rangeRandom(7) + 1;
        }

        else if (ch == TYPE_ROCK) {
            *b = CH_GEODOGE | FLAG_THISFRAME;
            colour = 1;
        }

        else if (ch == TYPE_GEODOGE) {
            *b = CH_DOGE_00 | FLAG_THISFRAME;
            colour = 3;
        }

        else if (ch == TYPE_DIRT) {
            *b = CH_DUST_0 | FLAG_THISFRAME;
            colour = 2;
        }

        else {

            if (xchar > 0 && xchar < _BOARD_COLS && ychar > 0 && ychar < _BOARD_ROWS) {

                if (ch == TYPE_BRICKWALL) {

                    *b = CH_DUST_0 | FLAG_THISFRAME;
                    colour = 7;
                }

                else if (ch == TYPE_STEELWALL) {
                    *b = CH_DUST_0 | FLAG_THISFRAME;
                    colour = 7;
                }
            }
        }

        if (*b & FLAG_THISFRAME)
            nDotsAtTrixel(5, (x >> 8) + (CHAR_TRIX_Y >> 1), (y >> 8) + (CHAR_TRIX_Y >> 1), 30, 50, colour);
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

    if (T1TC > availableIdleTime - 3000)
        return;

    if (theCave->weapon[level] != WEAPON_MACE)
        return;

    if ((inpt4 & 0x80) && !weaponLength) {
        weaponLength = 0;
        return;
    }


    if (inpt4 & 0x80)
        weaponLength--;

    //    if ((RAM[_INPT4] & 0x80) || theCave->weapon[level] != WEAPON_ROPE)
    //      return;

    else if (weaponLength < ROPE_PARTICLE_COUNT)
        weaponLength++;


    // if (ropeLength > ROPE_PARTICLE_COUNT)    // relies on unsigned int arithmetic
    //    ropeLength = ROPE_PARTICLE_COUNT;

    int baseX = (playerX * CHAR_TRIX_X + 2 + ((faceDirection * autoMoveX) >> 2)) << 8;
    int baseY = ((playerY * CHAR_TRIX_Y + 6) << 8) + (autoMoveY * (256 / 3)) - 3;

    int x = 0, y = 0;

    for (unsigned int i = 0; i < weaponLength; i++) {
        x += (xsin[(ropeDirection[i] >> 3) & 0x1F] * PIXEL_ASPECT) >> 8;
        y += (xsin[((ropeDirection[i] >> 3) + 8) & 0x1F] * 256) >> 8;

        if (x < -0100 || x > 0x200 || y < -0x300 || y > 0x000)
            drawBit((baseX + x) >> 8, ((baseY + y) >> 8), 1);
    }


    // The ball
    // clang-format off
    struct BALL {
        unsigned char x;
        unsigned char y;
    };
    
    struct BALL ball[] = {
        {1,0},
        {0,1},{1,1},{2,1},
        {0,2},{1,2},{2,2},
        {0,3},{1,3},{2,3},
        {0,4},{1,4},{2,4},
        {1,5},

    };

    // clang-format on

    for (int i = 0; i < sizeof(ball) / sizeof(ball[0]); i++)
        drawBit(((baseX + x) >> 8) + ball[i].x - 1, ((baseY + y) >> 8) + ball[i].y - 2, 7);

    if (!(inpt4 & 0x80))
        nDots(1, 0, 0, PT_TWO, 50, (baseX + x) >> 8, (baseY + y) >> 8, 20, 6);


    modifyCharAtTip(baseX + x, baseY + y);

    static short unsigned int wantedDirection = 0;

    if (ropeDirection[0] == wantedDirection >> 8)
        wantedDirection = getRandom32();

    ropeDirection[0] = turn_toward(ropeDirection[0], wantedDirection >> 8, 5);

    for (int i = weaponLength - 1; i > 0; i--)
        ropeDirection[i] = ropeDirection[i - 1];
}


void drawGun() {

    static int gunDelay = 0;

    if (--gunDelay < 0)
        gunDelay = 0;

    if (gunDelay || (inpt4 & 0x80) || theCave->weapon[level] != WEAPON_GUN)
        return;

    gunDelay = 10;

    int baseX = (playerX * CHAR_TRIX_X + 2 + ((faceDirection * autoMoveX) >> 2)) << 8;
    int baseY = ((playerY * CHAR_TRIX_Y + 6) << 8) + (autoMoveY * (256 / 3)) - 3;

    int x = 0, y = 0;

    int idx = sphereDot(baseX >> 8, baseY >> 8, PT_ONE, 200, 7);
    if (idx >= 0) {
        particle[idx].direction = 64;
        particle[idx].speed = 250;
    }


    // nDots(1, 0, 0, PT_TWO, 50, (baseX + x) >> 8, (baseY + y) >> 8, 20, 6);
    // nDots(1, 0, 0, PT_TWO, 50, (baseX + x) >> 8, ((baseY + y) >> 8) + 1, 20, 6);
}


void drawRope() {

    if ((RAM[_INPT4] & 0x80) || theCave->weapon[level] != WEAPON_ROPE)
        return;

    if (--weaponLength > ROPE_PARTICLE_COUNT)    // relies on unsigned int arithmetic
        weaponLength = ROPE_PARTICLE_COUNT;

    int baseX = (playerX * 5 + 2 + ((faceDirection * autoMoveX) >> 2)) << 8;
    int baseY = ((playerY * 10 + 6) << 8) + (autoMoveY * (256 / 3));

    int x = 0, y = 0;

    for (unsigned int i = 0; i < weaponLength; i++) {
        x += (xsin[(ropeDirection[i] >> 3) & 0x1F] * PIXEL_ASPECT) >> 8;
        y += (xsin[((ropeDirection[i] >> 3) + 4) & 0x1F] * 256) >> 8;

        drawBit((baseX + x) >> 8, (baseY + y) >> 8, 6);
        drawBit((baseX - x) >> 8, (baseY - y) >> 8, 6);
    }

    modifyCharAtTip(baseX + x, baseY + y);
    modifyCharAtTip(baseX - x, baseY - y);

    static int wantedDirection = 0;

    if (ropeDirection[0] == wantedDirection)
        wantedDirection = rangeRandom(256);

    else if (wantedDirection > ropeDirection[0])
        ropeDirection[0] += ((wantedDirection - ropeDirection[0]) >> 2) + 1;

    else
        ropeDirection[0] -= ((ropeDirection[0] - wantedDirection) >> 2) + 1;

    for (int i = weaponLength - 1; i > 0; i--)
        ropeDirection[i] = (ropeDirection[i] + ropeDirection[i - 1] * 3) >> 2;
}

struct Particle particle[PARTICLE_COUNT];

void initParticles() {

    for (int i = 0; i < PARTICLE_COUNT; i++)
        particle[i].age = 0;
}


void drawParticles() {

    for (int i = 0; i < PARTICLE_COUNT; i++) {

        if (T1TC > availableIdleTime - 500)
            return;

        if (particle[i].age) {

            int xOffset = (xsin[particle[i].direction >> 3] * particle[i].distance) >> 8;
            int yOffset = (xsin[((particle[i].direction >> 3) + 8) & 0x1F] * particle[i].distance * 3) >> 8;

            int y = (particle[i].y + yOffset) >> 8;
            int x = (particle[i].x + xOffset) >> 8;

            switch (particle[i].type) {

            case PT_TWO: {

                break;
            }

            case PT_SPIRAL2:
            case PT_SPIRAL: {

                particle[i].direction += PARTICLE_SPIRAL_ANGULAR_SPEED;

                if (!rangeRandom(250))
                    nDotsAtTrixel(4, x, y, 30, 0x20, particle[i].colour);
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

            if (!drawBit(x, y, particle[i].colour))
                particle[i].age = 0;

            else {

                --particle[i].age;

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

            int oldest = 0;
            while (++whichDrop < PARTICLE_COUNT && particle[whichDrop].age)
                if (particle[whichDrop].age < particle[oldest].age)
                    oldest = whichDrop;

            if (whichDrop == PARTICLE_COUNT)
                whichDrop = oldest;

            particle[whichDrop].type = type;
            particle[whichDrop].x = dotX << 8;

            particle[whichDrop].y = dotY << 8;
            particle[whichDrop].speed = 0;
            particle[whichDrop].age = age;
            particle[whichDrop].colour = colour;

            particle[whichDrop].direction = getRandom32();    // 16.16 angle
            particle[whichDrop].distance = 96;                // 16.16 speed
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
            if (type == PT_SPIRAL2)
                particle[idx].distance = rangeRandom(200) + 50;
        }
    }
}

void nDotsBackwards(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed) {

    if (gravity < 0)
        offsetY = CHAR_TRIX_Y - offsetY;

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX * CHAR_TRIX_X + offsetX, dripY * CHAR_TRIX_Y + offsetY, type, age, 3);

        // TODO vector
        particle[idx].x += particle[idx].age * particle[idx].speed;
        particle[idx].y += particle[idx].age * particle[idx].speed;

        // particle.speedX[idx] = -particle.speedX[idx];
        // particle.speedY[idx] = -particle.speedY[idx];
    }
}

void nDotsAtTrixel(int count, int dripX, int dripY, unsigned char age, int speed, unsigned char colour) {

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX, dripY, PT_SPIRAL, age, colour);
        if (idx >= 0)
            particle[idx].speed = speed;
    }
}


// EOF
