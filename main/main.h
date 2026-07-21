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

extern unsigned int availableIdleTime;

extern int pulsePlayerColour;

#if ENABLE_SHAKE
extern int shakeX, shakeY, shakeTime;
#endif


#define DEBUG_TIMES
#ifdef DEBUG_TIMES

// Sized CH_MAX (attribute.h) and indexed by raw character number -- see
// processBoardSquares() in board.c, which tracks the worst-case T1TC ticks
// spent per character number, one array slot per chName value.
// unsigned short: ticks for a single board cell never approach 64K, and
// this way the full usable range is available instead of losing half of it
// to a sign bit.
//
// Size given explicitly (133, i.e. CH_MAX -- hardcoded rather than pulling
// in attribute.h just for the one constant; checked against CH_MAX by a
// _Static_assert next to the real definition in main.c). Root cause of
// debug going missing from the debugger's globals view was tracked down to
// the Gopher2600 DWARF parser (coprocessor/developer/dwarf/dwarf_builder.go
// in gopher2600, not this codebase): a variable declared via an unsized
// `extern T foo[];` gets an incomplete array type at the declaration site,
// and the parser's abstract-origin/specification resolution never falls
// back to the complete type carried by the definition, so the variable's
// type resolves to nil and it's silently dropped. Not a volatile/attribute
// issue, not an LTO dead-store elimination issue, not a Go map ordering
// issue -- those were all considered and ruled out first. Fixed on the
// gopher2600 side but broke something else there and got reverted, so
// working around it here instead: giving the declaration an explicit size
// means it never has an incomplete type to begin with.
extern unsigned short debug[133];

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