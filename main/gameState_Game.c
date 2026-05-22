#include "defines_dasm.h"


#include "cdfjplus.h"
#include "main.h"

#include "colour.h"
#include "drawScreen.h"

void initKernel_Game() {

    setJumpVectors(_BUF_GAME_JUMP, _gameLoop, _gameExit, _SCANLINES);
    setPointer(DSJMP1PTR, _BUF_GAME_JUMP);
}


void initGameState_Game() {

    frame = 0;
}

void VB_Game() {

    setPointer(DSJMP1PTR, _BUF_GAME_JUMP);

    setPointer(_DS_GAME_COLUBK, _BUF_GAME_COLUBK);
    setPointer(_DS_GAME_COLUPF, _BUF_GAME_COLUPF);
    setPointer(_DS_GAME_COLUP0, _BUF_GAME_COLUP0);
    setPointer(_DS_GAME_COLUP1, _BUF_GAME_COLUP1);

    setPointer(_DS_GAME_PF0_LEFT, _BUF_GAME_PF0_LEFT);
    setPointer(_DS_GAME_PF1_LEFT, _BUF_GAME_PF1_LEFT);
    setPointer(_DS_GAME_PF2_LEFT, _BUF_GAME_PF2_LEFT);
    setPointer(_DS_GAME_PF0_RIGHT, _BUF_GAME_PF0_RIGHT);
    setPointer(_DS_GAME_PF1_RIGHT, _BUF_GAME_PF1_RIGHT);
    setPointer(_DS_GAME_PF2_RIGHT, _BUF_GAME_PF2_RIGHT);


    static unsigned int col;
    col++;

    unsigned char *p = RAM + _BUF_GAME_COLUBK;
    unsigned char *c = RAM + _BUF_GAME_COLUPF;

    for (int i = 0; i < _SCANLINES; i++) {
        *p++ = 0;    // convertColour((col + (i >> 1)) & 0xFF);

        *c++ = 0xF;
    }

    setPointer(_DS_GAME_COLUBK, _BUF_GAME_COLUBK);

    if (frame > 500)
        setGameState(GS_RAINBOW);

    return;
}

void OS_Game() {

    drawScreen();
}

// EOF
