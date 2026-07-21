#pragma once

enum SCHEDULE {
    SCHEDULE_NONE,          // idle -- nothing pending; scheduledTasks() is never dispatched while set
    SCHEDULE_INIT_STATE,    // deferred cross-state init in progress -- see scheduleInitState() in main.c
    SCHEDULE_UNPACK_CAVE,
    SCHEDULE_START_SCAN,
    SCHEDULE_PROCESS_BOARD,

    SCHEDULE_MAX
};

extern enum SCHEDULE gameSchedule;

void setSchedule(enum SCHEDULE nextGameSchedule);
void scheduledTasks();
void scheduleInitState();    // defined in main.c -- has natural access to gameState/nextGameState/kernel
                              // and the initialiseGameState[]/initialiseKernel[]/whichKernel[] tables

// EOF
