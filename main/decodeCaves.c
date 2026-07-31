#include "animations.h"
#include "defines_dasm.h"

#include "cdfjplus.h"

#include "animations.h"
#include "attribute.h"
#include "caveData.h"
#include "decodeCaves.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "random.h"
#include "scroll.h"
#include "swipe.h"
#include "wyrm.h"

/* **************************************** */
/* Types */
typedef unsigned char objectType;

/* DrawLine data */
/* When drawing lines, you can draw in all eight directions. This table gives
   the offsets needed to move in each of the 8 directions. */

const signed char ldxy[] = {-1, -1, 0, 1, 1, 1, 0, -1, -1, -1};

/* **************************************** */
/* Prototypes */
void decodeCave(int cave);
void StoreObject(int x, int y, objectType anObject);
void DrawLine(objectType anObject, int x, int y, int aLength, int aDirection);
void DrawFilledRect(objectType anObject, int x, int y, int aWidth, int aHeight, objectType aFillObject);
void DrawRect(objectType anObject, int x, int y, int aWidth, int aHeight);

static const unsigned char *theCaveData;
static int decodingRow;
static int doorX, doorY;
static int processedLevel;
int totalDogePossible;

static enum DECODE_STATE decodeState;

struct CAVE_DEFINITION *theCave;

static int decodeFlasher;


static struct {
    unsigned char cmd;
    unsigned char col, row, c, d, e, f;
} decode;

static unsigned char theCode;
static unsigned char theObject;


// Do NOT use the regular random generator -- otherwise the potentially deterministic
// init in this code will stop on "true" randomness elsewhere in the code.

static unsigned int decode_prng_a;
static unsigned int decode_prng_b;

static unsigned int decodeGetRandom32() {
    decode_prng_b = 36969 * (decode_prng_b & 65535) + (decode_prng_b >> 16);
    decode_prng_a = 18000 * (decode_prng_a & 65535) + (decode_prng_a >> 16);
    return decode_prng_b;
}


static unsigned int decodeRangeRandom(int range) {
    unsigned int v = decodeGetRandom32();
    v ^= v << 16;
    return ((v >> 16) * range) >> 16;
}


void decodeCave(int newCave) {

    theCave = (struct CAVE_DEFINITION *)caveList[newCave].cave;

    decodeState = DECODE_NONE;

    decode_prng_a = theCave->randomInit[level] ? theCave->randomInit[level] : prng_a;
    decode_prng_b = decode_prng_a++;

    doges = theCave->dogeRequired[level];

    time = (theCave->timeToComplete[level] << 8) + 60;
    millingTime = theCave->millingTime << 16;

    initWeather();    // resets/seeds weatherIntensity from theCave->weather -- see particle.c

    if (theCave->water) {    // as if a CH_WATER tile had been placed at this line
        showWater = true;
        liquidTrixel_8 = theCave->water << 8;    // low 8 bits are fractional
    }

    decodingRow = 0;
    decodeFlasher = 1;    // 21;
    totalDogePossible = 0;

    theCaveData = (&(theCave->objectData)) + theCave->objectCount * 6;

    decodeState = DECODE_START;


    if (theCave->flags & CAVEDEF_STAR_STATIC)
        startCharAnimation(TYPE_STAR, AnimateStar + 7);

    if (theCave->flags & CAVEDEF_START_WITH_WEAPON)
        weapon = theCave->weapon[level];
}


