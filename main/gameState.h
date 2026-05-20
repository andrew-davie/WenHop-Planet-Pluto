#pragma once

enum GAME_STATE {

    // Controls which VB and OS are run
    // Change through 'setGameState', change happens in OS

    GS_NULL,               // 0
    GS_DETECT_CONSOLE,     // 1
    GS_COPYRIGHT,          // 2
    GS_RAINBOW,            // 3
    GS_COUCH_COMPLIANT,    // 4
    GS_MENU,               // 5

    GS_MAX

};

void initGameState_Rainbow();
void VB_Rainbow();
void OS_Rainbow();

void initGameState_Copyright();
void VB_Copyright();
void OS_Copyright();

void initGameState_DetectConsole();
void VB_DetectConsole();
void OS_DetectConsole();

void initGameState_CouchCompliant();
void VB_CouchCompliant();
void OS_CouchCompliant();

void initGameState_Menu();
void VB_Menu();
void OS_Menu();

// EOF
