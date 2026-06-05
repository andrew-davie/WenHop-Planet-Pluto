// #include "defines_from_dasm_for_c.h"
#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "characterset.h"

#include "main.h"


#include "colour.h"
#include "drawPlayer.h"
#include "mellon.h"
#include "player.h"
#include "random.h"
#include "scroll.h"

#include "reverseBits.h"

#define SCORE_SCANLINES 21 /* todo: aritrary ATM */
#define SPRITE_DEPTH 30


static int playerSpriteY;

unsigned char dynamicPlayerColours[16];
unsigned char playerBaseColour[16];
unsigned char postProcessPlayerColours[16];

void initSprites() {

    playerSpriteY = -1;

    int rcol = getRandom32() & 0xF0;

    static const unsigned char playerColour[] = {

        0x28,    // 00 HAIR
        0x34,    // 01 SKIN
        0x06,    // 02 TOP1
        0x06,    // 03 TOP2
        0x44,    // 04 BOOT
        0x44,    // 05 PANT
        0x58,    // 06 BELT
        0x08,    // 07 SOLE
        0x08,    // 08 BONE
        0x2C,    // 09 HMT0
        0x28,    // 10 HMT1
        0x26,    // 11 HMT2
        0x22,    // 12 HMT3
        0x4A,    // 13 BDY0
        0x46,    // 14 BDY1
        0x44,    // 15 BDY2
    };

    for (int i = 0; i < 16; i++) {
        playerBaseColour[i] = dynamicPlayerColours[i] = convertColour(playerColour[i]) + rcol;
        postProcessPlayerColours[i] = dynamicPlayerColours[i] & 0xF0;
    }
}

const unsigned char cx[][4] = {
    {0x40, 0x40, 0x40, 0x40},
    {0x20, 0x30, 0x40, 0x50},
};

