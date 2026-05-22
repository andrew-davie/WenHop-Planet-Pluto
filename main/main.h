#pragma once

#include <stdbool.h>

#include "attribute.h"
#include "board.h"
#include "characterset.h"
#include "colour.h"
#include "decodeCaves.h"
#include "gameState.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "player.h"
#include "random.h"
#include "reverseBits.h"
#include "schedule.h"
#include "score.h"
#include "scroll.h"
#include "sound.h"

#define _WENHOP_SK_ID _SK_GAME_ID

#define NEW_LINE 0xFF
#define END_STRING NEW_LINE, NEW_LINE

#define HALFWAYX 20
#define HALFWAYY 32

#define TRILINES (PIECE_DEPTH / 3)

#define GET(a) (((unsigned char)((a) << 1)) >> 1)


#define TYPEOF(x) (CharToType[GET(x)])
#define ATTRIBUTE(x) (Attribute[TYPEOF(x)])
#define ATTRIBUTE_BIT(x, y) (ATTRIBUTE(x) & (y))

#define SPEED_BASE 8

#define FLAG_THISFRAME 0x80

#define FLAG(a) ((a) | FLAG_THISFRAME)

#define DIR_U 1
#define DIR_R 2
#define DIR_D 4
#define DIR_L 8


extern unsigned int frame;
extern int tvSystem;
extern unsigned int rand;
extern unsigned char colubk;
extern int level;
extern bool lockDisplay;
extern int millingTime;    // negative = expired
extern int doges;
extern int time;
extern int lavaSurfaceTrixel;
extern bool showWater;
extern bool showLava;
extern bool exitTrigger;

extern unsigned int cave;
extern bool caveCompleted;
extern unsigned char bufferedSWCHA;
extern unsigned int usableSWCHA;
extern unsigned int inhibitSWCHA;

extern unsigned char inpt4;
extern unsigned char swcha;
extern bool showTool;
extern int gameSpeed;

extern unsigned char *me;

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

extern const unsigned char joyDirectBit[4];
extern const signed char xInc[];
extern const signed char yInc[];
extern unsigned int availableIdleTime;

extern int pulsePlayerColour;


void ClearChannel(void *ptr);
void MemCopy32(void *ptr1, void *ptr2, unsigned int count);
void Random(unsigned int count);

void setJumpVectors(unsigned int buffer, short int loopAddress, short int endAddress, int length);

// unsigned int rangeRandom(short int range);

void setGameState(enum GAME_STATE state);

int sphereDot(int dotX, int dotY, int type, unsigned char age);
void nDots(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed);
void surroundingConglomerate(int col, int row);

int dirFromCoords(int x, int y, int prevX, int prevY);
void setupBoardScanner();
void initNewGame();


// EOF