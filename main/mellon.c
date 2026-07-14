#include "defines_dasm.h"

#include "animations.h"
#include "attribute.h"
#include "board.h"
#include "caveData.h"
#include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "gameState.h"
#include "kernels.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "random.h"
#include "score.h"
#include "sound.h"

int playerX;    // char pos 0-39 (use *5 for pixel)
int playerY;    // char pos 0-21 (use *10 for pixel and then *3 for scanline)

int frameAdjustX;
int frameAdjustY;
unsigned int pushCounter;
enum FaceDirectionX faceDirection;
bool playerDead;
int waitForNothing;
bool handled;
bool gearsActive;
bool gearsWaitRelease;

static unsigned char *meAtt;
static bool drop = false;


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

const unsigned char tapAnimation[] = {
    ID_TapUp,
    ID_TapPush,
    ID_TapDown,
    ID_TapPush,
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

    faceDirection = FACE_RIGHT;

    gearsActive = false;
    gearsWaitRelease = false;

    drop = false;
    attachment = 0;
    attachmentOffset = 0;

    startPlayerAnimation(ID_Stand);    // tmp
    // startPlayerAnimation(ID_Stand);
}

// void chooseIdleAnimation() {
//  return;

// #if ENABLE_IDLE_ANIMATION
// #define ANIM_COUNT (sizeof(animID)/2)

//     static const char animID[] = {
//         // ID_Blink,       200,
// //        ID_WipeHair,    120,
// //        ID_Impatient,   112,
//         ID_Turn,        117,
// //        ID_Look,        130,
//         // ID_Shades,      125,
//         // ID_ArmsCrossed, 113,
//     };

//     // suicide skeleton
// //     if (selectResetDelay > DEAD_RESTART_COUCH * 3 / 5) {
// //         if (playerAnimationID != ID_Skeleton2) {
// //             startPlayerAnimation(ID_Skeleton2);
// // //            ADDAUDIO(SFX_DRIP);
// //             SAY(__WORD_GOODBYE);
// //         }
// //     }

// //    else
//      {

//         // choose an idle animation
//         if ((inpt4 & 0x80) && usableSWCHA == 0xFF) {

//             if (playerAnimationID == ID_Skeleton2)
//                 startPlayerAnimation(ID_Stand);             // abort from
//                 suicide

//             else if (playerAnimationID == ID_Stand) {

//                 if (getRandom32() < 0x700000) {
//                     int idle = rangeRandom(ANIM_COUNT) << 1;
//                     if ((rndX & 0xFFF) < animID[idle + 1])
//                         startPlayerAnimation(animID[idle]);
//                 }
//             }
//         }
//     }

// #endif
// }

void grabDoge() {

    totalDogePossible--;

    // if (doges > 0)
    addScore(100);    // theCave->dogeValue);

    if (!--doges) {
        exitTrigger = true;
        //        FLASH(0x08, 8);     //open door
        ADDAUDIO(SFX_EXIT);
    } else
        ADDAUDIO(SFX_DOGE2);
}

int playerSlow = 0;
int tapDelay = 0;


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


const OFFSET sampleOffsetRight[] = {

    {-5, -2 * 3}, {-5, -4 * 3}, {-4, -6 * 3}, {-4, -6 * 3}, {-3, -7 * 3},
    {-2, -7 * 3}, {-1, -7 * 3}, {0, -8 * 3},  {0, 0},
};

