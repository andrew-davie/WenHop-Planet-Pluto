#pragma once

enum GAME_STATE {

    // Controls which VB and OS are run
    // Change through 'setGameState', change happens in OS

    GS_NULL,               // 0
    GS_DETECT_CONSOLE,     // 1
    GS_COPYRIGHT,          // 2
    GS_RAINBOW,            // 3
    GS_COUCH_COMPLIANT,    // 4

    GS_MAX

};

void initialise_GS_Rainbow();
void VB_GS_Rainbow();
void OS_GS_Rainbow();

void initialise_GS_Copyright();
void VB_GS_Copyright();
void OS_GS_Copyright();

void initialise_GS_DetectConsole();
void VB_GS_DetectConsole();
void OS_GS_DetectConsole();

void initialise_GS_CouchCompliant();
void VB_GS_CouchCompliant();
void OS_GS_CouchCompliant();

// EOF
