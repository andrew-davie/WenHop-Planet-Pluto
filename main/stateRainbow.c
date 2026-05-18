#include "cdfjplus.h"
#include "defines_dasm.h"

#include "colour.h"
#include "main.h"
#include "sound.h"
#include "state.h"

void initialise_GS_Rainbow() {
    frame = 0;
}

void VB_GS_Rainbow() {

    static unsigned int col;
    col++;

    unsigned char *p = RAM + _BUF_RAINBOW_COLUBK;
    for (int i = 0; i < _SCANLINES; i++)
        *p++ = convertColour((col + (i >> 1)) & 0xFF);

    setPointer(_DS_RBW_COLUBK, _BUF_RAINBOW_COLUBK);
    setPointer(DSJMP1PTR, _BUF_RAINBOW_JUMP);

    if (frame > 200)
        setGameState(GS_DETECT_CONSOLE);
}

void OS_GS_Rainbow() {

    playAudio();
}

// EOF
