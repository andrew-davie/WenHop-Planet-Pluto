#pragma once

#define TIMER_ON int st = T1TC;
#define TIMER_OFF                                                                                                      \
    if (T1TC - st > actualScore)                                                                                       \
        actualScore = T1TC - st;

#define T1TC_EXTRA 100
#define TIMED(name, ticks) enum { T1TC_##name = ((ticks) + T1TC_EXTRA) }
#define TIME_OK(name) availableIdleTime - T1TC > T1TC_##name

// EOF
