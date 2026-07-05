#include "cdfjplus.h"
#include "defines_dasm.h"

#include "colour.h"
#include "gameState.h"
#include "main.h"


void initKernel_Rainbow() {

    setJumpVectors(_BUF_RB_JUMP, _rainbowLoop, _rainbowExit, _SCANLINES);
    setPointer(DSJMP1PTR, _BUF_RB_JUMP);
}


void initGameState_Rainbow() {
    luminance = -15;
    lumTarget = 0;
    frame = 0;
}


void VB_Rainbow() {

    setPointer(DSJMP1PTR, _BUF_RB_JUMP);

    static unsigned int col;
    col++;

    unsigned char *p = RAM + _BUF_RB_COLUBK;
    for (int i = 0; i < _SCANLINES; i++)
        *p++ = convertColour((col + (i >> 1)) & 0xFF);

    setPointer(_DS_RBW_COLUBK, _BUF_RB_COLUBK);

    if (frame > 200)
        //        setGameState(GS_DETECT_CONSOLE);
        setGameState(GS_COUCH_COMPLIANT);
}

void OS_Rainbow() {
    adjustLuminance(3);
}

// EOF
