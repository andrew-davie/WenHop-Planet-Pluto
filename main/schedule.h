#pragma once

enum SCHEDULE {
    SCHEDULE_UNPACK_CAVE,
    SCHEDULE_START_SCAN,
    SCHEDULE_PROCESS_BOARD,

    SCHEDULE_MAX
};

extern enum SCHEDULE gameSchedule;

void setSchedule(enum SCHEDULE nextGameSchedule);
void scheduledTasks();

// EOF
