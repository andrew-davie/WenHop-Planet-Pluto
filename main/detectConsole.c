#include <stdbool.h>

#include "defines_dasm.h"
#include "detectConsole.h"

#include "cdfjplus.h"
#include "main.h"
#include <limits.h>

int tvSystem;

#define DETECT_FRAME_COUNT 10
#define ENABLE_60MHZ_AUTODETECT 0
#define ENABLE_SECAM 1

bool detectConsoleType() {

	bool finished = false;
	static unsigned int detectedPeriod;

	switch (frame) {

	case 0:

		tvSystem = _TV_SYSTEM_NTSC; // force NTSC frame for autodetect purposes

		T1TC = 0;
		T1TCR = 1;
		break;

	case DETECT_FRAME_COUNT: {

		detectedPeriod = T1TC;

		finished = true;

		static const struct fmt {

			int frequency;
			unsigned char format;

		} mapTimeToFormat[] = {

#define NTSC_70MHZ (0xB240F6 * DETECT_FRAME_COUNT / 10)
#define PAL_70MHZ (0xB3E40D * DETECT_FRAME_COUNT / 10)

			{
				NTSC_70MHZ,
				_TV_SYSTEM_NTSC,
			},

#if ENABLE_SECAM
#define SECAM_70MHZ                                                            \
	(((PAL_70MHZ - NTSC_70MHZ) / 2 + NTSC_70MHZ) * DETECT_FRAME_COUNT / 10)
			{
				SECAM_70MHZ,
				_TV_SYSTEM_SECAM,
			},
#endif
			{
				PAL_70MHZ,
				_TV_SYSTEM_PAL60,
			},

#if ENABLE_60MHZ_AUTODETECT

#define NTSC_60MHZ (0x98EB2F * DETECT_FRAME_COUNT / 10)
#define PAL_60MHZ (0x9A0EEF * DETECT_FRAME_COUNT / 10)
#define SECAM_60MHZ                                                            \
	(((PAL_60MHZ - NTSC_60MHZ) / 2 + NTSC_60MHZ) * DETECT_FRAME_COUNT / 10)

			{
				NTSC_60MHZ,
				_TV_SYSTEM_NTSC,
			},
#if ENABLE_SECAM
			{
				SECAM_60MHZ,
				_TV_SYSTEM_SECAM,
			},
#endif
			{
				PAL_60MHZ,
				_TV_SYSTEM_PAL60,
			},
#endif
		};

		int delta = INT_MAX;
		for (unsigned int i = 0;
			 i < sizeof(mapTimeToFormat) / sizeof(struct fmt); i++) {

			int dist = detectedPeriod - mapTimeToFormat[i].frequency;
			if (dist < 0)
				dist = -dist;

			if (dist < delta) {
				delta = dist;
				tvSystem = mapTimeToFormat[i].format;
				RAM[_tv_system] = tvSystem;
			}
		}

		break;
	}

	default:
		break;
	}

	return finished;
}

// EOF
