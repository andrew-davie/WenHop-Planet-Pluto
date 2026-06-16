#ifndef __MAIN_H
#define __MAIN_H

#include "defines_from_dasm_for_c.h"
#include <stdbool.h>

#define ENABLE_OVERLAY 0
#define ENABLE_SOUND 1
#define ENABLE_TOGGLE_DISPLAY_ON_DEATH 0
#define ENABLE_SHAKE 1
#define ENABLE_IDLE_ANIMATION 1
#define ENABLE_SNOW 1             /* 148 bytes */
#define ENABLE_TITLE_PULSE 0      /* 80 bytes */
#define ENABLE_EASTER_MYNAME 0    /* 72 bytes */
#define ENABLE_RAINBOW 1          /* 212 bytes */
#define ENABLE_60MHZ_AUTODETECT 0 /* 16 bytes */
#define ENABLE_ANIMATING_MAN 1    /* 244 bytes but man disappears */
#define ENABLE_RAGE_STATIC 1

#define ENABLE_DEBUG 1

#define CHAR_HEIGHT 21
#define CHAR_HEIGHT_HALF 15
#define CHAR_HEIGHT_OVERVIEW 9

#define SPRITE_DEPTH 23

#define MAXIMUM_AMOEBA_SIZE 200

#define RAIN_ACCEL 0x800
#define RAIN_FORMING_DRIP -0xA000
#define RAIN_IMPACT_DURATION 0x4000
#define RAIN_RESET_AFTER_IMPACT (RAIN_FORMING_DRIP - RAIN_IMPACT_DURATION - RAIN_ACCEL)
#define RAIN_DEAD (RAIN_RESET_AFTER_IMPACT + RAIN_IMPACT_DURATION)

// Note: MODIFY reciprocal[] entry in player.c if you change SPEED_BASE
#define SPEED_BASE 7

#define PIXELS_PER_CHAR 4
#define HALFWAYX 20
#define HALFWAYY 32
#define TRILINES (CHAR_HEIGHT / 3)

#define SCORE_SCANLINES 21
#define SCANLINES (_ARENA_SCANLINES - SCORE_SCANLINES)

#define _1ROW 40

#define ICON_BASE 100

void setJumpVectors(int midKernel, int exitKernel);
void InitializeNewGame();
void updateAnimation();


enum DisplayMode { DISPLAY_NORMAL, DISPLAY_HALF, DISPLAY_OVERVIEW, DISPLAY_NONE };

extern enum DisplayMode displayMode, lastDisplayMode;

extern const unsigned char BitRev[];

struct Animation {
    signed char index;
    signed char count;
};

#define setPointer(fetcher, offset) QPTR[fetcher] = (offset) << 20;

#define FLAG_THISFRAME 0x80
#define FLAG_UNCOVER 0x40

#define DEAD_RESTART_COUCH 200
#define EXTERNAL(i) ((const unsigned char *)(long)(i))

// extern const unsigned char iCC_skull[];
extern const unsigned char joyDirectBit[4];
extern unsigned char mm_tv_type;

extern int level;
extern int cave;

enum SCHEDULE {
    SCHEDULE_START,
    SCHEDULE_PROCESSBOARD,
    SCHEDULE_UNPACK_CAVE,
};

extern enum SCHEDULE gameSchedule;
extern int gameSpeed;
extern int gameFrame;

enum KERNEL_TYPE {
    KERNEL_COPYRIGHT,
    KERNEL_MENU,
    KERNEL_GAME,
    KERNEL_STATS,
    KERNEL_SKULL,
};

extern unsigned char inpt4;
extern unsigned char swcha;
extern unsigned char bufferedSWCHA;
extern unsigned int usableSWCHA;
extern unsigned int inhibitSWCHA;

extern int diamonds;
extern int lives;
extern int time;
extern bool waitRelease;
extern bool rageQuit;
extern unsigned int sparkleTimer;

extern int exitMode;
extern unsigned int idleTimer;
extern int millingTime;
// extern bool exitTrigger;

extern int selectResetDelay;
extern unsigned char *thisx;

extern int boardCol;

#if ENABLE_RAINBOW
extern bool rainbow;
#endif


extern unsigned int triggerPressCounter;
extern unsigned char enableParallax;
extern bool enableICC;

extern bool caveCompleted;

#define RAINHAILSHINE 12

extern unsigned char rainX[RAINHAILSHINE];
extern int rainY[RAINHAILSHINE], rainSpeed[RAINHAILSHINE];
extern char rainRow[RAINHAILSHINE];
extern int weather;
// extern int canPlay[5];

enum FaceDirection {
    FACE_LEFT = -1,
    FACE_RIGHT = 1,
    FACE_UP = -1,
    FACE_DOWN = 1,
};

extern unsigned int currentPalette;
extern int uncoverTimer;
extern unsigned int uncoverCount;
extern unsigned int availableIdleTime;
extern const unsigned char sinoid[16];

void Scheduler();
void processBoardSquares();
void reanimateDiamond(unsigned char *thisx);
bool handleSelectReset();
void phaseshiftDiamond(unsigned char *cell);
void initNewGame();
void checkButtonRelease();
void requestKernel(int kernel);

// extern int actualScore;

/*
#define START_TIMER \
    T1TC = 0; \
    T1TCR = 1;                  // ensure timer starts @ 0

#define END_TIMER { \
    T1TCR = 0; \
    int time = T1TC; \
    if (time > max_timer) \
        max_timer = time; \
    setScore(max_timer); \
    }

#define PROFILE(a) \
    static int max_timer = 0; \
    START_TIMER \
    a \
    END_TIMER
*/

#define GET(a) (((unsigned char)((a) << 2)) >> 2)
#define GET2(a) (((unsigned char)((a) << 1)) >> 1)

#define NEW_LINE 0xFF
#define END_STRING NEW_LINE, NEW_LINE

#endif
