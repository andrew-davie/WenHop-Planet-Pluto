#ifndef __MENU_H
#define __MENU_H

#define MUSTWATCH_STATS 0x200

#if ENABLE_DEBUG
#define MUSTWATCH_COPYRIGHT 0x20
#define MUSTWATCH_MENU 0x200
#else
#define MUSTWATCH_COPYRIGHT 0x20
#define MUSTWATCH_MENU 0x400
#endif

#define DETECT_FRAME_COUNT 10

void MenuOverscan();
void MenuVerticalBlank();
void SchedulerMenu();
void initKernel(int krn);
// void clearBuffer(int *buffer, int size);
void clearBuffer(int *buffer, int size);

void initMenuDatastreams();

extern unsigned int detectedPeriod;

#endif