#pragma once

#include "main.h"

#define AUDIO_LOCKED 0x80
#define AUDIO_SINGLETON 0x40
#define AUDIO_KILL 0x20
#define AUDIO_ATTENUATE 16

#define ADDAUDIO(id) audioRequest[id] = true

struct AudioTable {
	const unsigned char *sample;
	const unsigned char priority;
	unsigned char flags; // locked;
};

enum AudioCommand {
	CMD_LOOP = 254,
	CMD_STOP = 255,
};

enum AudioID {

	// Ordered by priority; see AudioSamples[]

	SFX_NULL,		// 00
	SFX_ABORT,		// 01
	SFX_WHOOSH,		// 02
	SFX_BLIP,		// 03
	SFX_REV,		// 04 car revving and fading away
	SFX_SELECTION,	// 05 short selection blip
	SFX_BEEP2,		// 06 beep horn twice
	SFX_FASTBEEP2,	// 07 beep horn twice (fast)
	SFX_EXIT,		// 08
	SFX_SPINWHEELS, // 09 wheel squealing... sorta
	SFX_MAGIC,		// 10
	SFX_FIREWORKS,	// 11 muffled fireworks for title screen
	SFX_DIRT,		// 12
	SFX_SPACE,		// 13

	SFX_MAX, // size only; not a sound

};

extern bool audioRequest[SFX_MAX];

struct Audio {
	unsigned char id;
	unsigned char delay;
	bool handled;
	unsigned int index;
	unsigned char attenuation; // 255 = 1.0 - lesser reduces appropriately
};

#define CONCURRENT_SFX 4

struct trackInfo {

	bool processed;
	int priority;
	const unsigned char *tune;
	int index;
	int progress;
	unsigned char instrument;
	unsigned char frequency;
	int volume;
	int noteDurationMultiplier;
	int baseSpeed;
	// int speed;
};

#define MUSIC_MAX 4 /* max number of music tracks */
#define VOLUME_NONPLAYING 900
#define VOLUME_PLAYING 400
#define VOLUME_MAX 1024

extern struct trackInfo music[MUSIC_MAX];

extern int sound_volume;
extern int sound_max_volume;

void playAudio();
bool addAudio(enum AudioID id);
void killAudio(enum AudioID id);
void killRepeatingAudio();
void initAudio(bool killTracks);
void loadTrack(int priority, const unsigned char *tune, int volume, int dur,
			   int instrument);
void startMusic();
void startFanfare();
void startChampJingle();
void startTrophyJingle();

// EOF