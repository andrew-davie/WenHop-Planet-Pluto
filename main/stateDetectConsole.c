#include "cdfjplus.h"
#include "defines_dasm.h"

#include "detectConsole.h"
#include "state.h"

// -----------------------------------------------------------------------------
// Console detection displays black screen for DETECT_FRAME_COUNT (10) frames

void VB_DetectConsole() {
	if (detectConsoleType())
		game_state = GS_COPYRIGHT;
}

void OS_DetectConsole() {
}

// EOF
