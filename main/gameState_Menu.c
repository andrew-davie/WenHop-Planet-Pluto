#include "defines_dasm.h"

#include "cdfjplus.h"
#include "colour.h"
#include "main.h"

void initKernel_Menu() {

    setJumpVectors(_BUF_MENU_JUMP, _menuLoop, _menuExit, _SCANLINES);
    setPointer(DSJMP1PTR, _BUF_MENU_JUMP);
}


void initGameState_Menu() {

    frame = 0;
}

void VB_Menu() {

    setPointer(DSJMP1PTR, _BUF_MENU_JUMP);


    static unsigned int col;
    col--;

    unsigned char *p = RAM + _BUF_RB_COLUBK;
    for (int i = 0; i < _SCANLINES; i++)
        *p++ = convertColour((col + (i >> 1)) & 0xFF);

    setPointer(_DS_MENU_COLUBK, _BUF_MENU_COLUBK);


    if (frame > 200)
        setGameState(GS_RAINBOW);
}

void OS_Menu() {
}

// EOF
