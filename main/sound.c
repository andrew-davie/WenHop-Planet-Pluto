// clang-format off

#include "cdfjplus.h"

#include <stdbool.h>

#include "main.h"
#include "colour.h"
#include "sound.h"

//#include "joystick.h"
//#include "random.h"

// clang-format on

void loadTrack(int priority, const unsigned char *tune, int volume, int dur,
			   int instrument);
void processMusic();

// const unsigned char trackSimple[];
// static const unsigned char trackSimple2[];

const unsigned char trackGridLockMelodyIntro[];
const unsigned char trackGridLockBase[];
const unsigned char trackChamp1[];
const unsigned char trackChamp2[];
const unsigned char trackTrophy1[];
const unsigned char trackTrophy2[];

#define TRIGGER_NEXT_NOTE 0x10000
// #define NULL_TRACK 0xFF
#define TRACK_END 1
#define TRACK_LOOP 0
#define TRACK_VOLUME 2

int sound_volume = 128;
int sound_max_volume = 768;
int volume[2];

#define CHAMP_VOL 100

struct Audio sfx[CONCURRENT_SFX];
struct trackInfo music[MUSIC_MAX];

// clang-format off

// const unsigned char sampleTick[] = {
//     0xF, 0x1F, 1, 2,
//     0, 0, 0, 6,
//     0xF, 0x1F, 1, 1,
//     0, 0, 0, 40,
//     CMD_LOOP,
// };

// const unsigned char sampleDrip[] = {
//     12, 12, 4, 1,
//     CMD_STOP,
// };

// const unsigned char sampleUncover[] = {
//     12, 6, 3, 1,
//     12, 4, 3, 2,
//     0, 0, 0, 1,
//     CMD_LOOP,
// };

// const unsigned char sampleDrip2[] = {
//     12, 6, 4, 1,
//     12, 4, 4, 1,
//     CMD_STOP,
// };

// const unsigned char sampleMagic[] = {
//     0xC, 0x8, 3, 2,
//     CMD_LOOP,
// };

// const unsigned char sampleMagic2[] = {
//     0x5, 0x8, 5, 15,
//     CMD_STOP,
// };

// const unsigned char samplePick[] = {
//     9,
//     12,
//     8,
//     1,
//     9,
//     20,
//     15,
//     1,
//     9,
//     31,
//     12,
//     1,
//     CMD_STOP,
// };

// const unsigned char sampleRock[] = {
//     8, 18, 5, 4,
//     8, 18, 5, 3,
//     8, 18, 4, 2,
//     8, 18, 3, 1,
//     CMD_STOP,
// };

// const unsigned char sampleRock2[] = {
//     8, 18, 4, 4,
//     8, 18, 3, 3,
//     8, 18, 2, 3,
//     CMD_STOP,
// };

__attribute__((used)) const unsigned char sampleDirt[] = {
    8, 31, 3, 10,
    CMD_STOP,
};

// const unsigned char sampleSpace[] = {
//     8, 2, 2, 2,
//     CMD_STOP,
// };

// const unsigned char sampleBlip[] = {
//     4, 18, 5, 1,
//     4, 18, 4, 2,
//     4, 18, 3, 4,
//     CMD_STOP,
// };

// const unsigned char sampleX[] = {
//     0xC, 0x8, 1, 2, CMD_LOOP,
//     // 12,10,2,1,
//     // CMD_LOOP,
// };


// const unsigned char sampleBubbler[] = {
//     0xE, 0x8, 1, 30,
//     CMD_STOP,
// };

__attribute__((used)) const unsigned char sampleBeepHornTwice[] = {
    12, 12, 7, 20,
    0,0,0,7,
    12, 12, 7, 50,
    CMD_STOP,
};

__attribute__((used)) const unsigned char sampleFastBeep2[] = {
    12, 12, 5, 10,
    0,0,0,5,
    12, 12, 5, 20,
    CMD_STOP,
};


// const unsigned char sampleUncovered[] = {
//     12, 5, 4, 3,
//     12, 7, 4, 3,
//     12, 5, 4, 3,
//     12, 7, 4, 3,
//     CMD_STOP,
// };

