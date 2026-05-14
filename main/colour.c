#include <stdbool.h>

#include "colour.h"
#include "main.h"
#include "savekey.h"

int flashTime = 1;
int luminance = 0;

// static int lastBgCol;
int openSlot;
int roller;

void interleaveColour(int *r) {
    if (++*r > saveKeyEnableICC)
        *r = 0;
}

void adjustLuminance(int /*rateMask*/) {

    if (luminance < 0)
        luminance++;
    if (luminance > 0)
        luminance = 0;    // MAJOR error
}

unsigned char TranslateColour[] = {0x00, 0x20, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0,
                                   0xD0, 0xB0, 0x90, 0x70, 0x70, 0x50, 0x20, 0x40};

unsigned char TranslateSecamColour[] = {0, 0xE, 0xC, 4, 4, 6, 6, 2, 2, 2, 8, 8, 8, 8, 0xC, 0xC};

unsigned char secamConvert(unsigned char col) {
    unsigned char c = TranslateSecamColour[col >> 4];

    if ((col & 0xF) >= 4) {
        if (!c)    // black -> white
            c = 0xE;
        else if (c == 2)    // blue -> aqua
            c = 0xA;
    }
    return c;
}

unsigned char convertColour(unsigned char colour) {
    switch (tvSystem) {

    case _TV_SYSTEM_SECAM: {
        colour = secamConvert(colour);
        return colour;
        break;
    }

    case _TV_SYSTEM_PAL:
    case _TV_SYSTEM_PAL60:
        colour = TranslateColour[colour >> 4] | (colour & 0xF);
        break;

    default:
        break;
    }

    return colour;    // adjustBrightness(colour);
}

void pulseBackgroundColour(unsigned char colour, int time) {
    colubk = convertColour(colour);
    flashTime = time;
}


void fadeBackgroundColour() {

    if (flashTime) {
        if (!--flashTime) {

            if (!(colubk & 0xF) || tvSystem == _TV_SYSTEM_SECAM)
                colubk = 0;
            else {

                colubk--;
                flashTime = 1;
            }
        }
    }
}


#if 0
void chooseBackgroundPalette() {

	luminance = 0;
	if (++carColour >= (int)(sizeof(carColours) / sizeof(carColours[0])))
		carColour = 0;
}
#endif

unsigned char adjustBrightness(unsigned char colour) {

    // Remember if iCC is operating, we will see blended colours (with black/no
    // pixel) So even if we set all white, we WILL see shades of grey.  So we
    // can fade to black OK But we cannot fade to white - only fade to bright.

    int lum = (colour & 0xF) + luminance;

    if (lum < 0) {
        lum = 0;
        colour = 0;
    }

    if (lum > 14) {
        colour = 0;
        lum = 14;
    }

    return (colour & 0xF0) | lum;
}

bool ignoreSelect = false;

#if 0
void setGamePalette() {

	unsigned char bgCol = ARENA_COLOUR; // flashTime ? ARENA_COLOUR : 0x44;

	int i = 0;
	unsigned char *pfCol = RAM + _BUF_COLUPF;
	unsigned char *bkCol = RAM + _BUF_COLUBK;
	unsigned char *bk2Col = RAM + _BUF_COLUBK2;

#if RANDOM_CAR_COLOUR
	int bgCharLine = 0;
	int pfCharLine = 0;
	int size = CHAR_HEIGHT;
#endif

	unsigned char rollColour[5];

	unsigned char grey = convertColour(2);

	bk2Col[0] = bk2Col[1] = grey;

	lastBgCol = bgCol;
	for (int j = 0; j < _ARENA_SCANLINES; j++) {
		bkCol[j] = convertColour(bgCol);
		bk2Col[j + 2] = grey;
	}

	if (openPassage) {
		if (openSlot < 16)
			openSlot++;
	} else {
		if (openSlot > 0)
			openSlot -= 2;
	}

	if (openSlot) {
		for (int j = 82 - openSlot; j < 82 + openSlot; j++)
			bk2Col[j] = 0;

		int gutter = convertColour(0x4);

		for (int i = 0; i < (openSlot >> 2) + 1; i++) {
			bk2Col[82 - openSlot - i - 1] = gutter;
			bk2Col[82 + openSlot + i + 1] = gutter;
		}
	}

	// The BOTTOM border bar of the parking lot...

	bkCol[0] = bkCol[_ARENA_SCANLINES - 1] = bkCol[_ARENA_SCANLINES - 2] = grey;

#if RANDOM_CAR_COLOUR
	rollColour[0] = rollColour[3] = convertColour(fgPalette[1]);
	rollColour[1] = rollColour[4] = bgPalette[pfCharLine];
	rollColour[2] = convertColour(fgPalette[0]);
#else
	rollColour[0] = rollColour[3] = convertColour(carColours[carColour][2]);
	rollColour[1] = rollColour[4] = convertColour(carColours[carColour][1]);
	rollColour[2] = convertColour(carColours[carColour][0]);

#endif
	int roll = roller;

	int r1 = roll + 1;
	int r2 = roll + 2;

	while (i < _ARENA_SCANLINES) {

		pfCol[0] = rollColour[roll];
		pfCol[1] = rollColour[r1];
		pfCol[2] = rollColour[r2];

		pfCol += 3;

#if RANDOM_CAR_COLOUR
		bgCharLine += 3;
		if (bgCharLine >= size) {
			bgCharLine = 0;
			rollColour[1] = rollColour[4] = bgPalette[++pfCharLine];
		}

#endif

		i += 3;
	}
}
#endif

// EOF