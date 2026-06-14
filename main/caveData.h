#pragma once

extern const unsigned char *caveList[];
extern const int caveCount;
extern unsigned char caveFlags;


#define CAVEDEF_LOCK_X 4
#define CAVEDEF_LOCK_Y 2
#define CAVEDEF_NOBORDER 1
#define CAVEDEF_STAR_STATIC 8
#define CAVEDEF_START_WITH_WEAPON 16


#define DRAW_EOF 255
#define DRAW_FILLED_RECT 254
#define DRAW_RECT 253
#define DRAW_LINE 252

#define DRAW_OBJ 251 /* "last" */


// EOF
