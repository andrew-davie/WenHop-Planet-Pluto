#include "cdfjplus.h" // <- contains references from defines_dasm.h

#include "colour.h"
#include "draw.h"
#include "main.h"

void draw6Bitmap(const unsigned char bitmap6[][6], int height, int y, int colour) {

	// Draw a 6-sprite wide bitmap

	unsigned char *p = RAM + _BUF_MENU_GRP0A + y;
	unsigned char *q = RAM + _BUF_MENU_COLUP0 + y - 1;
	const unsigned char *bm = &bitmap6[0][0];

	colour = convertColour(colour);

	if (y + height >= _ARENA_SCANLINES)
		height = _ARENA_SCANLINES - y;

	for (int line = 0; line < height; line++) {
		q[line] = colour;
		for (int c = 0; c < 6; c++) {
			p[c * _ARENA_SCANLINES + line] = *bm++;
		}
	}
}