// const unsigned char samplePush[] = {
//     8, 4, 4, 12,
//     CMD_STOP,
// };


__attribute__((used)) const unsigned char sampleSpinWheel[] = {
    8, 3, 3, 10,
    8, 3, 3, 20,
    8, 3, 2, 30,
    8, 3, 1, 40,
    CMD_STOP
};

__attribute__((used)) const unsigned char sampleRev[] = {

    
    // 7, 31, 15, 10,
    // 7, 31, 12, 10,
    // 7, 31, 8, 10,
    // 7, 31, 6, 10,
    // 7, 31, 4, 15,

    // 7, 31, 15, 10,
    // 7, 31, 12, 10,
    // 7, 31, 8, 10,
    // 7, 31, 6, 10,
    // 7, 31, 4, 15,




    // 7, 26, 10, 2,
    // 7, 27, 10, 2,
    // 7, 28, 10, 2,
    // 7, 29, 10, 2,
    // 7, 30, 10, 2,
    // 7, 31, 10, 45,

    // 7, 31, 10, 5,
    // 7, 30, 10, 5,
    // 7, 29, 10, 5,
    // 7, 28, 10, 5,
    // 7, 27, 10, 5,
    // 7, 26, 10, 5,
    // 7, 25, 10, 50,

    // 0,0,0,20,

    // 7, 26, 10, 2,
    // 7, 27, 10, 2,
    // 7, 28, 10, 2,
    // 7, 29, 10, 2,
    // 7, 30, 10, 2,
    // 7, 31, 10, 45,



    

//    7, 30,  0, 12,
//    7, 25,  1, 12,
//    7, 20,  2, 12,
//    7, 20,  3, 12,
//    7, 21,  4, 12,
//    7, 22,  5, 12,
//    7, 23,  6, 12,

   7, 31,  4, 12,
   7, 31,  5, 12,
   7, 31,  6, 12,


   7, 31,  7, 12,
   7, 30,  8, 12,
   7, 29,  9, 12,
   7, 28, 10, 12,
   7, 27, 11, 12,
   7, 26, 12, 12,
   7, 25, 13, 12,
   7, 24, 14, 12,

   7, 23, 12, 12,
   7, 22, 10, 12,
   7, 21, 8, 12,
   7, 20, 6, 12,
   7, 19, 4, 12,
   7, 18, 2, 12,
   7, 17, 0, 12,


    CMD_STOP,
};





const unsigned char sampleExplode[] = {
    8, 7, 15, 3,
//    8, 10, 14, 1,
    8, 13, 10, 2,
    8, 16, 6, 3,
    8, 19, 5, 4,
    8, 22, 4, 5,
    8, 25, 3, 6,
    // 8, 28, 2, 10,
    // 8, 29, 1, 15,
    // 8, 31, 1, 15,
    CMD_STOP,
};

// const unsigned char sampleExplodeQuiet[] = {
//     8, 7, 7, 2,
//     8, 13, 5, 2,
//     8, 19, 4, 4,
//     8, 25, 2, 6,
//     8, 29, 1, 15,
//     CMD_STOP,
// };

__attribute__((used)) const unsigned char sampleWhoosh[] = {
    15, 31, 1, 2*3,
    15, 31, 2, 2*3,
    15, 28, 3, 2*3,
    15, 25, 4, 3*3,
    15, 22, 5, 4*3,
    15, 19, 6, 5*3,
    15, 16, 7, 6*3,
    15, 13, 7, 5*3,
    15, 10, 4, 2,
    15, 7, 2, 1,
    CMD_STOP,
};


__attribute__((used)) const unsigned char sampleAbort[] = {


    8, 18, 6, 5,
    6, 5, 8, 3,
    6, 7, 8, 3,
    6, 5, 6, 3,
    6, 7, 6, 3,
    6, 5, 4, 3,
    6, 7, 4, 3,
    6, 5, 2, 3,
    6, 7, 2, 3,
    // dc CMD_STOP

    // 15, 7, 2, 2,
    // 15, 10, 4, 2,
    // 15, 13, 7, 10,
    // 15, 16, 7, 6,
    // 15, 19, 6, 5,
    // 15, 22, 5, 4,
    // 15, 25, 4, 3,
    // 15, 28, 3, 2,
    // 15, 31, 2, 2,
    // 15, 31, 1, 2,
    CMD_STOP,
};


