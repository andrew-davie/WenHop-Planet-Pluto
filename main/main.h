#pragma once

#include <stdbool.h>

#include "gameState.h"


#define _WENHOP_SK_ID _SK_GAME_ID
#define GROUPING 10

#define NEW_LINE 0xFF
#define END_STRING NEW_LINE, NEW_LINE

#define ENABLE_SHAKE 1
#define ENABLE_SWIPE 1

//------------------------------------------------------------------------------


#define CHAR_TRIX_X 5
#define CHAR_TRIX_Y 10

#define CHAR_CENTER_X (CHAR_TRIX_X >> 1)
#define CHAR_CENTER_Y (CHAR_TRIX_Y >> 1)

#define CHAR_X (4 * CHAR_TRIX_X)
#define CHAR_Y (3 * CHAR_TRIX_Y)

#define BOARD_TRIX_X (_BOARD_COLS * CHAR_TRIX_X)
#define BOARD_TRIX_Y (_BOARD_ROWS * CHAR_TRIX_Y)

#define SCREEN_TRIX_X 40
#define SCREEN_TRIX_Y (_SCANLINES / 3)

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------

#define GET(a) (((unsigned char)((a) << 1)) >> 1)


#define TYPEOF(x) (CharToType[GET(x)])
#define ATTRIBUTE(x) (Attribute[TYPEOF(x)])
#define ATTRIBUTE_BIT(x, y) (ATTRIBUTE(x) & (y))

#define SPEED_BASE 5
#define MOVE_SPEED 15

#define FLAG_THISFRAME 0x80

#define FLAG(a) ((a) | FLAG_THISFRAME)

#define DIR_U 1
#define DIR_R 2
#define DIR_D 4
#define DIR_L 8
extern int armFrequency;

extern unsigned int frame;
extern int tvSystem;
extern int armFrequency;
extern int armCycles;

extern unsigned int rand;
extern unsigned char colubk;
extern int level;
extern int millingTime;    // negative = expired
extern int doges;
extern int time;
extern int lavaSurfaceTrixel;
extern bool showWater;
extern bool showLava;

extern int cave;
extern unsigned char bufferedSWCHA;
extern unsigned int usableSWCHA;
extern unsigned int inhibitSWCHA;

extern unsigned char inpt4;
extern unsigned char swcha;
extern bool showTool;

extern int gameFrame, gameSpeed;

extern const signed char dirOffset[];
extern const signed char xdir[];
extern const signed char ydir[];
extern int exitMode;
extern bool waitRelease;

extern int gravity;
extern int nextGravity;

extern int boardRow;
extern int boardCol;
extern unsigned int idleTimer;
extern int lives;
extern unsigned int sparkleTimer;
extern bool playerDead;
extern int gameFrame;

extern const int xInc[];
extern const int yInc[];
extern unsigned int availableIdleTime;

extern int pulsePlayerColour;

#if ENABLE_SHAKE
extern int shakeX, shakeY, shakeTime;
#endif

void ClearChannel(void *ptr);
void MemCopy32(void *ptr1, void *ptr2, unsigned int count);
void Random(unsigned int count);

void setJumpVectors(unsigned int buffer, short int loopAddress, short int endAddress, int length);

void setGameState(enum GAME_STATE state);

int dirFromCoords(int x, int y, int prevX, int prevY);
void initNewGame();

struct dataStreams {
    unsigned char dataStream;
    unsigned short buffer;
};


void initDataStreams(const struct dataStreams *streams, int streamCount);

void setShake(int time);


// EOF