void drawPlayerSprite() {    // --> 3956 max (30/5/2026)

    myMemsetInt((unsigned int *)(RAM + _BUF_GAME_GRP0), 0, _BUFFER_SIZE * 2 / 4);

    static int root = 0;
    root++;

    int rooted = cx[1][(root >> 3) & 3];

    if (pulsePlayerColour) {
        //    if (--pulsePlayerColour) {
        --pulsePlayerColour;                           //       if (!(--pulsePlayerColour & 7)) {
        int cswitch = rangeRandom(4) ? 0x40 : 0x20;    // getRandom32() & 0xF0;
        for (int i = 0; i < 16; i++)
            postProcessPlayerColours[i] = cswitch;
        //     }
    }

    else {
        rooted = cx[0][(root >> 3) & 3];
        for (int i = 0; i < 16; i++)
            postProcessPlayerColours[i] = playerBaseColour[i] & 0xF0;
    }

#if ENABLE_SHAKE
    int x = (scrollX + shakeX) >> 16;
    int y = (scrollY + shakeY) >> 16;
#else
    int x = scrollX >> 16;
    int y = scrollY >> 16;

#endif


    int ypos = (playerY + 1) * CHAR_Y - y * 3 - frameAdjustY * gravity - 8 + autoMoveY - SCORE_SCANLINES;
    int xpos = playerX * CHAR_TRIX_X - x;


    // static int txp = 0;
    // if ((++txp >> 3) > 38)
    //     txp = 0;
    // xpos = txp >> 3;
    // ypos = 20;    // tmp

    if (((frameAdjustY || frameAdjustX || autoMoveX || autoMoveY)) ||
        (xpos >= 0 && xpos < BOARD_TRIX_X && ypos >= 0 && ypos < _SCANLINES - CHAR_Y)) {


        const unsigned char *spr = spriteShape[*playerAnimation];
        // if (!spr) {    // tmp

        //     FLASH(0x44, 2);
        //     spr = spriteShape[1];
        // }


        int shapeHeight = *spr++;

        ypos += CHAR_Y - (shapeHeight & 0x3f);

        int frameXOffset = *(const signed char *)spr++;
        int frameYOffset = *(const signed char *)spr++;

        int lavaLine = (lavaSurfaceTrixel - (scrollY >> 16)) * 3;
        playerSpriteY = ypos - frameYOffset - 1;

        int pX = (xpos) * 4 + (faceDirection * (frameXOffset + frameAdjustX + autoMoveX)) + 2;

        if (pulsePlayerColour) {

            int ppc = ((pulsePlayerColour * 1) >> 5) + 3;

            pX += rangeRandom(ppc) - (ppc >> 1);
            playerSpriteY += rangeRandom(ppc) - (ppc >> 1);
        }

        if (playerSpriteY < 0 || playerSpriteY >= _SCANLINES - SPRITE_DEPTH || pX > 159)
            return;


        RAM[_P0_X] = pX;
        if (faceDirection == FACE_LEFT) {
            RAM[_P1_X] = RAM[_P0_X];
            RAM[_P0_X] += 8;
        } else
            RAM[_P1_X] = RAM[_P0_X] + 8;

        extern int gravity;
        int destLine = -1;

        if (gravity < 0)
            destLine = (shapeHeight & 0x3F) - 4;

        if (shapeHeight & SPRITE_DOUBLE) {

            unsigned char *p0Colour = RAM + _BUF_GAME_COLUP0 + playerSpriteY + 1;
            unsigned char *p0 = RAM + _BUF_GAME_GRP0 + playerSpriteY;

            unsigned char *p1Colour = p0Colour + _SCANLINES;
            unsigned char *p1 = p0 + _SCANLINES;

            for (int line = 0; line < (shapeHeight & 0x3f); line++) {

                if (faceDirection == FACE_RIGHT) {
                    p0[destLine] = *spr++;
                    p1[destLine] = *spr++;
                }

                else {
                    p0[destLine] = reverseBits[(unsigned char)*spr++];
                    p1[destLine] = reverseBits[(unsigned char)*spr++];
                }

                int c1 = *spr >> 4;
                int c2 = *spr++ & 0xF;

                if (shapeHeight & SPRITE_ABSCOLOUR) {
                    p0Colour[destLine] = c1;
                    p1Colour[destLine] = c2;
                }

                else {

                    p0Colour[destLine] = (dynamicPlayerColours[c1] & 0xF) ^ postProcessPlayerColours[c1];
                    p1Colour[destLine] = (dynamicPlayerColours[c2] & 0xF) ^ postProcessPlayerColours[c2];

                    if (playerSpriteY++ >= lavaLine) {
                        p0Colour[destLine] = ((p0Colour[destLine] & 0x0f) - 2) ^ (rooted & 0xF0);
                        p1Colour[destLine] = ((p1Colour[destLine] & 0x0f) - 2) ^ (rooted & 0xF0);
                    }
                }

                destLine += gravity;
            }

        }

        else {

            unsigned char *p0Colour = RAM + _BUF_GAME_COLUP0 + playerSpriteY + 1;
            unsigned char *p0 = RAM + _BUF_GAME_GRP0 + playerSpriteY;

            for (int line = 0; line < (shapeHeight & 0x3F); line++) {

                if (faceDirection == FACE_RIGHT)
                    p0[destLine] = *spr++;
                else
                    p0[destLine] = reverseBits[(unsigned char)*spr++];

                int c1 = *spr++ >> 4;

                if (shapeHeight & SPRITE_ABSCOLOUR)
                    p0Colour[destLine] = c1;

                else {

                    p0Colour[destLine] = (dynamicPlayerColours[c1] & 0xF) ^ postProcessPlayerColours[c1];

                    if (playerSpriteY++ >= lavaLine) {
                        p0Colour[destLine] = ((p0Colour[destLine] & 0x0f) - 2) ^ (rooted & 0xF0);
                    }
                }

                destLine += gravity;
            }
        }
    }
}

// EOF