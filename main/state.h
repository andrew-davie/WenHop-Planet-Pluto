#pragma once

enum GAME_STATE {

	// Controls which VB and OS are run
	// Convention: Change only in OS

	GS_DETECT_CONSOLE, // 0
	GS_COPYRIGHT,	   // 1
	GS_DEMO,		   // 2

	GS_MAX

};

extern enum GAME_STATE gameState;

void VB_Rainbow();
void OS_Rainbow();

void VB_Copyright();
void OS_Copyright();

void VB_DetectConsole();
void OS_DetectConsole();

// EOF