int decodeExplicitData() {

    switch (decodeState) {
    case DECODE_START:

        if (decodingRow < _BOARD_ROWS) {

            int bgchar = theCave->interiorCharacter;
            bgchar |= (bgchar << 24) | (bgchar << 16) | (bgchar << 8);

            myMemsetInt((unsigned int *)(RAM + _BOARD + decodingRow * _BOARD_COLS), bgchar, _BOARD_COLS / 4);

            for (int x = 0; x < _BOARD_COLS; x++)
                for (int object = 0; object < theCave->objectCount; object++) {
                    unsigned char *p = (&(theCave->objectData)) + object * 6;
                    if ((decodeGetRandom32() >> 24) < p[level + 1])
                        StoreObject(x, decodingRow, p[0]);
                }

            unsigned char border = theCave->borderCharacter;

            if (border) {

                if (!decodingRow || decodingRow == _BOARD_ROWS - 1)
                    myMemsetInt((unsigned int *)(RAM + _BOARD + decodingRow * _BOARD_COLS),
                                (border << 24 | border << 16 | border << 8 | border), _BOARD_COLS / 4);
                else {
                    StoreObject(0, decodingRow, border);
                    StoreObject(_BOARD_COLS - 1, decodingRow, border);
                }
            }

            decodingRow++;
        }

        else {


            decode.d = decode.e = 0;
            decodeState = DECODE_STOP;
            processedLevel = false;
        }

        break;

    case DECODE_STOP: {

        if (decode.d == decode.e) {

            theCode = *theCaveData++;

            if (theCode == DRAW_EOF) {

                // Now handle the individual level additions
                // format <block> 0xFF... <block> 0xFF
                // so we want to get to the right block based on level

                if (!processedLevel) {
                    for (int skipToBlock = 0; skipToBlock < level; skipToBlock++)
                        while (*theCaveData++ != 0xFF)
                            ;
                    processedLevel = true;
                    break;
                }

                for (int skipBotBlock = level; skipBotBlock < 4; skipBotBlock++)
                    while (*theCaveData++ != DRAW_EOF)
                        ;

                decodeState = DECODE_FLASH;
                break;
            }

            if (theCode <= DRAW_OBJ) {
                theObject = theCode;
                decode.cmd = 0;
            }

            else {
                decode.cmd = theCode;
                theObject = *theCaveData++;
            }

            decode.col = *theCaveData++;
            decode.row = *theCaveData++;

            if (!decode.cmd) {

                StoreObject(decode.col, decode.row, theObject);

                if (theObject == CH_DOORCLOSED || theObject == CH_DOOROPEN_0) {
                    doorX = decode.col;
                    doorY = decode.row;
                }

                else if (theObject == CH_MELLON_HUSK_BIRTH) {

                    playerX = decode.col;
                    playerY = decode.row;

                    // Snap the camera onto the player as soon as we know
                    // where they are -- but do NOT start the star swipe here.
                    // decodeExplicitData() is called repeatedly by
                    // scheduleUnpackCave() (schedule.c), possibly across many
                    // frames, and this object may be decoded partway through
                    // that process, with more of the cave still to go. If we
                    // started the iris-in right here, it would begin growing
                    // mid-decode, competing with decode for CPU time and
                    // animating toward a screen that isn't finished being
                    // built yet. initGameState_Game() already left the swipe
                    // idle and the screen fully hidden (see initStarSwipe()),
                    // which holds correctly with no further action needed
                    // until decode is actually done. scheduleUnpackCave()
                    // starts the real iris-in itself, once its loop confirms
                    // there's nothing left to decode -- see the comment there
                    // for the centring math (moved there unchanged).
                    resetTracking();
                }
            }

            else {

                decode.d = 0;

                decode.c = *theCaveData++;
                decode.e = *theCaveData++;

                if (decode.cmd == DRAW_FILLED_RECT) {
                    decode.f = *theCaveData++;
                    if (decode.e & 0x80) {    // instant
                        decode.e &= 0x7F;
                        decode.d = decode.e - 1;
                    }
                }
            }
        }

        else {

            decode.d++;

            switch (decode.cmd) {

            case DRAW_LINE:
                DrawLine(theObject, decode.col, decode.row, decode.d, decode.c);
                break;

            case DRAW_FILLED_RECT:
                DrawFilledRect(theObject, decode.col, decode.row, decode.c, decode.d, decode.f);
                break;

            case DRAW_RECT:
                decode.d = decode.e;
                DrawRect(theObject, decode.col, decode.row, decode.c, decode.d);
                break;

            default:
                break;
            }
        }

        break;
    }

    case DECODE_FLASH:

        if (theCave->borderCharacter)
            DrawRect(theCave->borderCharacter /*& ((decodeFlasher & 4) ? 0 : 0xFF)*/, 0, 0, _BOARD_COLS, _BOARD_ROWS);

        if (!--decodeFlasher)
            StoreObject(doorX, doorY, CH_DOORCLOSED);
        break;

    default:
        break;
    }

    return decodeFlasher;
}

void StoreObject(int x, int y, objectType anObject) {

    // guard rails (debugging) ...
    if (x < 0 || x >= _BOARD_COLS       //
        || y < 0 || y >= _BOARD_ROWS    //
        || anObject >= CH_MAX)
        return;    // MAJOR bug but recover (?)

    unsigned char *me = RAM + _BOARD + x + y * _BOARD_COLS;
    unsigned char type = TYPEOF(anObject);

    if (TYPEOF(*me) == TYPE_DOGE)
        totalDogePossible--;

    switch (type) {

    case TYPE_WYRM: {

        if (!newWyrm(x, y))
            anObject = CH_BLANK;
        break;
    }

    case TYPE_LAVA: {

        showLava = true;
        int line = y * CHAR_TRIX_Y;
        if ((liquidTrixel_8 >> 8) > 0 && line < (liquidTrixel_8 >> 8))
            liquidTrixel_8 = line << 8;

        break;
    }

    case TYPE_WATER: {

        showWater = true;
        int line = (y + 1) * CHAR_TRIX_Y;
        if ((liquidTrixel_8 >> 8) > 0 && line < (liquidTrixel_8 >> 8))
            liquidTrixel_8 = line << 8;

        break;
    }

    case TYPE_DOGE: {
        totalDogePossible++;
        break;
    }

    default:
        break;
    }

    // ensure parity on gears
    // wtf? on non-functional if using shorter way
    if (anObject == CH_GRINDER_0) {
        if ((x + y) & 1)
            anObject = CH_GRINDER_0;
        else
            anObject = CH_GRINDER_1;
    }

    *me = anObject;
}

void DrawLine(objectType anObject, int x, int y, int aLength, int aDirection) {

    if (y < _BOARD_ROWS)
        for (int counter = 0; counter < aLength; counter++) {
            StoreObject(x, y, anObject);
            x += ldxy[aDirection + 2];
            y += ldxy[aDirection];
        }
}

void DrawRect(objectType anObject, int x, int y, int aWidth, int aHeight) {

    DrawLine(anObject, x, y, aWidth, 2);
    DrawLine(anObject, x, y + aHeight - 1, aWidth, 2);
    DrawLine(anObject, x, y, aHeight, 4);
    DrawLine(anObject, x + aWidth - 1, y, aHeight, 4);
}

void DrawFilledRect(objectType anObject, int x, int y, int aWidth, int aHeight, objectType aFillObject) {

    if (anObject & 0x80) {

        // 1st byte is flag + object type
        // last is the chance of object being filled

        for (int i = 0; i < aWidth; i++) {
            for (int j = 0; j < aHeight; j++) {

                if (!decodeRangeRandom(aFillObject))
                    StoreObject(x + i, y + j, anObject & 0x7F);
            }
        }
    } else {
        for (int counter1 = aHeight - 2; counter1 > 0; counter1--)
            DrawLine(aFillObject, x + 1, y + counter1, aWidth - 2, 2);
        DrawRect(anObject, x, y, aWidth, aHeight);
    }
}

// EOF