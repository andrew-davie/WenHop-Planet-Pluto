#pragma once

#include <stdbool.h>

// #include "main.h"

#define AUDIO_LOCKED 0x80
#define AUDIO_SINGLETON 0x40
#define AUDIO_KILL 0x20
#define AUDIO_ATTENUATE 16

#define ADDAUDIO(id) audioRequest[id] = true

struct AudioTable {
    const unsigned char *sample;
    const unsigned char priority;
    unsigned char flags;    // locked;
};

enum AudioCommand {
    CMD_LOOP = 254,
    CMD_STOP = 255,
};

enum AudioID {

    // Ordered by PRIORITY; see AudioSamples[]

    SFX_NULL,             // 00
    SFX_UNCOVERED,        // 01
    SFX_COUNTDOWN2,       // 02  time expiring
    SFX_PICKAXE,          // 03
    SFX_DOGE2,            // 04
    SFX_EXPLODE,          // 05
    SFX_WHOOSH,           // 06
    SFX_BLIP,             // 07
    SFX_EXTRA,            // 08
    SFX_EXIT,             // 09
    SFX_ZAP,              // 10
    SFX_ZAP2,             // 11
    SFX_EXPLODE_QUIET,    // 12
    SFX_MAGIC,            // 13
    SFX_MAGIC2,           // 14
    SFX_ROCK,             // 15
    SFX_ROCK2,            // 16
    SFX_SCORE,            // 17
    SFX_DOGE,             // 18
    SFX_DOGE3,            // 19
    SFX_DIRT,             // 20
    SFX_PUSH,             // 21
    SFX_SPACE,            // 22
    SFX_DRIP,             // 23
    SFX_BUBBLER,          // 24
    SFX_DRIP2,            // 25
    SFX_UNCOVER,          // 26
    SFX_KEYPRESS,         // 27

#if _ENABLE_LAVA2
    SFX_LAVA,    // 25
#endif

    SFX_MAX,    // size only; not a sound
};


extern bool audioRequest[SFX_MAX];

struct Audio {
    unsigned char id;
    unsigned char delay;
    bool handled;
    unsigned int index;
    unsigned char attenuation;    // 255 = 1.0 - lesser reduces appropriately
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
void loadTrack(int priority, const unsigned char *tune, int volume, int dur, int instrument);
void startMusic();
void startFanfare();
void startTrophyJingle();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"

#define X(audc, audf) ((unsigned char)(((audc) << 6) | audf))
#define DUR(note) X(note, 0)

#pragma GCC diagnostic pop

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

#define TRIGGER_NEXT_NOTE 0x10000
// #define NULL_TRACK 0xFF
#define TRACK_END 1
#define TRACK_LOOP 0
#define TRACK_VOLUME 2

// EOF