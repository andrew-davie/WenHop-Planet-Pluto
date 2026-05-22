#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "attribute.h"
#include "characterset.h"
#include "drawscreen.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "player.h"
#include "random.h"
#include "scroll.h"

unsigned int ropeLength;
unsigned char ropeDirection[ROPE_PARTICLE_COUNT];

const int xsin[32] = {
    //    0, 97, 181, 236, 256, 236, 181, 97, 0, -97, -181, -236, -256, -236, -181, -97,
    0, 49,  97,  142,  181,  212,  236,  251,  256,  251,  236,  212,  181,  142,  97,  49,
    0, -49, -97, -142, -181, -212, -236, -251, -256, -251, -236, -212, -181, -142, -97, -49,
};

void modifyCharAtTip(int x, int y) {

    int xchar = (x * (256 / 5)) >> 16;
    int ychar = (y * (256 / 10)) >> 16;

    unsigned char *b = RAM + _BOARD + ychar * _BOARD_COLS + xchar;
    int ch = CharToType[GET(*b)];
    //    int audio = SFX_DRIP;

    if (ch == TYPE_ROCK) {
        *b = CH_GEODOGE | FLAG_THISFRAME;
        //      audio = SFX_ROCK;
    } else if (ch == TYPE_GEODOGE) {
        *b = CH_DOGE_00 | FLAG_THISFRAME;
        //    audio = SFX_DOGE3;
    } else if (ch == TYPE_DIRT) {
        *b = CH_DUST_0 | FLAG_THISFRAME;
        //  audio = SFX_DIRT;
    } else if (ch == TYPE_DOGE) {
        //  audio = SFX_BUBBLER;
        *b = CH_DOGE_00 | FLAG_THISFRAME;
    }

    if (*b & FLAG_THISFRAME) {

        // if (audio != SFX_DRIP)
        //     ADDAUDIO(audio);
        nDotsAtTrixel(5, (x >> 8) + 2, (y >> 8) + 5, 30, 50);
    }
}

// bool ropeEnabled = false;
const int PIXEL_ASPECT = 181;

void drawRope() {

    if (--ropeLength > ROPE_PARTICLE_COUNT)    // relies on unsigned int arithmetic
        ropeLength = ROPE_PARTICLE_COUNT;

    int baseX = (playerX * 5 + 2 + ((faceDirection * autoMoveX) >> 2)) << 8;
    int baseY = ((playerY * 10 + 6) << 8) + (autoMoveY * (256 / 3));

    int x = 0, y = 0;

    for (unsigned int i = 0; i < ropeLength; i++) {

        x += (xsin[(ropeDirection[i] >> 3) & 0x1F] * PIXEL_ASPECT) >> 8;
        y += (xsin[((ropeDirection[i] >> 3) + 4) & 0x1F] * 256) >> 8;

        drawBit((baseX + x) >> 8, (baseY + y) >> 8);
        drawBit((baseX - x) >> 8, (baseY - y) >> 8);
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

    for (int i = ropeLength - 1; i > 0; i--)
        ropeDirection[i] = (ropeDirection[i] + ropeDirection[i - 1] * 3) >> 2;
}

struct Particle particle[PARTICLE_COUNT];

void drawParticles() {

    for (int i = 0; i < PARTICLE_COUNT; i++) {
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
                    nDotsAtTrixel(4, x, y, 30, 0x20);
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

            if (!drawBit(x, y))
                particle[i].age = 0;

            else {

                --particle[i].age;

                particle[i].distance += particle[i].speed;
            }
        }
    }
}

int sphereDot(int dotX, int dotY, int type, unsigned char age) {

    int whichDrop = -1;

    int col = dotX - ((scrollX * 5) >> 16);
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
            particle[whichDrop].speed = 0;    // rangeRandom(15) + 16;
            particle[whichDrop].age = age;

            particle[whichDrop].direction = getRandom32();    // 16.16 angle
            particle[whichDrop].distance = 96;                // 16.16 speed
        }
    }

    return whichDrop;
}


void nDots(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed) {

    if (gravity < 0)
        offsetY = TRILINES - offsetY;

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX * 5 + offsetX, dripY * TRILINES + offsetY, type, age);
        if (idx >= 0) {
            particle[idx].speed = rangeRandom(speed >> 1);
            if (type == PT_SPIRAL2)
                particle[idx].distance = rangeRandom(200) + 50;
        }
    }
}

void nDotsBackwards(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed) {

    if (gravity < 0)
        offsetY = TRILINES - offsetY;

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX * 5 + offsetX, dripY * TRILINES + offsetY, type, age);

        // TODO vector
        particle[idx].x += particle[idx].age * particle[idx].speed;
        particle[idx].y += particle[idx].age * particle[idx].speed;

        // particle.speedX[idx] = -particle.speedX[idx];
        // particle.speedY[idx] = -particle.speedY[idx];
    }
}

void nDotsAtTrixel(int count, int dripX, int dripY, unsigned char age, int speed) {

    for (int i = 0; i < count; i++) {
        int idx = sphereDot(dripX, dripY, PT_SPIRAL, age);
        if (idx >= 0)
            particle[idx].speed = speed;
    }
}


// EOF
