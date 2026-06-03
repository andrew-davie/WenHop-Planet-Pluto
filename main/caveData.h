#pragma once

extern const unsigned char *caveList[];
extern const int caveCount;
extern unsigned char caveFlags;


#define COLOURPOOL_SIZE 115

#define CAVEDEF_OVERVIEW 0x80
#define CAVEDEF_PARALLAX 0x40
#define CAVEDEF_RAINBOW 0x20
#define CAVEDEF_LEARN 0x10
#define CAVEDEF_ROCK_GENERATE 0x08

#define CAVEDEF_LOCK_X 4
#define CAVEDEF_LOCK_Y 2
#define CAVEFED_NOBORDER 1


#define CAVE_REQUIRES_COMPATIBLE_PALETTE 0x80000000


#define DRAW_EOF 255
#define DRAW_FILLED_RECT 254
#define DRAW_RECT 253
#define DRAW_LINE 252

#define DRAW_OBJ 251 /* "last" */


// EOF
