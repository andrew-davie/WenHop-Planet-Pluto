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
    GS_GAME,               // 6
    GS_SKULL,              // 7
    GS_GLOBE,              // 8
    GS_RASTER_BLEED,       // 9

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

void initGameState_Game();
void VB_Game();
void OS_Game();

void initGameState_Skull();
void VB_Skull();
void OS_Skull();

void initGameState_Globe();
void VB_Globe();
void OS_Globe();

void initGameState_RasterBleed();
void VB_RasterBleed();
void OS_RasterBleed();

// EOF
