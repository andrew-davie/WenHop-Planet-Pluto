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

// int last_prng_a;
// int last_prng_b;

void decodeCave(int cave) {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"

    theCave = (struct CAVE_DEFINITION *)(((int)caveList[cave]) & 0xFFFF);

#pragma GCC diagnostic pop

    decodeState = DECODE_NONE;

    cave_random_a = theCave->randomInit[level];
    cave_random_b = cave_random_a++;    // ensure one is non-zero!

    lockDisplay = theCave->flags & CAVEDEF_OVERVIEW;


    // displayMode = lockDisplay ? DISPLAY_HALF : DISPLAY_NORMAL;

    doges = theCave->dogeRequired[level];
    time = (theCave->timeToComplete[level] << 8) + 60;
    millingTime = theCave->millingTime * 60;

    myMemsetInt((unsigned int *)(RAM + _BOARD), 0, (_BOARD_COLS * _BOARD_ROWS) / 4);

    decodingRow = 0;
    decodeFlasher = 21;
    totalDogePossible = 0;

    theCaveData = (&(theCave->objectData)) + theCave->objectCount * 6;

    decodeState = DECODE_START;
}

unsigned char cmd;
unsigned char a, b, c, d, e, f;
unsigned char theCode;
unsigned char theObject;

// unsigned int restore_prng_a, restore_prng_b;

int decodeExplicitData(int /*sfx*/) {

    // restore_prng_a = prng_a;
    // restore_prng_b = prng_b;

    // prng_a = last_prng_a;
    // prng_b = last_prng_b;

    switch (decodeState) {
    case DECODE_START:

        // if (sfx)
        //     ADDAUDIO(SFX_SCORE);

        if (decodingRow < _BOARD_ROWS) {


            DrawLine(theCave->interiorCharacter /*| FLAG_UNCOVER*/, 0, decodingRow, _BOARD_COLS, 2);

            for (int x = 0; x < _BOARD_COLS; x++)
                for (int object = 0; object < theCave->objectCount; object++) {
                    unsigned char *p = (&(theCave->objectData)) + object * 6;

                    // unsigned char *me = RAM + _BOARD + x + decodingRow * _BOARD_COLS;
                    // if (GET(*me) == 0)
                    if ((getCaveRandom32() >> 24) < p[level + 1])
                        StoreObject(x, decodingRow, p[0]);
                }

            if (theCave->borderCharacter) {

                if (!decodingRow || decodingRow == _BOARD_ROWS - 1)
                    DrawLine(theCave->borderCharacter, 0, decodingRow, _BOARD_COLS, 2);

                else {
                    StoreObject(0, decodingRow, theCave->borderCharacter);
                    StoreObject(_BOARD_COLS - 1, decodingRow, theCave->borderCharacter);
                }
            }

            decodingRow++;
        }

        else {


            d = e = 0;
            // theCaveData = caveList[cave].cavePtr + sizeof(struct CAVE_DEFINITION);
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

            a = *theCaveData++;
            b = *theCaveData++;

            if (!cmd) {

                // if (theObject == CH_DOGE)
                //     theObject = CH_DOGE_PULSE_0 + rangeRandom(7);

                StoreObject(a, b, theObject);

                if (theObject == CH_DOORCLOSED) {
                    doorX = a;
                    doorY = b;
                }

                else if (theObject == CH_MELLON_HUSK_BIRTH) {

                    playerX = a;
                    playerY = b;

                    // scrollX = playerX * CHAR_TRIX_X - HALFWAY_X;

                    // if (scrollX < 0)
                    //     scrollX = 0;

                    // if (scrollX >= BOARD_TRIX_X - SCREEN_TRIX_X)
                    //     scrollX = BOARD_TRIX_X - SCREEN_TRIX_X;

                    // scrollX <<= 16;


                    // scrollY = playerY * CHAR_TRIX_Y - HALFWAY_Y;

                    // if (scrollY < 0)
                    //     scrollY = 0;

                    // if (scrollY >= BOARD_TRIX_Y - SCREEN_TRIX_Y)
                    //     scrollY = BOARD_TRIX_Y - SCREEN_TRIX_Y;

                    // scrollY <<= 16;

                    resetTracking();
                }

                //                thumbnailSpeed = -1;
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
                DrawLine(theObject, a, b, d, c);
                // if (sfx)
                //     ADDAUDIO(SFX_DRIP);
                break;

            case DRAW_FILLED_RECT:
                // thumbnailSpeed = -2;
                DrawFilledRect(theObject, a, b, c, d, f);
                // if (sfx)
                //     ADDAUDIO(SFX_DIRT);
                break;

            case DRAW_RECT:
                d = e;
                DrawRect(theObject, a, b, c, d);
                // if (sfx)
                //     ADDAUDIO(SFX_BLIP);
                break;

            default:
                break;
            }

            // if (d == e)
            //     thumbnailSpeed = -10;
        }

        break;
    }

    case DECODE_FLASH:

        if (theCave->borderCharacter)
            DrawRect(theCave->borderCharacter /*& ((decodeFlasher & 4) ? 0 : 0xFF)*/, 0, 0, _BOARD_COLS, _BOARD_ROWS);

        // tmp        if (!(decodeFlasher & 0b11))
        // if (sfx)
        //     ADDAUDIO(SFX_DRIP);
        if (!--decodeFlasher)
            StoreObject(doorX, doorY, CH_DOORCLOSED);
        break;

    default:
        break;
    }

    // last_prng_a = prng_a;
    // last_prng_b = prng_b;

    // prng_a = restore_prng_a;
    // prng_b = restore_prng_b;

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