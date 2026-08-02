// #include "defines_from_dasm_for_c.h"
#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

// #include "characterset.h"

#include "main.h"


#include "colour.h"
#include "drawPlayer.h"
#include "mellon.h"
#include "playerAnimation.h"
#include "random.h"
#include "scroll.h"

#include "reverseBits.h"
#include "sprites.h"


#define SCORE_SCANLINES 21 /* todo: aritrary ATM */
#define SPRITE_DEPTH 30


static int playerSpriteY;

static unsigned char playerBaseColour[16];
static unsigned char postProcessPlayerColours[16];

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


void initSprites() {

    playerSpriteY = -1;

    int rcol = getRandom32() & 0xF0;

    int oldlum = luminance;
    luminance = 0;

    for (int i = 0; i < 16; i++) {
        playerBaseColour[i] = playerColour[i] + rcol;
    }

    luminance = oldlum;
}

const unsigned char cx[][4] = {
    {0x40, 0x40, 0x40, 0x40},
    {0x20, 0x30, 0x40, 0x50},
};

void drawPlayerSprite() {    // --> 3956 max (30/5/2026)

    myMemsetInt((unsigned int *)(RAM + _BUF_GAME_GRP0), 0, _BUFFER_SIZE * 2 / 4);

    // Once the walk-in glide onto the teleport tile has settled (autoMoveFrameCount == 0,
    // same gate board.c's teleportLocked branch uses before it starts the countdown), let the
    // static + swirl dots read as the player being swallowed into the tile instead of drawing
    // them standing on top of it. Mirrored on arrival: stay hidden through (most of) the arrival
    // swirl (isTeleportArrivalPlayerHidden(), mellon.c) too, so the player doesn't pop into view
    // standing on the tile before it's finished generating -- they appear 0.5s before the swirl
    // itself actually ends (isTeleportArrivalPlayerHidden()'s own comment), not exactly when it
    // ends, same as they vanished into one on departure.
    //
    // This does NOT return early, though -- it only skips the actual shape-drawing loops
    // below (search "hidden" further down), after RAM[_P0_X]/playerSpriteY etc. have still
    // been computed and committed from this frame's real playerX/playerY/scrollX/scrollY.
    // GRP0 is already all zero from the memset above, so nothing shows regardless. Returning
    // early right here used to leave those position registers stale at whatever they were
    // the last time the sprite actually drew -- often still the *previous cave's* position,
    // since a full cave switch happens while hidden -- so the first visible frame, instead of
    // just appearing in the right place, had a frame or two of snapping over from the old
    // stale position as soon as something (this function, next call) finally recomputed it.
    // Also hidden once the exit-sequence fade (playerExitFade, updatePlayerAnimation() --
    // mellon.c's own comment) has pushed the player all the way to black -- 15 is where that
    // fade's luminance clamp guarantees full black for any base hue, so past that point they're
    // fully invisible already; skipping the draw outright avoids a faint black silhouette still
    // being visibly distinct against non-black backgrounds behind them (water, lava, etc., whose
    // own colouring the shape-drawing loops below apply on top of the luminance clamp).
    bool hidden = isPlayerHidden() || (exitMode && playerExitFade >= 15);

    static int root = 0;
    root++;

    int rooted = cx[1][(root >> 3) & 3];

    if (pulsePlayerColour) {
        --pulsePlayerColour;
        int cswitch = rangeRandom(4) ? 0x40 : 0x20;
        for (int i = 0; i < 16; i++)
            postProcessPlayerColours[i] = cswitch;
    }

    else {
        rooted = cx[0][(root >> 3) & 3];
        for (int i = 0; i < 16; i++)
            postProcessPlayerColours[i] = playerBaseColour[i];
    }

#if ENABLE_SHAKE
    int sX = scrollX + shakeX;
    if (sX < SCROLL_MIN_X)
        sX = SCROLL_MIN_X;
    if (sX > SCROLL_MAX_X)
        sX = SCROLL_MAX_X;

    int sY = scrollY + shakeY;
    if (sY < SCROLL_MIN_Y)
        sY = SCROLL_MIN_Y;
    if (sY > SCROLL_MAX_Y)
        sY = SCROLL_MAX_Y;

    int x = sX >> 16;
    int y = sY >> 16;
#else
    int x = scrollX >> 16;
    int y = scrollY >> 16;

#endif


    int ypos = (playerY + 1) * CHAR_Y - y * 3 - frameAdjustY * gravity - 8 + autoMoveY - SCORE_SCANLINES;
    int xpos = playerX * CHAR_TRIX_X - x;

    if (((frameAdjustY || frameAdjustX || autoMoveX || autoMoveY)) ||
        (xpos >= 0 && xpos < BOARD_TRIX_X && ypos >= 0 && ypos < _SCANLINES - CHAR_Y)) {


        const unsigned char *spr = spriteShape[*playerAnimation];
        if (!spr)
            return;    // shape not defined

        int shapeHeight = *spr++;

        ypos += CHAR_Y - (shapeHeight & 0x3f);

        int frameXOffset = -*(const signed char *)spr++;
        int frameYOffset = *(const signed char *)spr++;

        int lavaLine = ((liquidTrixel_8 >> 8) - (scrollY >> 16)) * 3;
        playerSpriteY = ypos - frameYOffset - 1;

        int pX = (xpos) * 4 + (faceDirection * (8 + frameXOffset + frameAdjustX + autoMoveX)) + 3;

        if (pulsePlayerColour) {

            int ppc = ((pulsePlayerColour * 1) >> 5) + 3;

            pX += rangeRandom(ppc) - (ppc >> 1);
            playerSpriteY += rangeRandom(ppc) - (ppc >> 1);
        }

        if (playerSpriteY < 0 || playerSpriteY >= _SCANLINES - SPRITE_DEPTH || pX < -7 || pX > 159)
            return;


        RAM[_P0_X] = pX;
        if (faceDirection == FACE_LEFT) {
            RAM[_P1_X] = RAM[_P0_X];
            RAM[_P0_X] += 8;
        } else
            RAM[_P1_X] = RAM[_P0_X] + 8;

        // Position is fully committed above -- everything from here down only decides what
        // gets drawn into GRP0, which the memset at the top already left at "nothing". See
        // this function's opening comment for why hiding happens here, not on early entry.
        //
        // Exit fade is the one hidden case that never comes back -- teleport's hidden window
        // ends with the player reappearing, so keeping pX/playerSpriteY fresh above matters
        // there (avoids a stale-position snap on the first visible frame, per this function's
        // own opening comment). Once the exit fade has taken over, there's no "reappearing" to
        // prepare for, and forcing the X position to 0 here (instead of leaving it at wherever
        // the player actually was) is what actually stops a visible artifact showing through --
        // just skipping the shape-drawing loops below wasn't enough on its own.
        if (hidden) {
            if (exitMode && playerExitFade >= 15) {
                RAM[_P0_X] = 0;
                RAM[_P1_X] = 0;
            }
            return;
        }

        int destLine = -1;

        if (gravity < 0)
            destLine = (shapeHeight & 0x3F) - 4;

        if (shapeHeight & SPRITE_DOUBLE) {

            unsigned char *p0Colour = RAM + _BUF_GAME_COLUP0 + playerSpriteY;
            unsigned char *p0 = RAM + _BUF_GAME_GRP0 + playerSpriteY;

            unsigned char *p1Colour = p0Colour + _SCANLINES + 2;
            unsigned char *p1 = p0 + _SCANLINES + 2;

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

                    p0Colour[destLine] = convertColour(postProcessPlayerColours[c1]);
                    p1Colour[destLine] = convertColour(postProcessPlayerColours[c2]);

                    if (playerSpriteY++ >= lavaLine) {
                        if (showWater) {
                            p0Colour[destLine] = 0x96;    // light blue -- matches colour.c's wbg[] underwater backdrop
                            p1Colour[destLine] = 0x96;
                        } else {
                            p0Colour[destLine] = ((p0Colour[destLine] & 0x0f) - 2) ^ (rooted & 0xF0);
                            p1Colour[destLine] = ((p1Colour[destLine] & 0x0f) - 2) ^ (rooted & 0xF0);
                        }
                    }
                }

                // <= 0 (not just < 0): once darkening has pushed a colour all the way down to
                // this hue's own darkest shade (lum == 0), the next step is true black (0x00),
                // not (colour & 0xF0) -- which would just leave that hue's floor showing forever,
                // never actually reaching black. Same reasoning both p0 and p1 below.
                int lum = (p0Colour[destLine] & 0xF) + luminance - playerExitFade;
                if (lum <= 0)
                    p0Colour[destLine] = 0;
                else {
                    if (lum > 15)
                        lum = 15;
                    p0Colour[destLine] = (p0Colour[destLine] & 0xF0) + lum;
                }
                lum = (p1Colour[destLine] & 0xF) + luminance - playerExitFade;
                if (lum <= 0)
                    p1Colour[destLine] = 0;
                else {
                    if (lum > 15)
                        lum = 15;
                    p1Colour[destLine] = (p1Colour[destLine] & 0xF0) + lum;
                }


                destLine += gravity;
            }

        }

        else {

            unsigned char *p0Colour = RAM + _BUF_GAME_COLUP0 + playerSpriteY;
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

                    p0Colour[destLine] = convertColour(postProcessPlayerColours[c1]);

                    if (playerSpriteY++ >= lavaLine) {
                        if (showWater)
                            p0Colour[destLine] = 0x96;    // light blue -- matches colour.c's wbg[] underwater backdrop
                        else
                            p0Colour[destLine] = ((p0Colour[destLine] & 0x0f) - 2) ^ (rooted & 0xF0);
                    }
                }
                // <= 0 (not just < 0): see the SPRITE_DOUBLE branch above's comment -- once
                // darkening bottoms out at this hue's own floor, the next step is true black.
                int lum = (p0Colour[destLine] & 0xF) + luminance - playerExitFade;
                if (lum <= 0)
                    p0Colour[destLine] = 0;
                else {
                    if (lum > 15)
                        lum = 15;
                    p0Colour[destLine] = (p0Colour[destLine] & 0xF0) + lum;
                }


                destLine += gravity;
            }
        }
    }
}

// EOF