// const unsigned char sample10987654321[] = {

//     4, 10, 1, 1,
//     4, 10, 3, 1,
//     4, 10, 5, 1,
//     4, 10, 6, 1,
//     4, 10, 5, 15,
//     4, 10, 3, 3,
//     4, 10, 2, 3,
//     4, 10, 1, 3,
//     CMD_STOP,
// };

// const unsigned char sampleNone[] = {
//     CMD_STOP,
// };

const unsigned char sampleSFX[] = {
    // C,F,V,LEN
    5, 14, 4, 1,
    5, 12, 6, 1,
    5, 11, 7, 1,
    5, 3, 10, 2,
    5, 4, 8, 1,
    5, 5, 6, 1,
    5, 6, 4, 1,
    5, 7, 1, 1,
    CMD_STOP,
};

const unsigned char sampleX2[] = {
    // C,F,V,LEN
    5, 14, 1, 1,
    5, 12, 2, 1,
    5, 11, 3, 1,
    5, 3, 4, 2,
    5, 3, 4,2,
    5,  4, 3, 1,
    5,  5, 1, 1,
    5,  4, 3, 1,
    5, 5, 1, 1,
    CMD_STOP,
};

#if 1
// clang-format on

const unsigned char sampleNone[] = {
	CMD_STOP,
};

const unsigned char sampleBlip[] = {
	4, 18, 5, 1, 4, 18, 4, 2, 4, 18, 3, 4, CMD_STOP,
};

const unsigned char sampleSelectionBlip[] = {
	12, 6, 4, 1, 12, 4, 4, 1, CMD_STOP,

};

const unsigned char sampleExit[] = {
	12, 16, 1,	1, 12, 16, 4,  1, 12, 16, 10, 1, 12, 16,	   8,
	4,	12, 16, 6, 1,  12, 16, 4, 1,  12, 16, 2, 1,	 CMD_STOP,
};

const unsigned char sampleMagic[] = {
	0xC, 8, 3, 2, CMD_LOOP,
};

const unsigned char sampleFireworks[] = {
	8, 18, 10, 1, 8, 18, 9, 2, 8, 18, 8, 2, 8, 18, 7, 2, 8, 18, 4, 12, CMD_STOP,

};

const unsigned char sampleSpace[] = {
	8, 2, 1, 2, CMD_STOP,

};

const struct AudioTable AudioSamples[] = {

	{sampleNone, 0, 0}, // 0  PLACEHOLDER - NOT USED AS SOUND

	// MUST correspond to AudioID enum ordering/number
	// MUST be in priority order!

	{sampleAbort, 127, 0},						 // 01 SFX_ABORT
	{sampleWhoosh, 127, 0},						 // 02 SFX_WHOOSH
	{sampleBlip, 125, 0},						 // 03 SFX_BLIP
	{sampleRev, 117, 0},						 // 04 SFX_REV
	{sampleSelectionBlip, 115, 0},				 // 05 SFX_SELECTION
	{sampleBeepHornTwice, 110, AUDIO_ATTENUATE}, // 06 SFX_BEEP2
	{sampleFastBeep2, 100, AUDIO_ATTENUATE},	 // 07 SFX_FASTBEEP2
	{sampleExit, 99, 0},						 // 08 SFX_EXIT
	{sampleSpinWheel, 96, 0},					 // 09 SFX_SPINWHEEL
	{sampleMagic, 91, AUDIO_KILL},				 // 10 SFX_MAGIC
	{sampleFireworks, 90, 0},					 // 11 SFX_FIREWORKS
	{sampleDirt, 9, 0},							 // 12 SFX_DIRT
	{sampleSpace, 8, 0},						 // 13 SFX_SPACE

};

// clang-format off
#else

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

