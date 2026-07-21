
#include <stdbool.h>

#include "defines_dasm.h"

#include "cdfjplus.h"

#include "main.h"

#include "attribute.h"
#include "colour.h"
#include "gameState.h"
#include "kernels.h"
#include "main.h"
#include "random.h"
#include "savekey.h"
#include "score.h"
#include "sound.h"
#include "swipe.h"

#ifdef DEBUG_TIMES


// Per-character worst-case T1TC timing, one slot per chName value (see
// processBoardSquares() in board.c). 0 = never timed yet.
//
// No volatile, no attributes -- neither was the actual cause of debug
// vanishing from the globals view (see main.h for the real cause, in the
// Gopher2600 DWARF parser). Explicit initialiser kept -- makes this a
// definite definition rather than a tentative one, cheap to keep either
// way.
unsigned short debug[CH_MAX] = {[0 ... CH_MAX - 1] = 0};

// keeps the hardcoded size in the main.h extern declaration honest if
// CH_MAX ever changes
_Static_assert(
    CH_MAX == 133,
    "debug[] extern declaration in main.h is hardcoded to size 133 (CH_MAX) -- update both if CH_MAX changes");

#endif


#if ENABLE_SHAKE

int shakeX;
int shakeY;


int shakeTime;


void setShake(int time) {
    shakeTime = time;
}


#endif


#define INTIM_TO_ARM_CYCLES 3749 /* INTIM * 64 / 76 * 4452 */

int tvSystem;
int armFrequency;    // 60 or 70
int armCycles;

int level;
int millingTime;    // negative = expired
int doges;
int time;
int lavaSurfaceTrixel;
bool showWater;
bool showLava;

int cave;
unsigned char bufferedSWCHA;
unsigned int usableSWCHA;
unsigned int inhibitSWCHA;

int exitMode;
bool waitRelease;
bool showTool;

int gravity;
int nextGravity;

unsigned char inpt4;
unsigned char swcha;

int gameSpeed;
int gameFrame;

int boardRow;
int boardCol;
int lives;
unsigned int sparkleTimer;

unsigned int idleTimer;
unsigned int availableIdleTime;
int pulsePlayerColour;


const signed char dirOffset[] = {-_BOARD_COLS, 1, _BOARD_COLS, -1, 0};
const signed char xdir[] = {0, 1, 0, -1, 0};
const signed char ydir[] = {-1, 0, 1, 0, 0};


enum GAME_STATE gameState, nextGameState;

unsigned char kernel;    // use _KERNEL_* values from 6502
unsigned char colubk;

void Null();    // empty function, for any use

void runARM_SystemReset();
void runARM_Load_SaveKey();
void runARM_VerticalBlank();
void runARM_Overscan();

void HandleControls();
void SilenceTIA();

void initNextLife();


/******************************* Variables *******************************/
// stay ARM-side

unsigned int rand = 10531789;    // 32 bit LFSR random number
unsigned int frame;


unsigned char soundMode = _SND_MODE_TIA;    // sound mode used/passed Atari-side

// Atari input direct access variables - joysticks and RESET/SELECT are
// automatically wait/repeat handled
bool input_flag[15] = {false};

#define p1_u input_flag[0]
#define p1_d input_flag[1]
#define p1_l input_flag[2]
#define p1_r input_flag[3]
#define p0_u input_flag[4]
#define p0_d input_flag[5]
#define p0_l input_flag[6]
#define p0_r input_flag[7]
#define p0_b input_flag[8]
#define p1_b input_flag[9]
#define RESET_swch input_flag[10]
#define SELECT_swch input_flag[11]
#define P0_diff input_flag[12]
#define P1_diff input_flag[13]
#define CBW_swch input_flag[14]

// for each control user can assign a number of frames in between
// the first press acknowledge and the second, default 14
// use 0 to bypass the wait timer
const unsigned char input_wait[12] = {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14};
#define p1_u_wait input_wait[0]
#define p1_d_wait input_wait[1]
#define p1_l_wait input_wait[2]
#define p1_r_wait input_wait[3]
#define p0_u_wait input_wait[4]
#define p0_d_wait input_wait[5]
#define p0_l_wait input_wait[6]
#define p0_r_wait input_wait[7]
#define p0_b_wait input_wait[8]
#define p1_b_wait input_wait[9]
#define RESET_swch_wait input_wait[10]
#define SELECT_swch_wait input_wait[11]

