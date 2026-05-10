
#pragma once

enum GAME_STATE {

	// Controls which VB and OS are run

	GS_DETECT_CONSOLE, // 0
	GS_COPYRIGHT,	   // 1
	GS_DEMO,		   // 2

};

extern enum GAME_STATE game_state;

void VB_Rainbow();
void OS_Rainbow();

void VB_Copyright();
void OS_Copyright();

void VB_DetectConsole();
void OS_DetectConsole();

// EOF