__asm__(".section .rodata\n"
        ".global AudioSamples2\n"
        "AudioSamples2:\n"

        ".hword " STRINGIFY(__sampleNone) "\n"
        ".byte 0\n"
        ".byte 0\n"


        ".hword sampleAbort\n"
        ".byte 127\n"
        ".byte 0\n"

        ".hword sampleWhoosh\n"
        ".byte 127\n"
        ".byte 0\n"

        ".hword " STRINGIFY(__sampleBlip) "\n"
        ".byte 125\n"
        ".byte 0\n"

        ".hword sampleRev\n"
        ".byte 117\n"
        ".byte 0\n"

        ".hword " STRINGIFY(__sampleSelectionBlip) "\n"
        ".byte 92\n"
        ".byte 0\n"

        ".hword sampleBeepHornTwice\n"
        ".byte 110\n"
        ".byte " STRINGIFY(AUDIO_ATTENUATE) "\n"

        ".hword sampleFastBeep2\n"
        ".byte 100\n"
        ".byte " STRINGIFY(AUDIO_ATTENUATE) "\n"

        ".hword " STRINGIFY(__sampleExit) "\n"
        ".byte 99\n"
        ".byte 0\n"

        ".hword sampleSpinWheel\n"
        ".byte 96\n"
        ".byte 0\n"


        ".hword " STRINGIFY(__sampleMagic) "\n"
        ".byte 91\n"
        ".byte " STRINGIFY(AUDIO_KILL) "\n"

        ".hword " STRINGIFY(__sampleFireworks) "\n"
        ".byte 90\n"
        ".byte 0\n"

        ".hword sampleDirt\n"
        ".byte 9\n"
        ".byte 0\n"

        ".hword " STRINGIFY(__sampleSpace) "\n"
        ".byte 8\n"
        ".byte 0\n"

);

// clang-format on

extern const short int AudioSamples2[SFX_MAX];
const struct AudioTable *AudioSamples =
	(const struct AudioTable *)AudioSamples2;
#endif
bool audioRequest[SFX_MAX];

void killRepeatingAudio() {

	for (int channel = 0; channel < 2; channel++) {
		RAM[_AUDC0 + channel] = 0;
		RAM[_AUDF0 + channel] = 0;
		RAM[_AUDV0 + channel] = 0;
	}

	for (int i = 0; i < CONCURRENT_SFX; i++)
		if (AudioSamples[sfx[i].id].flags & AUDIO_KILL)
			sfx[i].id = 0;

	for (int i = 0; i < SFX_MAX; i++)
		if (AudioSamples[i].flags & AUDIO_KILL)
			audioRequest[i] = false;
}

void initAudio(bool killTracks) {

	killRepeatingAudio();

	if (killTracks)
		for (int i = 0; i < MUSIC_MAX; i++)
			music[i].tune = 0;

	sound_max_volume = VOLUME_MAX;
}

void startMusic() {

	loadTrack(0, trackGridLockMelodyIntro, 50, 0xC0, 1);
	loadTrack(10, trackGridLockBase, 100, 0xC0, 0);
}

void startChampJingle() {

	sound_volume = VOLUME_NONPLAYING;
	loadTrack(20, trackChamp1, CHAMP_VOL + 80, 0x54, 0);
	loadTrack(10, trackChamp2, CHAMP_VOL, 0x54, 1);
}

void startTrophyJingle() {

	//	static int speed = 0x80;

	sound_volume = VOLUME_NONPLAYING;
	loadTrack(100, trackTrophy2, 200, 0x80, 0);
	loadTrack(100, trackTrophy1, 200, 0x80, 0);

	// 	if (speed < 256)
	// 		speed++;
}

void killAudio(enum AudioID id) {

	audioRequest[id] = false;
	for (int i = 0; i < CONCURRENT_SFX; i++)
		if (sfx[i].id == id)
			sfx[i].id = 0;
}