const OFFSET sampleOffsetLeft[] = {

    {5, -2 * 3}, {5, -4 * 3}, {4, -6 * 3}, {4, -6 * 3}, {3, -7 * 3}, {2, -7 * 3}, {1, -7 * 3}, {0, -8 * 3}, {0, 0},
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


const OFFSET dropOffsetRight[] = {

    {0, -8 * 3},     //
    {-1, -7 * 3},    //
    {-2, -7 * 3},    //
    {-3, -7 * 3},    //
    {-4, -6 * 3},    //
    {-4, -6 * 3},    //
    {-5, -4 * 3},    //
    {-5, -2 * 3},    //
    {0, 0},          //
};

const OFFSET dropOffsetLeft[] = {

    {0, -8 * 3},    //
    {1, -7 * 3},    //
    {2, -7 * 3},    //
    {3, -7 * 3},    //
    {4, -6 * 3},    //
    {4, -6 * 3},    //
    {5, -4 * 3},    //
    {5, -2 * 3},    //
    {0, 0},         //
};

const OFFSET dropOffsetDown[] = {

    {0, -8 * 3},     //
    {-1, -7 * 3},    //
    {-1, -6 * 3},    //
    {-2, -5 * 3},    //
    {-2, -2 * 3},    //
    {-2, 2 * 3},     //
    {-1, 5 * 3},     //
    {0, 8 * 3},      //
    {0, 0},          //
};

const OFFSET dropOffsetUp[] = {
    {0, -8 * 3},     //
    {1, -8 * 3},     //
    {1, -10 * 3},    //
    {0, -12 * 3},    //
    {0, -14 * 3},    //
    {1, -13 * 3},    //
    {1, -12 * 3},    //
    {1, -11 * 3},    //
    {0, 0},          //
};


const OFFSET *dropOffset[] = {
    dropOffsetUp,       // up
    dropOffsetRight,    // right
    dropOffsetDown,     // down
    dropOffsetLeft,     // left
};

bool checkHighPriorityMove(int dir) {

    static int zapDelay = 0;
    if (zapDelay)
        --zapDelay;


    unsigned char joyBit = joyDirectBit[dir] << 4;
    if (usableSWCHA & joyBit)
        return false;

    getRandom32();


    if (!waitRelease) {
        if (!(inpt4 & 0x80)) {

            meAtt = me + dirOffset[dir];

            if (attachment) {

                int type2 = CharToType[GET(*meAtt)];
                if (Attribute[type2] & ATT_BLANK) {

                    if (type2 == TYPE_GEODOGE)
                        attachment = CH_GEODOGE_FALLING;

                    else if (type2 == TYPE_ROCK)
                        attachment = CH_ROCK_FALLING;

                    if (dir == 1 || dir == 2)
                        attachment |= 0x80;

                    drop = true;
                    attachmentOffset = dropOffset[dir];
                    return true;
                }
            }

            else {

                unsigned char pickup = PickupCharacter[CharToType[GET(*meAtt)]];
                if (pickup) {

                    attachment = GET(pickup);

                    if (dir == 1 || dir == 2)
                        attachment |= 0x80;

                    attachmentOffset = pickupOffset[dir];

                    *meAtt = FLAG(CH_DUST_0);
                    waitRelease = true;
                    return true;
                }
            }
        }
    }


    if (faceDirectionDef[dir] && faceDirection != faceDirectionDef[dir]) {
        pushCounter = 0;    // so we get an animation during turn
        faceDirection = faceDirectionDef[dir];
    }

    unsigned char *meOffset = me + dirOffset[dir];
    enum ObjectType destType = CharToType[GET(*meOffset)];

    {    // no fire button


        //        bool grabbed = false;
        int type = CharToType[GET(*meOffset)];

        if (tapDelay)
            tapDelay--;

        // turn on/off a tap
        if (!tapDelay && type == TYPE_TAP) {
            *meOffset = *meOffset ^ (CH_TAP_1 ^ CH_TAP_0);
            if (*meOffset == CH_TAP_1) {
                // showWater = true;
                // showLava = false;
                if (21 * CHAR_TRIX_Y < lavaSurfaceTrixel)
                    lavaSurfaceTrixel = 21 * CHAR_TRIX_Y;
            }

            *(meOffset + _1ROW) = (GET(*(meOffset + _1ROW)) == CH_HUB) ? CH_HUB_1 : CH_HUB;

            startPlayerAnimation(tapAnimation[dir]);
            tapDelay = 10;

            waitForNothing = 6;
            //            startPlayerAnimation(ID_Stand);
            handled = true;
        }

        // else if (type == TYPE_WEAPON) {

        //     *meOffset = FLAG(CH_MELLON_HUSK);
        //     *me = FLAG(CH_BLANK);
        //     extern int weapon;
        //     weapon = theCave->weapon[level];
        //     return handled = true;
        // }

        else if (type == TYPE_GRINDER || type == TYPE_GRINDER_1) {
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


            // startPlayerAnimation(ID_Turn);

            //            if (!zapDelay || !rangeRandom(4))
            FLASH(0x28, 13);

            pulsePlayerColour = 100;

            if (!zapDelay || (!rangeRandom(30))) {
                zapDelay = 50;
                // ADDAUDIO(SFX_ZAP2);
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

            // if (!totalDogePossible) {

            ADDAUDIO(SFX_ZAP2);
            ADDAUDIO(SFX_WHOOSH);
            weapon = theCave->weapon[level];
            nDots(10, playerX + xdir[dir], playerY + ydir[dir], PT_SPIRAL2, 40, 3, 4, 50, 7);
            // FLASH(0x2A, 4);

            playerX += xdir[dir];
            playerY += ydir[dir];

            moveHusk(dir, me, meOffset);

            int dir2 = (gravity < 0) ? dir ^ 2 : dir;

            if (playerAnimationID != WalkAnimation[dir2])
                startPlayerAnimation(WalkAnimation[dir2]);

            if (!autoMoveFrameCount) {

                autoMoveFrameCount = ((MOVE_SPEED) << playerSlow);

                autoMoveX = autoMoveDeltaX = animDeltaX[dir] >> playerSlow;
                autoMoveY = autoMoveDeltaY = animDeltaY[dir] >> playerSlow;
            }

            handled = true;
            // }

            // else {
            //     FLASH(0x40, 10);
            //     ADDAUDIO(SFX_ROCK);
            // }
        }


        else if (Attribute[destType] & (ATT_BLANK | ATT_PERMEABLE | ATT_GRAB | ATT_EXIT)) {

            // startCharAnimation(TYPE_MELLON_HUSK,
            // AnimateBase[TYPE_MELLON_HUSK]);

            pushCounter = 0;

            if (Attribute[destType] & ATT_BLANK)
                ADDAUDIO(SFX_SPACE);

            else if (destType == TYPE_OUTBOX) {
                *meOffset = CH_EXITBLANK;
                ADDAUDIO(SFX_WHOOSH);
                exitMode = 151;
                waitRelease = true;
            }

            else if (destType == TYPE_FLIP_GRAVITY) {
                nextGravity = -gravity;
                FLASH(0xC5, 3);
#if ENABLE_SHAKE
                setShake(20);
#endif
                // ADDAUDIO(SFX_SCORE);
            }

            // else if (destType == TYPE_EASTEREGG) {
            //     FLASH(0x8, 10);
            //     time += 50 << 8;
            //     lockDisplay = false;
            // }

            if (Attribute[destType] & ATT_GRAB) {
                grabDoge(/* meOffset */);
                //     grabbed = true;
                // if (grabbed)
                nDots(10, playerX + xdir[dir], playerY + ydir[dir], PT_SPIRAL2, 40, 3, 4, 50, 7);
            }


            playerX += xdir[dir];
            playerY += ydir[dir];

            frameAdjustX = frameAdjustY = 0;

            if (!exitMode) {

                moveHusk(dir, me, meOffset);
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
                        if ((ATTRIBUTE_BIT(*(me + dirOffset[d]), ATT_PIPE)) ||
                            GET(*(me + dirOffset[d])) == CH_MELLON_HUSK)
                            udlr |= 1 << d;
                    }

                    *me = udlrChar[udlr];

                    // if (*me == CH_HUB_1 && Attribute[CharToType[GET(*(me - _BOARD_COLS))]] &
                    // ATT_BLANK)
                    //     *(me - _BOARD_COLS) = CH_TAP_0;

                    showTool = true;
                }
            }

            // else
            //     // *me = FLAG(CH_DUST_ROCK_0);
            //     *me = FLAG(CH_BLANK);

            playerSlow = 0;
            if (!autoMoveFrameCount && ((Attribute[destType] & (ATT_DIRT | ATT_WATERFLOW)) || destType == TYPE_LAVA)) {

                // playerSlow = 1;    // tmp ((Attribute[destType] & ATT_WATERFLOW)) ? 2 : 1;

                ADDAUDIO(SFX_DIRT);
                // dirtFlag = true;
                startCharAnimation(TYPE_MELLON_HUSK, AnimateBase[TYPE_MELLON_HUSK]);

                //                nDots(6, playerX, playerY, PT_ONE, 25, 2, 5, 50);
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

    if (handled) {
        bufferedSWCHA |= joyBit;
        usableSWCHA |= joyBit;
        inhibitSWCHA = joyBit;
    }

    return handled;
}

bool checkLowPriorityMove(int dir) {

    unsigned char joyBit = joyDirectBit[dir] << 4;
    if (usableSWCHA & joyBit) {
        return false;
    }

    // int offX = playerX + xdir[dir];
    // int offY = playerY + ydir[dir];


    int offset = dirOffset[dir];
    unsigned char *meOffset = me + offset;
    enum ObjectType destType = CharToType[GET(*meOffset)];

#if 1    // disable push
    if (/*faceDirectionDef[dir] && */ (!(theCave->flags & CAVEDEF_STAR_STATIC)) &&
        (Attribute[destType] & (ATT_MINE | ATT_PIPE))) {

        // FLASH(0x94, 4);

        // if (!pushCounter)
        //     if (destType == TYPE_ROCK_BONUS)
        //         startCharAnimation(TYPE_ROCK_BONUS, AnimateRockBonus + 2);


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
            //            startPlayerAnimation(ID_Locked);          // works
            //            nicely as start of push
        }

        if (pushCounter > 8) {


            static signed char xOffset[] = {0, CHAR_TRIX_X, 0, -CHAR_TRIX_X};
            static signed char yOffset[] = {-(CHAR_CENTER_Y >> 1), 0, CHAR_CENTER_Y >> 1, 0};

            if (Attribute[destType] & ATT_MINE) {

                addScore(VALUE_BREAK_GEODE);
                *meOffset = ATTRIBUTE_BIT(*meOffset, ATT_GEODOGE) ? FLAG(CH_CONVERT_GEODE_TO_DOGE) : CH_DUST_ROCK_0;

                // surroundingConglomerate(playerX + xdir[dir], playerY + ydir[dir]);

                // if (destType == TYPE_ROCK_BONUS) {

                //     ADDAUDIO(SFX_DOGE2);

                //     *meOffset = FLAG(CH_STAR);

                // }

                // else
                if (destType == TYPE_ROCK) {


                    ADDAUDIO(SFX_ROCK);

                    nDots(10, playerX, playerY, PT_ONE, 30,
                          xOffset[dir] + CHAR_CENTER_X /*+ rangeRandom(CHAR_TRIX_X) - (CHAR_TRIX_X >> 1)*/,
                          /*rangeRandom(CHAR_TRIX_Y) - (CHAR_TRIX_Y >> 1) + */ yOffset[dir] + CHAR_CENTER_Y, 40, 2);
                    nDots(10, playerX, playerY, PT_ONE, 15,
                          xOffset[dir] + CHAR_CENTER_X + rangeRandom(CHAR_TRIX_X) - (CHAR_TRIX_X >> 1),
                          rangeRandom(CHAR_TRIX_Y) - (CHAR_TRIX_Y >> 1) + yOffset[dir] + CHAR_CENTER_Y, 50, 3);
                } else {
                    ADDAUDIO(SFX_DOGE);

                    nDots(6, playerX, playerY, PT_TWO, 30, xOffset[dir] + CHAR_CENTER_X, yOffset[dir] + CHAR_CENTER_Y,
                          40, 7);
                }
            }

            else {
                *meOffset = FLAG(CH_CONVERT_PIPE);
            }

            // fixSurroundingConglomerates(meOffset);

            waitForNothing = 1;
            startPlayerAnimation(ID_Stand);

            pushCounter = 0;

            // extern int dogeBlockCount;
            // extern int cumulativeBlockCount;
            // dogeBlockCount++;
            // cumulativeBlockCount++;

            if (faceDirection > 0) {
                me += 2;
                boardCol += 2;    // SKIP processing it!
            }

            //            ADDAUDIO(SFX_PUSH);
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
        int idx = sphereDot(dripX, dripY, PT_BUBBLE, age, 1);
        if (idx >= 0) {
            particle[idx].speed = 10;    //(-0x2800 - rangeRandom(0x2800)) >> 4;
            // particle.speedX[idx] >>= 4;

            particle[idx].dir = 128 + rangeRandom(64) - 32;
        }
    }
}

void movePlayer(unsigned char *me) {

    handled = false;


    if (drop) {

        attachmentOffset = 0;
        *meAtt = attachment;
        drop = false;
        waitRelease = true;
        attachment = 0;
        return;
    }


    if (pulsePlayerColour) {
        nDots(2, playerX, playerY, PT_ONE, 25, CHAR_TRIX_X >> 1, CHAR_TRIX_Y >> 1, 100, 7);
        return;
    }


    // breath bubbles
    static int breath;
    if (showWater && playerY * CHAR_TRIX_Y > lavaSurfaceTrixel) {

        breath++;
        if (!(breath & 35) && (breath & 63) < 21) {
            int x = (playerX * 5) + 3;
            int y = (playerY * CHAR_TRIX_Y) + 4;
            bubbles(1, x - 1, y - 2, 400, 0x1000);
            // ADDAUDIO(SFX_BUBBLER);
        }
    }

    // else
    //     killAudio(SFX_BUBBLER);

    static unsigned char lastUsableSWCHA = 0;

    if (usableSWCHA != lastUsableSWCHA) {
        waitForNothing = 0;
        // tapDelay = 0;
    }

    // if (waitForNothing) {
    //     --waitForNothing;
    //     usableSWCHA = 0xFF;
    //     return;
    // }

    if (autoMoveFrameCount)
        return;


    lastUsableSWCHA = usableSWCHA;


    if (gearsWaitRelease && (usableSWCHA & 0xF0) == 0xF0)    // fixes gopher debug/exit 'lockup'
        gearsWaitRelease = false;


    for (int dir = 0; dir < 4; dir++)
        if (checkHighPriorityMove(dir)) {
            return;
        }

    for (int dir = 0; dir < 4 && !handled; dir++)
        if (checkLowPriorityMove(dir))
            return;

    // switch back to standing facing forward, turning if required

    if (!autoMoveFrameCount) {

        if (playerAnimationID == ID_WalkUp || playerAnimationID == ID_MineUp)
            startPlayerAnimation(ID_StandUp);

        else if (playerAnimationID == ID_Walk || playerAnimationID == ID_Mine)
            startPlayerAnimation(ID_StandLR);

        else if (playerAnimationID == ID_WalkDown || playerAnimationID == ID_MineDown)
            startPlayerAnimation(ID_Stand);
    }

    // after all movement checked, anything falling on player?
    // potential bug - if you're pushing and something falls on you

    if (Attribute[CharToType[*(me - _1ROW * gravity)]] & ATT_CRUSHES) {
        //        SAY(__WORD_WATCHOUT);
        startPlayerAnimation(ID_Die);
        return;
    }

    pushCounter = 0;
    idleTimer++;

    // if (!autoMoveFrameCount)
    //     chooseIdleAnimation();
}

// EOF