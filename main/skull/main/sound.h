#include "main.h"

#define AUDIO_LOCKED 0x80
#define AUDIO_SINGLETON 0x40
#define AUDIO_KILL 0x20

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

    SFX_NULL, // 0

    SFX_UNCOVERED,  // 1
    SFX_COUNTDOWN2, // 2   time expiring
    SFX_DIAMOND2,   // 3
    SFX_WHOOSH,     // 5
    SFX_BLIP,       // 6
    SFX_EXTRA,      // 7
    SFX_EXIT,       // 8
    SFX_EXPLODE,    // 9
    SFX_MAGIC,      // 10
    SFX_MAGIC2,     // 10
    SFX_ROCK,       // 11
    SFX_ROCK2,      // 12
    SFX_SCORE,      // 13
    SFX_DIAMOND,    // 14
    SFX_DIAMOND3,   // 15
    SFX_DIRT,       // 16
    SFX_PUSH,       // 17
    SFX_SPACE,      // 18
    SFX_DRIP,       // 19
    SFX_AMOEBA,     // 20
    SFX_DRIP2,      // 21
    SFX_UNCOVER,    // 22
                    // SFX_TICK,               // 24

    SFX_MAX, // size only; not a sound
};

extern bool audioRequest[SFX_MAX];

struct Audio {
    unsigned char id;
    unsigned char delay;
    bool handled;
    unsigned int index;
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
    // int speed;
};

#define MUSIC_MAX 10 /* max number of music tracks */
#define VOLUME_NONPLAYING 600
#define VOLUME_PLAYING 600
#define VOLUME_MAX 1024

extern struct trackInfo music[MUSIC_MAX];

extern int sound_volume;
extern int sound_max_volume;

void playAudio();
bool addAudio(enum AudioID id);
void killAudio(enum AudioID id);
void killRepeatingAudio();
void initAudio();
void loadTrack(int priority, const unsigned char *tune, int volume, int dur, int instrument);
void startMusic();

// EOF