void processSoundEffects() {

	for (unsigned int id = 0; id < SFX_MAX; id++) {

		if (audioRequest[id]) {

#if ENABLE_SOUND

			if (AudioSamples[id].flags & AUDIO_SINGLETON) {
				bool dup = false;
				for (int i = 0; i < CONCURRENT_SFX; i++)
					if (sfx[i].id == id) {
						dup = true;
						break;
					}
				if (dup)
					continue;
			}

			int lowest = -1;
			for (int i = 0; i < CONCURRENT_SFX; i++) {

				int idx = sfx[i].id;

				if (!idx) { // empty slot
					lowest = i;
					break;
				}

				if ((!(AudioSamples[idx].flags &
					   AUDIO_LOCKED) && // not locked, and...
					 (lowest < 0 ||		// either we haven't found a lowest yet
					  AudioSamples[idx].priority <
						  AudioSamples[sfx[lowest].id].priority)
					 // or the priority of this track is lower than the lowest
					 // found so far...
					 ))

					lowest = i;
			}

			// we've now found the lowest priority sound in our current batch...
			// if the lowest slot is lower priority than new sound, replace it
			if (lowest >= 0) {
				// && (!sfx[lowest].id || AudioSamples[id].priority >=
				//                                         AudioSamples[sfx[lowest].id].priority))
				//                                         {

				sfx[lowest].index = 0;
				sfx[lowest].id = id;
				sfx[lowest].delay = AudioSamples[id].sample[3];

				sfx[lowest].attenuation =
					(AudioSamples[id].flags & AUDIO_ATTENUATE)
						? rangeRandom(256)
						: 255;

				audioRequest[id] = false;
			}

			else
				break; // sounds full or higher priority, ignore any more lower
					   // priority sounds

#endif
		}
	}

	// process the sfx segments' commands

	for (int i = 0; i < CONCURRENT_SFX; i++) {
		struct Audio *sfxx = &sfx[i];
		sfxx->handled = false;

		if (sfxx->id && !--sfxx->delay) {

			const struct AudioTable *s = &AudioSamples[sfxx->id];
			sfxx->index += 4;
			unsigned char cmd = s->sample[sfxx->index];

			if (cmd == CMD_STOP)
				sfxx->id = 0;

			else {

				if (cmd == CMD_LOOP) {
					// FLASH(0x94, 4);
					sfxx->index = 0;
				}

				sfxx->delay = s->sample[sfxx->index + 3];
			}
		}
	}

	for (int channel = 0; channel < 2; channel++) {

		unsigned char audC = 0;
		unsigned char audV = 0;
		unsigned char audF = 0;

		struct Audio *best = 0;
		for (int i = 0; i < CONCURRENT_SFX; i++) {
			if (sfx[i].id && !sfx[i].handled)
				if (!best || AudioSamples[best->id].priority <
								 AudioSamples[sfx[i].id].priority)
					best = &sfx[i];
		}

		if (best) {

			best->handled = true;

			audC = AudioSamples[best->id].sample[best->index];
			audF = AudioSamples[best->id].sample[(best->index) + 1];
			audV = (AudioSamples[best->id].sample[(best->index) + 2] *
					((int)best->attenuation + 1)) >>
				   8;

			switch (best->id) {
			// case SFX_MAGIC2:
			case SFX_MAGIC:
				audF = getRandom32() & 0xF;
				audV = 2;
				break;

				// case SFX_X: {
				// 	audF = getRandom32() & 0xF;
				// 	break;
				// }
			}
		}

		// #if ENABLE_SOUND

		if (audC && audV >= volume[channel]) {
			RAM[_AUDC0 + channel] = audC;
			RAM[_AUDF0 + channel] = audF;
			RAM[_AUDV0 + channel] = audV;

			// sound_volume -= 5;
			// if (sound_volume < 0)
			// 	sound_volume = 0;
		}
		// #endif
	}
}

void playAudio() {

	volume[0] = volume[1] = 0;

	processMusic();
	processSoundEffects();
}

#if 1

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define X(audc, audf) ((unsigned char)(((audc) << 6) | audf))
#define DUR(note) X(note, 0)

#define FULLNOTE 0b01000000,
#define HALFNOTE 0b10000000,
#define QUARTERNOTE 0b11000000,

#define C1 (0)
#define C4 (1)
#define C6 (2)
#define C12 (3)

#define SNARE X(4, 17)

