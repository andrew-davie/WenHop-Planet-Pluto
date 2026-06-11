#include "defines_dasm.h"

#include "cdfjplus.h"

#include "attribute.h"
#include "caveData.h"
#include "characterset.h"
#include "decodecaves.h"
#include "main.h"
#include "mellon.h"
#include "random.h"
#include "scroll.h"
#include "wyrm.h"


/* **************************************** */
/* Types */
// typedef const unsigned char UBYTE;
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
// void DrawHorizontalLine(objectType anObject, int x, int y, int aLength);

#if 0
void NextRandom(int *RandSeed1, int *RandSeed2);
int RandSeed1, RandSeed2;
#endif

const unsigned char *theCaveData;
// int caveFlags;
int decodingRow;
unsigned char caveMirrorXY;
static int doorX, doorY;
int processedLevel;
// extern int thumbnailSpeed;
int totalDogePossible;

enum DECODE_STATE decodeState;

struct CAVE_DEFINITION *theCave;

static int decodeFlasher;

int last_prng_a;
int last_prng_b;

void decodeCave(int cave) {

    theCave = (struct CAVE_DEFINITION *)caveList[cave];

    decodeState = DECODE_NONE;

    last_prng_a = theCave->randomInit[level];
    last_prng_b = last_prng_a++;

    doges = theCave->dogeRequired[level];

    time = (theCave->timeToComplete[level] << 8) + 60;
    millingTime = theCave->millingTime * 60;

    decodingRow = 0;
    decodeFlasher = 21;
    totalDogePossible = 0;

    theCaveData = (&(theCave->objectData)) + theCave->objectCount * 6;

    decodeState = DECODE_START;
}

unsigned char cmd;
unsigned char x, y, c, d, e, f;
unsigned char theCode;
unsigned char theObject;


int decodeExplicitData(int /*sfx*/) {

    prng_a = last_prng_a;
    prng_b = last_prng_b;

    switch (decodeState) {
    case DECODE_START:

        if (decodingRow < _BOARD_ROWS) {

            int bgchar = theCave->interiorCharacter;
            bgchar |= (bgchar << 24) | (bgchar << 16) | (bgchar << 8);

            myMemsetInt((unsigned int *)(RAM + _BOARD + decodingRow * _1ROW), bgchar, _BOARD_COLS / 4);

            for (int x = 0; x < _BOARD_COLS; x++)
                for (int object = 0; object < theCave->objectCount; object++) {
                    unsigned char *p = (&(theCave->objectData)) + object * 6;
                    if ((getRandom32() >> 24) < p[level + 1])
                        StoreObject(x, decodingRow, p[0]);
                }

            unsigned char border = theCave->borderCharacter;

            if (border) {

                if (!decodingRow || decodingRow == _BOARD_ROWS - 1)
                    myMemsetInt((unsigned int *)(RAM + _BOARD + decodingRow * _1ROW),
                                (border << 24 | border << 16 | border << 8 | border), _BOARD_COLS / 4);
                else {
                    StoreObject(0, decodingRow, border);
                    StoreObject(_BOARD_COLS - 1, decodingRow, border);
                }
            }

            decodingRow++;
        }

        else {


            d = e = 0;
            decodeState = DECODE_STOP;
            processedLevel = false;
        }

        break;

    case DECODE_STOP: {

        if (d == e) {

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
                cmd = 0;
            }

            else {
                cmd = theCode;
                theObject = *theCaveData++;
            }

            x = *theCaveData++;
            y = *theCaveData++;

            if (!cmd) {

                StoreObject(x, y, theObject);

                if (theObject == CH_DOORCLOSED || theObject == CH_DOOROPEN_0) {
                    doorX = x;
                    doorY = y;
                }

                else if (theObject == CH_MELLON_HUSK_BIRTH) {

                    playerX = x;
                    playerY = y;

                    resetTracking();
                }
            }

            else {

                d = 0;

                c = *theCaveData++;
                e = *theCaveData++;

                if (cmd == DRAW_FILLED_RECT) {
                    f = *theCaveData++;
                    if (e & 0x80) {    // instant
                        e &= 0x7F;
                        d = e - 1;
                    }
                }
            }
        }

        else {

            d++;

            switch (cmd) {

            case DRAW_LINE:
                DrawLine(theObject, x, y, d, c);
                break;

            case DRAW_FILLED_RECT:
                DrawFilledRect(theObject, x, y, c, d, f);
                break;

            case DRAW_RECT:
                d = e;
                DrawRect(theObject, x, y, c, d);
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

    last_prng_a = prng_a;
    last_prng_b = prng_b;

    return decodeFlasher;
}

void StoreObject(int x, int y, objectType anObject) {

    unsigned char *me = RAM + _BOARD + x + y * _1ROW;
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
        if (lavaSurfaceTrixel > 0 && line < lavaSurfaceTrixel)
            lavaSurfaceTrixel = line;

        break;
    }

    case TYPE_WATER: {

        showWater = true;
        int line = y * CHAR_TRIX_Y;
        if (lavaSurfaceTrixel > 0 && line < lavaSurfaceTrixel)
            lavaSurfaceTrixel = line;

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

    for (int counter1 = aHeight - 2; counter1 > 0; counter1--)
        DrawLine(aFillObject, x + 1, y + counter1, aWidth - 2, 2);
    DrawRect(anObject, x, y, aWidth, aHeight);
}

// EOF