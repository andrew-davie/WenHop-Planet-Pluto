#pragma once

#include <stdbool.h>


extern int scrollX;
extern int scrollY;

void scroll();
void resetTracking();
bool isScrolling();


#define SCROLL_MAX_X ((BOARD_TRIX_X - SCREEN_TRIX_X) << 16)
#define SCROLL_MIN_X 0
#define SCROLL_MAX_Y ((BOARD_TRIX_Y - SCREEN_TRIX_Y) << 16)
#define SCROLL_MIN_Y 0


// EOF