#define a1_SHARP X(C6, 17),
#define a2_SHARP X(C1, 17),
#define a3 X(C12, 23),
#define a3_SHARP X(C1, 8),
#define a4 X(C12, 11),
#define a4_SHARP X(C12, 10),
#define b2 X(C6, 7),
#define b3 X(C6, 3),
#define c2 X(C1, 31),
#define c2_SHARP X(C1, 29),
#define c3 X(C1, 15),
#define c3_SHARP X(C1, 14),
#define c4 X(C12, 19),
#define c4_SHARP X(C12, 18),
#define c5 X(C4, 29),
#define d2_SHARP X(C6, 12),
#define d3 X(C1, 12),
#define d3_SHARP X(C1, 12),
#define d4_SHARP X(C12, 17),
#define d5 X(C4, 26),
#define d5_SHARP X(C4, 25),
#define e3 X(C12, 31),
#define e4 X(C12, 15),
#define f2 X(C1, 23),
#define f3 X(C12, 29),
#define f4 X(C12, 14),
#define f5 X(C4, 21),
#define g3 X(C12, 26),
#define g3_SHARP X(C12, 24),
#define g4 X(C12, 12),
#define g4_SHARP X(C1, 4),

#define b4 X(C4, 31),
#define c5_SHARP X(C4, 29),
#define g5_SHARP X(C4, 18),
#define f5_SHARP X(C4, 20),
#define d4 X(C12, 17),
#define b5 X(C4, 15),

#define e5 X(C4, 23),
#define g5 X(C4, 19),
#define a5 X(C4, 17),
#define c6_SHARP X(12, 4),
#define d6 X(C4, 12),
#define f4_SHARP X(12, 13),
#define e6 X(C4, 11),
#define f6_SHARP X(C4, 10),
#define g6 X(C4, 9),
#define a6 X(C4, 8),
#define b6 X(C4, 7),
#define c7 X(C1, 0),
#define f6 X(C4, 10),
#define c6 X(C4, 14),
#define f6_SHARP X(C4, 10),
#define d6_SHARP X(C4, 12),
#define e7 X(C4, 5),

#pragma GCC diagnostic pop

// clang-format off


const unsigned char trackTrophy1[] = {

    HALFNOTE
    c5  e5  g5
    d5  f5  a5
    e5  g5  b5
    g5  a5  c6

    b5  g5  e5
    a5  f5  d5
    g5  e5  c5
    c5  g4  c5

    FULLNOTE c5 c5 c5 c5
    TRACK_END
};


const unsigned char trackTrophy2[] = {

    FULLNOTE

    c4  g3 
    d4  a3 
    e4  b3 
    g4  c4 
    e4  c4 
    d4  a3 
    c4  g3 
    c4  c4 

    TRACK_END
};



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


const unsigned char trackGridLockMelodyIntro[] = {
    FULLNOTE
    c3 e3 g3 f3 a3 c4 g3 b3 d4 c3 e3 g3 c3 e3 g3 f3 a3 c4 g3 b3 d4 c3 e3 g3 f3 a3 c4 g3 b3 d4 g3 b3
    d4 a3 c4 e4 b3 d4 f4 c3 e3 g3 c3 e3 g3 c3 e3 g3 f3 a3 c4 g3 b3 d4 c3 e3 g3 f3 a3 c4 g3 b3 d4

    TRACK_LOOP
};

const unsigned char trackGridLockBase[] = {
    FULLNOTE
    c2 c2 g3 g3 a2_SHARP a2_SHARP f2 f2 d2_SHARP d2_SHARP g3 g3 c2 c2 c2 c2 c2 c2 g3 g3 a2_SHARP
    a2_SHARP f2 f2 d2_SHARP d2_SHARP g3 g3 c2 c2 c2 c2 f2 f2 c3 c3 g3 g3 d3 d3 a2_SHARP a2_SHARP
    e3 e3 c2 c2 c2 c2 g3 g3 d3 d3 a2_SHARP a2_SHARP e3 e3 b2 b2 f3 f3 c2 c2 c2 c2 c2 c2 g3
    g3 a2_SHARP a2_SHARP f2 f2 d2_SHARP d2_SHARP g3 g3 c2 c2 c2 c2 f2 f2 c3 c3 g3 g3 d3
    d3 a2_SHARP a2_SHARP e3 e3 c2 c2 c2 c2

    TRACK_LOOP
};


