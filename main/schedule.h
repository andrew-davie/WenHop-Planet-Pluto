#pragma once

enum SCHEDULE {
    SCHEDULE_UNPACK_CAVE,
    SCHEDULE_START,
    SCHEDULE_PROCESSBOARD,
};

extern enum SCHEDULE gameSchedule;

void setSchedule(enum SCHEDULE nextGameSchedule);
void scheduledTasks();

// EOF
