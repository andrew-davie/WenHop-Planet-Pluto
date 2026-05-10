#include "cdfjplus.h"
#include "defines_dasm.h"
#include "sound.h"

#include "state.h"

void VB_Copyright() {

	gameState = GS_DEMO;
}

void OS_Copyright() {
	playAudio();
}

// EOF