// Instrument envelopes have 16 bytes defining the volume multipliers for each "audio tick"

static const unsigned char adsr_Trombone[] = {

    0, 100, 200, 250,
    200, 160, 160, 160,
    160,160,160,160,
    80,40,0,0,

};

static const unsigned char adsr_Rage[] = {

    100, 200, 255, 255, 255, 255, 255, 255, 255, 255, 125, 75, 50, 25, 0, 0};


static const unsigned char *const instrument[] = {
    adsr_Trombone,
    adsr_Rage,
};

// clang-format on

void loadTrack(int priority, const unsigned char *tune, int volume, int dur,
			   int instrument) {

	int best = -1;

	for (int slot = 0; slot < MUSIC_MAX; slot++) {

		if (!music[slot].tune) {
			best = slot;
			break;
		}

		if (best < 0 || music[slot].priority < music[best].priority)
			best = slot;
	}

	music[best].priority = priority;
	music[best].tune = tune;
	music[best].index = -1;
	music[best].progress = TRIGGER_NEXT_NOTE;
	music[best].instrument = instrument;
	music[best].volume = volume;
	music[best].noteDurationMultiplier =
		dur; // 0x80 = single note, 0x40 = half-note, etc
	music[best].baseSpeed = dur;
}

// // clang-format on

const int multiplier[] = {0, 0x100, 0x200, 0x400};
const unsigned char RENOTE[] = {1, 4, 6, 12};

void processMusic() {

	if (sound_volume < sound_max_volume)
		sound_volume += 2;

	else {
		sound_volume -= 3;
		if (sound_volume < 0)
			sound_volume = 0;
	}

	for (int i = 0; i < MUSIC_MAX; i++) {

		struct trackInfo *track = &music[i];
		track->processed = false;

		if (track->tune && track->progress >= TRIGGER_NEXT_NOTE) {

			track->progress -= TRIGGER_NEXT_NOTE;

			bool done;

			do {

				done = true;
				unsigned char nextNote = track->tune[++track->index];

				if (nextNote == TRACK_END)
					track->tune = 0;

				else {

					if (nextNote == TRACK_LOOP) {
						track->index = -1;
						done = false;
					}

					else if (nextNote == TRACK_VOLUME) {
						track->volume = track->tune[++track->index];
						done = false;
					}

					else if (!(nextNote & 0b00111111)) {
						track->noteDurationMultiplier =
							(track->baseSpeed * multiplier[nextNote >> 6]) >> 8;
						done = false;
					}
				}

			} while (!done);
		}
	}

	for (int channel = 0; channel < 2; channel++) {

		RAM[_AUDV0 + channel] = 0;
		RAM[_AUDF0 + channel] = 0;
		RAM[_AUDC0 + channel] = 0;
	}

	for (int channel = 0; channel < 2; channel++) {

		struct trackInfo *best = 0;

		for (int i = 0; i < MUSIC_MAX; i++) {
			struct trackInfo *track = &music[i];
			if (track->tune && !track->processed)
				if (!best || best->priority < track->priority)
					best = track;
		}

		if (best) {

			best->processed = true;

			unsigned char note = best->tune[best->index];
			int envelope_ptr = (best->progress) >> 12;

			RAM[_AUDV0 + channel] = volume[channel] =
				(instrument[best->instrument][envelope_ptr] * sound_volume *
				 best->volume) >>
				(4 + 10 + 8);

			// if (note == 0xFF)
			//     RAM[_BUF_AUDV + channel] = 0;

			RAM[_AUDF0 + channel] = note;
			RAM[_AUDC0 + channel] = RENOTE[note >> 6];
		}

		else
			break;
	}

	for (int i = 0; i < MUSIC_MAX; i++)
		if (music[i].tune)
			music[i].progress +=
				(8273 * music[i].noteDurationMultiplier) >> 8; // WTF BOO?!
}

#endif

// EOF