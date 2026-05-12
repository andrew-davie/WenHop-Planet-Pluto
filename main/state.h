#pragma once

enum GAME_STATE {

	// Controls which VB and OS are run
	// Change through 'setNextGameState', change happens in OS

	GS_NULL,			// 0
	GS_DETECT_CONSOLE,	// 1
	GS_COPYRIGHT,		// 2
	GS_DEMO,			// 3
	GS_COUCH_COMPLIANT, // 4

	GS_MAX

};

// extern enum GAME_STATE gameState;

void initialise_GS_DETECT_CONSOLE();
void initialise_GS_COPYRIGHT();
void initialise_GS_DEMO();
void initialise_GS_COUCH_COMPLIANT();

void VB_Rainbow();
void OS_Rainbow();

void VB_Copyright();
void OS_Copyright();

void VB_DetectConsole();
void OS_DetectConsole();

void VB_CouchCompliant();
void OS_CouchCompliant();

// EOF
