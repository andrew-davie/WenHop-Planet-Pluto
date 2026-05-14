#include "cdfjplus.h"
#include "defines_dasm.h"

#include "colour.h"
#include "sound.h"
#include "state.h"

void initialise_GS_Rainbow() {
}

void VB_GS_Rainbow() {

    static unsigned int col;
    col++;

    for (int i = 0; i < _SCANLINES; i++)
        RAM[_BUF_RAINBOW_COLUBK + i] = convertColour((col + (i >> 1)) & 0xFF);

    setPointer(DS0PTR, _BUF_RAINBOW_COLUBK);
}

void OS_GS_Rainbow() {

    playAudio();
}

// EOF
