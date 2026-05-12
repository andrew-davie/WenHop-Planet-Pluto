#include "cdfjplus.h"
#include "defines_dasm.h"
#include "main.h"
#include "sound.h"

#include "colour.h"
#include "detectConsole.h"
#include "draw.h"
#include "grid6.h"
#include "state.h"

#define CHAMP_VOL 100

// clang-format off

const unsigned char trackChamp1[] = {

    HALFNOTE c5 c5
    HALFNOTE d5
    FULLNOTE c5
    HALFNOTE g4 g5
    FULLNOTE e5
    TRACK_END
};

const unsigned char trackChamp2[] = {

    FULLNOTE
    c4 g3 c4 g3
    TRACK_VOLUME, CHAMP_VOL*2, 
    HALFNOTE e3 g3 e3 g3
    FULLNOTE c4
    TRACK_END
};

// clang-format on

void initialise_GS_COPYRIGHT() {

#define CC 4

	//	setJumpVectors(_CHAMP_KERNEL, _EXIT_CHAMP_KERNEL, _ARENA_SCANLINES - 1);

	// sets the menu sprites position
	RAM[_P0_X] = 56;
	RAM[_P1_X] = 64;

	sound_volume = VOLUME_NONPLAYING;
	loadTrack(20, trackChamp1, CHAMP_VOL + 80, 0x54, 0);
	loadTrack(10, trackChamp2, CHAMP_VOL, 0x54, 1);

	frame = 0;
}

void VB_Copyright() {

	if (frame < 250) {

#define CGSPACER 4
#define TOP (_ARENA_SCANLINES / 2 - BAND - 15)
#define CGP (gfx_grid_champgames_champ_gif_HEIGHT)
#define BAND (CGP + CGSPACER * 2)

		unsigned char *l = RAM + _BUF_MENU_PF2_LEFT;
		unsigned char *r = RAM + _BUF_MENU_PF2_RIGHT;
		unsigned char *c = RAM + _BUF_MENU_COLUPF;
		unsigned char *spc = RAM + _BUF_MENU_COLUP0;

		for (int sl = 0; sl < TOP; sl++)
			*l++ = *r++ = *c++ = *spc++ = 0;

		for (int sl = TOP; sl < TOP + 2 * BAND; sl++) {

			*l++ = 255;
			*r++ = 255;
			*spc++ = 0x6;

			if (tvSystem == _TV_SYSTEM_NTSC)
				*c++ = convertColour(sl < TOP + BAND ? 0x90 : 0x40);
			else
				*c++ = convertColour(sl < TOP + BAND ? 0x92 : 0x42);
		}

		for (int sl = TOP + 2 * BAND; sl < _ARENA_SCANLINES; sl++)
			*l++ = *r++ = *c++ = *spc++ = 0;

		draw6Bitmap(gfx_grid_champgames_champ_gif, gfx_grid_champgames_champ_gif_HEIGHT, TOP + CGSPACER + 1, 8);
		draw6Bitmap(gfx_grid_champgames_games_gif, gfx_grid_champgames_games_gif_HEIGHT, TOP + BAND + CGSPACER + 1, 8);

#if UNLIMITED_SOLVES
//		_ARENA_COLOUR = 0x40;
//		draw6Bitmap(gfx_grid_not_for_release_gif,
// gfx_grid_not_for_release_gif_HEIGHT, TOP - 50, 8);
#endif

		if (frame > 60)

			draw6Bitmap(gfx_grid_champgames_presents_gif, gfx_grid_champgames_presents_gif_HEIGHT, TOP + 2 * BAND + 10,
						8);

#ifndef RESTRICTED_DEMO
		if ((RAM[_SK_ID] == _WENHOP_SK_ID) && frame > 100) {

			int col = convertColour(frame & 16 ? 0x16 : 0x12);

			draw6Bitmap(gfx_grid_savekey_gif, gfx_grid_savekey_gif_HEIGHT,
						_ARENA_SCANLINES - 1 - gfx_grid_savekey_gif_HEIGHT, col);

			if (RAM[_SK_RESET]) {
				draw6Bitmap(gfx_grid_savekey_reset_gif, gfx_grid_savekey_reset_gif_HEIGHT,
							_ARENA_SCANLINES - 6 - gfx_grid_savekey_gif_HEIGHT - gfx_grid_savekey_reset_gif_HEIGHT, 6);
				//				            convertColour(frame & 16 ? 6 : 2));

				if (!(frame & 15))
					ADDAUDIO(SFX_SELECTION);
			}
		}
#endif

#ifndef FINAL_VERSION
#define DBAS 110

#endif

	}

	else {

		setNextGameState(GS_COUCH_COMPLIANT);
	}

	// else {

	// 	setNextGameState(GS_DEMO);
	// }
}

void OS_Copyright() {
	playAudio();
}

// EOF
