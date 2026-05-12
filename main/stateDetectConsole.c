#include "cdfjplus.h"
#include "defines_dasm.h"

#include "detectConsole.h"
#include "main.h"
#include "state.h"

// -----------------------------------------------------------------------------
// Console detection displays black screen for DETECT_FRAME_COUNT (10) frames

void initialise_GS_DETECT_CONSOLE() {
}

void VB_DetectConsole() {
	if (detectConsoleType())
		setNextGameState(GS_COPYRIGHT);
}

void OS_DetectConsole() {
}

// EOF
