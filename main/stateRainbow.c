#include "cdfjplus.h"
#include "defines_dasm.h"

#include "colour.h"
#include "sound.h"
#include "state.h"

void initialise_GS_DEMO() {
}

void VB_Rainbow() {

	static unsigned int col;
	col++;

	for (int i = 0; i <= 191; i++)
		RAM[_buffer0 + i] = convertColour((col + (i >> 1)) & 0xFF);

	setPointer(DS0PTR, _buffer0);
}

void OS_Rainbow() {

	playAudio();
}

// EOF
