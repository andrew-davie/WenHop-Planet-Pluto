#include "defines_dasm.h"

#include "animations.h"
#include "attribute.h"
#include "board.h"
#include "caveData.h"
#include "colour.h"
#include "decodeCaves.h"
#include "gameState.h"
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
static unsigned int pushCounter;
enum FaceDirectionX faceDirection;
bool playerDead;
static int waitForNothing;
bool handled;
static bool gearsActive;
static bool gearsWaitRelease;
static int kdelay = 0;

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


const OFFSET sampleOffsetRight[] = {

    {-5, -2 * 3}, {-5, -2 * 3}, {-5, -2 * 3}, {-5, -2 * 3}, {-5, -2 * 3},


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

bool checkHighPriorityMove(BoardCursor *cur, int dir) {

    static int zapDelay = 0;
    if (zapDelay)
        --zapDelay;


    unsigned char joyBit = joyDirectBit[dir] << 4;
    if (usableSWCHA & joyBit)
        return false;

    if (!kdelay && !waitRelease) {
        if (!(inpt4 & 0x80)) {

            meAtt = cur->me + dirOffset[dir];

            if (attachment) {

                int type2 = CharToType[GET(*meAtt)];
                if (Attribute[type2] & ATT_BLANK) {

                    if (type2 == TYPE_GEODOGE)
                        attachment = CH_GEODOGE_FALLING;

                    else if (type2 == TYPE_ROCK)
                        attachment = CH_ROCK_FALLING;

                    if (dir == 1 || dir == 2)
                        attachment |= FLAG_THISFRAME;

                    drop = true;
                    attachmentOffset = dropOffset[dir];
                    return true;
                }
            }

            else if (!attachment) {

                unsigned char pickup = PickupCharacter[CharToType[GET(*meAtt)]];
                if (pickup) {

                    kdelay = 5;

                    faceDirection = faceDirectionDef[dir];

                    attachment = pickup;

                    extern const unsigned char AnimateBomb[];
                    if (pickup == CH_BOMB)
                        startCharAnimation(TYPE_BOMB, AnimateBomb);

                    if (dir == 1 || dir == 2)
                        attachment |= FLAG_THISFRAME;

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

            ADDAUDIO(SFX_ZAP2);
            ADDAUDIO(SFX_WHOOSH);
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


        else if (Attribute[destType] & (ATT_BLANK | ATT_PERMEABLE | ATT_GRAB | ATT_EXIT)) {

            pushCounter = 0;

            if (Attribute[destType] & ATT_BLANK)
                ADDAUDIO(SFX_SPACE);

            else if (destType == TYPE_OUTBOX) {

                *meOffset = CH_EXITBLANK;
                ADDAUDIO(SFX_WHOOSH);
                exitMode = 40;
                waitRelease = true;
            }

            else if (destType == TYPE_FLIP_GRAVITY) {
                nextGravity = -gravity;
                FLASH(0xC5, 3);
#if ENABLE_SHAKE
                setShake(20);
#endif
            }

            if (Attribute[destType] & ATT_GRAB) {
                grabDoge();
                nDots(10, playerX + xdir[dir], playerY + ydir[dir], PT_SPIRAL2, 40, 3, 4, 50, 7);
            }


            playerX += xdir[dir];
            playerY += ydir[dir];

            frameAdjustX = frameAdjustY = 0;

            if (!exitMode) {

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
                *meOffset = ATTRIBUTE_BIT(*meOffset, ATT_GEODOGE) ? FLAG(CH_CONVERT_GEODE_TO_DOGE) : CH_DUST_ROCK_0;

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
        int idx = sphereDot(dripX, dripY, PT_BUBBLE, age, 1);
        if (idx >= 0) {
            particle[idx].speed = 10;
            particle[idx].dir = 128 + rangeRandom(64) - 32;
        }
    }
}


void movePlayer(BoardCursor *cur) {

    if (kdelay)
        --kdelay;
    handled = false;


    if (drop) {


        if (GET(attachment) == CH_BOMB) {
            extern const unsigned char AnimateBomb[];
            startCharAnimation(TYPE_BOMB, AnimateBomb + 2);
        }

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
        }
    }

    static unsigned char lastUsableSWCHA = 0;

    if (usableSWCHA != lastUsableSWCHA) {
        waitForNothing = 0;
    }

    if (autoMoveFrameCount)
        return;


    lastUsableSWCHA = usableSWCHA;


    if (gearsWaitRelease && (usableSWCHA & 0xF0) == 0xF0)    // fixes gopher debug/exit 'lockup'
        gearsWaitRelease = false;


    for (int dir = 0; dir < 4; dir++)
        if (checkHighPriorityMove(cur, dir))
            return;

    for (int dir = 0; dir < 4 && !handled; dir++)
        if (checkLowPriorityMove(cur, dir))
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

    if (Attribute[CharToType[*(cur->me - _BOARD_COLS * gravity)]] & ATT_CRUSHES) {
        startPlayerAnimation(ID_Die);
        return;
    }

    pushCounter = 0;
    idleTimer++;
}

// EOF