// for each control user can assign a number of frames in between
// the second acknowledge and any after, default 7
// use 0 to bypass the repeat timer
const unsigned char input_repeat[12] = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
#define p1_u_repeat input_repeat[0]
#define p1_d_repeat input_repeat[1]
#define p1_l_repeat input_repeat[2]
#define p1_r_repeat input_repeat[3]
#define p0_u_repeat input_repeat[4]
#define p0_d_repeat input_repeat[5]
#define p0_l_repeat input_repeat[6]
#define p0_r_repeat input_repeat[7]
#define p0_b_repeat input_repeat[8]
#define p1_b_repeat input_repeat[9]
#define RESET_swch_repeat input_repeat[10]
#define SELECT_swch_repeat input_repeat[11]

// internal control handling variables - no need for direct user access
unsigned short input_counter[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned short input_target[12];


//------------------------------------------------------------------------------

void setGameState(enum GAME_STATE state) {

    nextGameState = state;    // actioned in OS
}


void (*const initialiseGameState[GS_MAX])() = {

    Null,                            // 0
    initGameState_DetectConsole,     // 1  GS_DETECT_CONSOLE
    initGameState_Copyright,         // 2  GS_COPYRIGHT
    initGameState_CouchCompliant,    // 4  GS_COUCH_COMPLIANT
    initGameState_Menu,              // 5  GS_MENU
    initGameState_Game,              // 6  GS_GAME
    initGameState_Skull,             // 7  GS_SKULL
    initGameState_Globe,             // 8  GS_GLOBE
    initGameState_RasterBleed,       // 9  GS_RASTER_BLEED
};

void (*const initialiseKernel[_KERNEL_MAX])() = {

    initKernel_DetectConsole,     // 0
    initKernel_Copyright,         // 2
    initKernel_CouchCompliant,    // 3
    initKernel_Menu,              // 4
    initKernel_Game,              // 5
    initKernel_Skull,             // 6
    initKernel_Globe,             // 7
    initKernel_RasterBleed,       // 8
};

//------------------------------------------------------------------------------

void (*const runFunc[])() = {

    Null,                    // _RUNARM_NULL
    runARM_SystemReset,      // _RUNARM_SYSTEM_RESET
    runARM_Load_SaveKey,     // _RUNARM_LOAD_SAVEKEY
    runARM_VerticalBlank,    // _RUNARM_VB_VBLANK
    runARM_Overscan,         // _RUNARM_OS_VBLANK
};


int main() {    // <-- 6507/ARM interfaced here!

    (*runFunc[RAM[_RUN_FUNC]])();
    return 0;
}


void Null() {
}


void runARM_SystemReset() {

    for (int i = 0; i < 34; i++)
        setIncrement(i, 1, 0);    // all fetcher increments to 1

    gameState = GS_NULL;
    setGameState(GS_DETECT_CONSOLE);

#if ENABLE_SWIPE
    setSwipePhase(SWIPE_GROW);
#endif

    RAM[_kernel] = _KERNEL_DETECT_CONSOLE;
    RAM[_tvSystem] = tvSystem = _TV_SYSTEM_NTSC;
    RAM[_soundMode] = soundMode = _SND_MODE_TIA;
    RAM[_colubk] = colubk = 0;

    setPointer(DS31PTR, _kernel);    // pass initial state to Atari

    saveKeyEnableICC = 1;

    initAudio(true);
    startMusic();
}


void runARM_Load_SaveKey() {

    if (RAM[_SK_ID] == _WENHOP_SK_ID) {

        // Transfer 6502 SaveKey state to local variables

        saveKeyEnableICC = RAM[_SK_ENABLE_ICC];

        // Dubious fixes for illegal SK values
        // TODO: put any other 'catch errors' here - it will "fix" the SK

        if (saveKeyEnableICC > CC_ICC) {
            saveKeyEnableICC = RAM[_SK_ENABLE_ICC] = CC_PCC;    // error!  FIX IT
        }

        unsigned short *odometer = (unsigned short *)(RAM + _SK_ODOMETER);
        (*odometer)++;
    }

    // init random AFTER SK, so we can use ODOMETER as a seed

    initRandom();
}


// -----------------------------------------------------------------------------

void (*const verticalBlank[GS_MAX])() = {

    Null,                 // 0
    VB_DetectConsole,     // 1
    VB_Copyright,         // 2
    VB_CouchCompliant,    // 3
    VB_Menu,              // 4
    VB_Game,              // 5
    VB_Skull,             // 6
    VB_Globe,             // 7
    VB_RasterBleed,       // 8
};


void runARM_VerticalBlank() {

    availableIdleTime = RAM[_INTIM] * armCycles - 5000;

    if (gameState == nextGameState)
        (*verticalBlank[gameState])();

    // common to ALL VB...
    // (nothing yet)
}

// -----------------------------------------------------------------------------

void (*const overscan[GS_MAX])() = {

    Null,                 // 0
    OS_DetectConsole,     // 1  GS_DETECT_CONSOLE
    OS_Copyright,         // 2  GS_COPYRIGHT
    OS_CouchCompliant,    // 3  GS_COUCH_COMPLIANT
    OS_Menu,              // 4  GS_MENU
    OS_Game,              // 5  GS_GAME
    OS_Skull,             // 6  GS_SKULL
    OS_Globe,             // 7  GS_GLOBE
    OS_RasterBleed,       // 8 GS_RASTER_BLEED
};

const unsigned char whichKernel[GS_MAX] = {    // matches kernel's own type -- all _KERNEL_* values are tiny, never written

    0,                         // 0
    _KERNEL_DETECT_CONSOLE,    // 1 GS_DETECT_CONSOLE
    _KERNEL_COPYRIGHT,         // 2 GS_COPYRIGHT
    _KERNEL_COPYRIGHT,         // 3 GS_COUCH_COMPLIANT (re-used COPYRIGHT)
    _KERNEL_MENU,              // 4 GS_MENU
    _KERNEL_GAME,              // 5 GS_GAME
    _KERNEL_SKULL,             // 6 GS_SKULL
    _KERNEL_GLOBE,             // 7 GS_GLOBE
    _KERNEL_COPYRIGHT,         // 8 GS_RASTER_BLEED
};

void runARM_Overscan() {


    availableIdleTime = RAM[_INTIM] * armCycles - 5000;


    playAudio();

    (*overscan[gameState])();

    if (gameState != nextGameState) {

        gameState = nextGameState;
        (*initialiseGameState[gameState])();

        if (whichKernel[gameState] != kernel) {
            kernel = whichKernel[gameState];
            (*initialiseKernel[kernel])();
        }
    }


    frame++;

    fadeBackgroundColour();

    RAM[_kernel] = kernel;
    RAM[_tvSystem] = tvSystem;    // could be one-off only
    RAM[_soundMode] = soundMode;

    RAM[_colubk] = colubk;    // some kernels will have a DS, some use this single colour

    setPointer(DS31PTR, _kernel);    // reset so we get vars properly
}

// -----------------------------------------------------------------------------

// Controller Handler - converts raw input to debounced pulsed wait and repeat
// timings

void HandleControls() {

    unsigned short SWCH_input = (unsigned short)RAM[_SWCHA];

    SWCH_input |= ((RAM[_INPT4] >> 7) & 1) << 8      //
                  | ((RAM[_INPT5] >> 7) & 1) << 9    //
                  | (RAM[_SWCHB] & 1) << 10          //
                  | ((RAM[_SWCHB] >> 1) & 1) << 11;

    CBW_swch = ((RAM[_SWCHB] & 0b00001000) != 0);
    P0_diff = ((RAM[_SWCHB] & 0b01000000) != 0);
    P1_diff = ((RAM[_SWCHB] & 0b10000000) != 0);

    for (int i = 0; i <= 11; i++) {
        input_flag[i] = false;
        if ((SWCH_input & 1) == 0) {
            input_counter[i]++;
            if (input_counter[i] == 1) {
                input_flag[i] = true;
                input_target[i] = input_wait[i] + 1;
            }
            if (input_counter[i] == input_target[i]) {
                input_flag[i] = true;
                input_target[i] = input_target[i] + input_repeat[i] + 1;
            }
        } else {
            input_counter[i] = 0;
        }
        SWCH_input = SWCH_input / 2;
    }
}

// Used to set TIA sound to all silent / no note
void SilenceTIA() {

    for (int i = 0; i < 2; i++) {
        RAM[_BUF_AUDV + i] = 0;
        RAM[_BUF_AUDC + i] = 0;
        RAM[_BUF_AUDF + i] = 0;
    }
}

void setJumpVectors(unsigned int buffer, short int loopAddress, short int endAddress, int length) {

    for (int i = 0; i < length - 1; i++)
        RAM_2B[(buffer / 2) + i] = loopAddress;
    RAM_2B[(buffer / 2) + length - 1] = endAddress;
}


int dirFromCoords(int x, int y, int prevX, int prevY) {

    int dir = 0;
    if (x < prevX)
        dir |= DIR_L;
    if (x > prevX)
        dir |= DIR_R;

    if (y < prevY)
        dir |= DIR_U;
    if (y > prevY)
        dir |= DIR_D;

    return dir;
}

void initNewGame() {

    actualScore = 0;
    partialScore = 0;

    lives = 3;

    initNextLife();
}


void initNextLife() {

    pulsePlayerColour = 0;
}


void initDataStreams(const struct dataStreams *streams, int streamCount) {

    for (int i = 0; i < streamCount; i++)
        setPointer(streams[i].dataStream, streams[i].buffer);
}


// EOF
