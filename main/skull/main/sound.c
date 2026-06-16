// clang-format off

#include <stdbool.h>
#include "defines_cdfj.h"

#include "main.h"
#include "sound.h"

#include "random.h"

// clang-format on

void loadTrack(int priority, const unsigned char *tune, int volume, int dur,
               int instrument);
void processMusic();

const unsigned char trackSimple[];
static const unsigned char trackSimple2[];
#define TRIGGER_NEXT_NOTE 0x10000
// #define NULL_TRACK 0xFF
#define TRACK_END 1
#define TRACK_LOOP 0

int sound_volume = 128;
int sound_max_volume = 1024;
int volume[2];

struct Audio sfx[CONCURRENT_SFX];
struct trackInfo music[MUSIC_MAX];

// clang-format off

const unsigned char sampleTick[] = {
    0xF, 0x1F, 1, 2,
    0, 0, 0, 6,
    0xF, 0x1F, 1, 1,
    0, 0, 0, 40,
    CMD_LOOP,
};

const unsigned char sampleDrip[] = {
    12, 12, 4, 1,
    CMD_STOP,
};

const unsigned char sampleUncover[] = {
    12, 6, 3, 1,
    12, 4, 3, 2,
    0, 0, 0, 1,
    CMD_LOOP,
};

const unsigned char sampleDrip2[] = {
    12, 6, 4, 1,
    12, 4, 4, 1,
    CMD_STOP,
};

const unsigned char sampleMagic[] = {
    0xC, 0x8, 3, 2,
    CMD_LOOP,
};

const unsigned char sampleMagic2[] = {
    0x5, 0x8, 5, 15,
    CMD_STOP,
};

const unsigned char samplePick[] = {
    9,
    12,
    8,
    1,
    9,
    20,
    15,
    1,
    9,
    31,
    12,
    1,
    CMD_STOP,
};

const unsigned char sampleRock[] = {
    8, 18, 5, 4,
    8, 18, 5, 3,
    8, 18, 4, 2,
    8, 18, 3, 1,
    CMD_STOP,
};

const unsigned char sampleRock2[] = {
    8, 18, 4, 4,
    8, 18, 3, 3,
    8, 18, 2, 3,
    CMD_STOP,
};

const unsigned char sampleDirt[] = {
    8, 31, 3, 10,
    CMD_STOP,
};

const unsigned char sampleSpace[] = {
    8, 2, 2, 2,
    CMD_STOP,
};

const unsigned char sampleBlip[] = {
    4, 18, 5, 1,
    4, 18, 4, 2,
    4, 18, 3, 4,
    CMD_STOP,
};

const unsigned char sampleAmoeba[] = {
    0xC, 0x8, 1, 2, CMD_LOOP,
    // 12,10,2,1,
    // CMD_LOOP,
};


const unsigned char sampleBubbler[] = {
    0xE, 0x8, 1, 30,
    CMD_STOP,
};

const unsigned char sampleExxtra[] = {
    12, 5, 4, 10,
    12, 7, 4, 10,
    12, 5, 4, 10,
    12, 7, 4, 10,
    12, 5, 4, 10,
    12, 7, 4, 10,
    CMD_STOP,
};

const unsigned char sampleUncovered[] = {
    12, 5, 4, 3,
    12, 7, 4, 3,
    12, 5, 4, 3,
    12, 7, 4, 3,
    CMD_STOP,
};

const unsigned char samplePush[] = {
    8, 4, 4, 12,
    CMD_STOP,
};

const unsigned char sampleExplode[] = {
    8, 7, 9, 2,
    8, 10, 8, 2,
    8, 13, 7, 2,
    8, 16, 7, 3,
    8, 19, 6, 4,
    8, 22, 5, 5,
    8, 25, 4, 6,
    8, 28, 3, 10,
    8, 29, 2, 15,
    8, 31, 1, 15,
    CMD_STOP,
};

const unsigned char sampleExplodeQuiet[] = {
    8, 7, 7, 2,
    8, 13, 5, 2,
    8, 19, 4, 4,
    8, 25, 2, 6,
    8, 29, 1, 15,
    CMD_STOP,
};

const unsigned char sampleWhoosh[] = {
    15, 31, 1, 2,
    15, 31, 2, 2,
    15, 28, 3, 2,
    15, 25, 4, 3,
    15, 22, 5, 4,
    15, 19, 6, 5,
    15, 16, 7, 6,
    15, 13, 7, 10,
    15, 10, 4, 2,
    15, 7, 2, 2,
    CMD_STOP,
};


const unsigned char sample10987654321[] = {

    4, 10, 1, 1,
    4, 10, 3, 1,
    4, 10, 5, 1,
    4, 10, 6, 1,
    4, 10, 5, 15,
    4, 10, 3, 3,
    4, 10, 2, 3,
    4, 10, 1, 3,
    CMD_STOP,
};

const unsigned char sampleNone[] = {
    CMD_STOP,
};

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

const unsigned char sampleDiamond2[] = {
    // C,F,V,LEN
    5, 14, 1, 1, 5, 12, 2, 1, 5, 11, 3, 1, 5, 3, 4, 2, 5, 3,        4,
    2, 5,  4, 3, 1, 5,  5, 1, 1, 5,  4, 3, 1, 5, 5, 1, 1, CMD_STOP,
};


const unsigned char sampleDiamond3[] = {
    // C,F,V,LEN
    5, 14, 1, 1, 5, 11, 3, 1, 5, 3, 4, 1, 5, 4, 3, 1, 5, 5, 1, 1, CMD_STOP,
};

const unsigned char sampleExit[] = {
    12, 16, 1, 1,
    12, 16, 4, 1,
    12, 16, 10, 1,
    12, 16, 8, 4,
    12, 16, 6, 1,
    12, 16, 4, 1,
    12, 16, 2, 1,
    CMD_STOP,
};

// const unsigned char sampleBirth[] = {

// //    8,4,1,1,
// //    8,4,4,1,
// //    8,4,8,1,
//     8,4,14,8,
//     8,4,10,4,
//     8,4,8,4,
//     8,4,6,4,
//     8,4,4,4,
//     8,4,2,4,
//     CMD_STOP,
// };

// clang-format on

const struct AudioTable AudioSamples[] = {

    {sampleNone, 0, 0}, // 0  PLACEHOLDER - NOT USED AS SOUND

    // MUST correspond to AudioID enum ordering/number
    // MUST be in priority order!

    {sampleUncovered, 201, 0},              // SFX_UNCOVERED,          // 0
    {sample10987654321, 200, AUDIO_LOCKED}, // SFX_COUNTDOWN2,         // 1
    {sampleSFX, 200, 0},                    // SFX_DIAMOND2,           // 2
    {sampleWhoosh, 127, 0},                 // SFX_WHOOSH,             // 3
    {sampleBlip, 125, 0},                   // SFX_BLIP,               // 4
    {sampleExxtra, 110, 0},                 // SFX_EXTRA,              // 5
    {sampleExit, 99, 0},                    // SFX_EXIT,               // 6
    {sampleExplode, 99, 0},                 // SFX_EXPLODE,            // 7
    {sampleMagic, 50, AUDIO_KILL},          // SFX_MAGIC,              // 8
    {sampleMagic2, 50, 0},                  // SFX_MAGIC2,             // 9
    {sampleRock, 11, 0},                    // SFX_ROCK,               // 10
    {sampleRock2, 10, 0},                   // SFX_ROCK2,              // 11
    {sampleDrip2, 10, 0},                   // SFX_SCORE,              // 12
    {sampleDiamond2, 9, 0},                 // SFX_DIAMOND,            // 13
    {sampleDiamond3, 9, 0},                 // SFX_DIAMOND3,           // 14
    {sampleDirt, 9, 0},                     // SFX_DIRT,               // 15
    {samplePush, 8, 0},                     // SFX_PUSH,               // 16
    {sampleSpace, 8, 0},                    // SFX_SPACE,              // 17
    {sampleDrip2, 8, 0},                    // SFX_DRIP,               // 18
    {sampleAmoeba, 7,
     AUDIO_LOCKED | AUDIO_SINGLETON | AUDIO_KILL}, // SFX_AMOEBA, // 18
    {sampleDrip, 5, 0}, // SFX_DRIP2,              // 19
    {sampleUncover, 2,
     AUDIO_LOCKED | AUDIO_KILL}, // SFX_UNCOVER,            // 20
    // { sampleTick,           0, AUDIO_LOCKED | AUDIO_KILL          }, //
    // SFX_TICK, // 22

};

bool audioRequest[SFX_MAX];

void killRepeatingAudio() {

  for (int channel = 0; channel < 2; channel++) {
    RAM[_BUF_AUDF + channel] = 0;
    RAM[_BUF_AUDC + channel] = 0;
    RAM[_BUF_AUDV + channel] = 0;
  }

  for (int i = 0; i < CONCURRENT_SFX; i++)
    if (AudioSamples[sfx[i].id].flags & AUDIO_KILL)
      sfx[i].id = 0;

  for (int i = 0; i < SFX_MAX; i++)
    if (AudioSamples[i].flags & AUDIO_KILL)
      audioRequest[i] = false;
}

void initAudio() {

  killRepeatingAudio();

  for (int i = 0; i < MUSIC_MAX; i++)
    music[i].tune = 0;

  sound_max_volume = VOLUME_MAX;
}

void startMusic() {

  // #if __ENABLE_ATARIVOX
  // RAM[_BUF_SPEECH] = 0xFF;
  // #endif

  // for (int i = 0; i < MAX_TRACKS; i++)
  //     sfx[i].id = 0;

  // RAM[_BUF_AUDV] =
  // RAM[_BUF_AUDV + 1] = 0;

  loadTrack(0, trackSimple2, 150, 0x100, 1);
  loadTrack(10, trackSimple, 150, 0x100, 0);
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

        if ((!(AudioSamples[idx].flags & AUDIO_LOCKED) && // not locked, and...
             (lowest < 0 || // either we haven't found a lowest yet
              AudioSamples[idx].priority <
                  AudioSamples[sfx[lowest].id].priority)
             // or the priority of this track is lower than the lowest found so
             // far...
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

        audioRequest[id] = false;
      }

      else
        break; // sounds full or higher priority, ignore any more lower priority
               // sounds

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
        if (cmd == CMD_LOOP)
          sfxx->index = 0;

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
        if (!best ||
            AudioSamples[best->id].priority < AudioSamples[sfx[i].id].priority)
          best = &sfx[i];
    }

    if (best) {

      best->handled = true;

      audC = AudioSamples[best->id].sample[best->index];
      audF = AudioSamples[best->id].sample[(best->index) + 1];
      audV = AudioSamples[best->id].sample[(best->index) + 2];

      switch (best->id) {

      case SFX_MAGIC2:
      case SFX_MAGIC:
        audF = getRandom32() & 0xF;
        break;

      case SFX_AMOEBA: {
        // static unsigned char amoebaF;
        // if (!best->index && best->delay == AudioSamples[best->id].sample[3])
        //     amoebaF = (getRandom32() & 0xF) | 8;
        // audF = amoebaF;
        audF = getRandom32() & 0xF;
        break;
      }

        // case SFX_DEADBEAT: {

        //         static int f;
        //         if (!best->index && best->delay ==
        //         AudioSamples[best->id].sample[3] - 1)
        //         {

        //             tuneIndex++;
        //             if (beat2[tuneIndex] == 0)
        //                 tuneIndex = 0;

        //             f = getRandom32();
        //         }

        //         audF = f; //getRandom32(); //beat2[tuneIndex];
        //     }
        //     break;

        // case SFX_DEADBEAT2: {

        //         if (!best->index && best->delay ==
        //         AudioSamples[best->id].sample[3] - 1)
        //         {

        //             tuneIndex++;
        //             if (beat[tuneIndex] == 0)
        //                 tuneIndex = 0;
        //         }

        //         audF = beat[tuneIndex];
        //     }
        //     break;
      }
    }

#if ENABLE_SOUND

    if (audC && audV >= volume[channel]) {
      RAM[_BUF_AUDC + channel] = audC;
      RAM[_BUF_AUDF + channel] = audF;
      RAM[_BUF_AUDV + channel] = audV;

      sound_volume -= 5;
      if (sound_volume < 0)
        sound_volume = 0;
    }
#endif
  }
}

void playAudio() {
  return; // tmp

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

#define e5 X(4, 23),
#define g5 X(4, 19),
#define a5 X(4, 17),
#define c6_SHARP X(12, 4),
#define d6 X(4, 12),
#define f4_SHARP X(12, 13),
#define e6 X(4, 11),
#define f6_SHARP X(4, 10),
#define g6 X(4, 9),
#define a6 X(4, 8),
#define b6 X(4, 7),
#define c7 X(1, 0),
#define f6 X(4, 10),
#define c6 X(4, 14),
#define f6_SHARP X(4, 10),
#define d6_SHARP X(4, 12),
#define e7 X(4, 5),

#pragma GCC diagnostic pop

// ref: http://www.retrointernals.org/ATT_ROCK-dash/music.html
// ref:
// https://forums.atariage.com/topic/176497-atari-2600-frequency-and-tuning-chart-new-v11/#comment-2198932

// clang-format off


/*
Channel 1 (Melody):
------------------------------------------
| e4  f#4  g#4 | a4  b4  c#4 | d4  c#4  b4 | a4 |
| e4  f#4  g#4 | a4  b4  c#4 | d4  e4  d4  | c#4 |
------------------------------------------

Channel 2 (Harmony):
------------------------------------------
| b3  c#4  d4  | e4  f#4  g#4 | a4  g#4  f#4 | e4 |
| b3  c#4  d4  | e4  f#4  g#4 | a4  b4  a4  | g#4 |
------------------------------------------*/


// static const unsigned char trackBombe[] = {
//     d4 c4 d4 b2
//     TRACK_END
// };

// static const unsigned char trackAccompany[] = {

//     // a2_SHARP a2_SHARP a2_SHARP a2_SHARP
//     // a3 a3 a4_SHARP a4_SHARP

//     // d4 a4 d3 d2_SHARP

//     FULLNOTE
//     e3 f3 g3 f3
//     e3 f3 g3 f3
//     e3 f3 g3 f3
//     e3 f3 g3 f3

//     // e3 f3 f3 a3_SHARP
//     // g3 g4 g4 g4

//     // a2_SHARP a2_SHARP a2_SHARP a3
//     // a3_SHARP a3_SHARP a3_SHARP a3

//     HALFNOTE
//     c4 d4 e4 f4
//     d4 e4 f4 g4
//     c4 d4 e4 f4
//     d4 e4 f4 g4


//     // e4 g4
//     // e4 g4
//     // e4 g4
//     // e4 g4

//     //   e3 g3
//     //   e3 g3
//     //   e3 g3

//     //    f4 f4 f4 f4
//     //    g4 g4 g3 g3


// //  e4  g4  a4  b4  b4  a4  g4  f4 e4  g4  a4  b4  b4  a4  g4  f4
// //  e4  g4  a4  b4  b4  a4  g4  f4 e4  g4  a4  b4  b4  a4  g4  f4
// //  d4  e4  f4  g4  g4  f4  e4  d4  d4  e4  f4  g4  g4  f4  e4  d4
// //    a3 b3 c3 d3 c3 b3 d3 a3 g6 g6 g6 a6 a6 a6
// // e4 d4 e4 a3 d4 e4 d4 c4 d4 e4 a3 g4
// // e4 d4 e4 a3 d4 e4 d4 c4 d4 e4 a3 g4
// // e4 d4 e4 g4 f4 e4 d4 e4 a3 d4 e4
// // e4 d4 e4 g4 f4 e4 d4 e4 a3 d4 e4
// // c4 d4 e4 a3 d4 e4 d4 c4 d4 e4 a3 g4
// // e4 d4 e4 a3 d4 e4 d4 c4 d4 e4 a3 g4
// // e4 d4 e4 g4 f4 e4 d4 e4 a3 d4 e4
// // e4 d4 e4 g4 f4 e4 d4 e4 a3 d4 e4
// // g3 e3 c3 d3 g3 e3 c3 d3
// // a3 d3 g3 g3 g3 e3 c3 d3

// // g3 e3 c3 d3 g3 e3 c3 d3
// // a3 d3 g3 g3 g3 e3 c3 d3

// //  0xFF,
// //  0xFF,
// //  0xFF,
// //  0xFF,

// //  0xFF,
// //  0xFF,
// //  0xFF,
// //  0xFF,


//     TRACK_LOOP
// };

// clang-format off

const unsigned char trackRage[] = {

    d4 d4 d4 d4
    d5 d6 e4 e4
    d4_SHARP d4_SHARP d4_SHARP a2_SHARP
    TRACK_END
};

const unsigned char trackSimple[] = {

    // Boulder Dash theme
    // HALFNOTE

    f3 a3 c4 f4
    g3 a3_SHARP c4 g4
    c4_SHARP d4_SHARP f4 g4_SHARP
    d4_SHARP d5 e4 c5
    f3 f4 c3 g3
    d3_SHARP g4 g3 d3_SHARP
    f3 f4 c3 g3
    c4_SHARP f5 f4 c4_SHARP
    d3_SHARP d4_SHARP a2_SHARP f3
    b3 d5_SHARP d4_SHARP b3
    c3 e4 d3 f4
    a3_SHARP a3_SHARP a4_SHARP a3_SHARP
    f4 f4 f4 f4
    f4 f4 f4 f4
    f4 f4 f4 f4
    f4 f4 f4 f4
    f4 f4 f4 f4
    f4 f4 f4 f4
    f4 f4 f4 f4
    d4_SHARP d4_SHARP d4_SHARP d4_SHARP
    f4 f5 f4 d5_SHARP
    f4 d5 f4 c5
    d4_SHARP d5_SHARP d4_SHARP d5_SHARP
    d4_SHARP a4_SHARP d4_SHARP d5_SHARP
    f4 f4 f4 f4
    f4 f4 f4 f4
    f4 f4 f4 f4
    d4_SHARP d4_SHARP d4_SHARP d4_SHARP
    a4 f4 c4 a3
    g4 d4_SHARP a3_SHARP d3_SHARP
    a4 f4 c4 a3
    g4 d4_SHARP a3_SHARP d3_SHARP


    TRACK_LOOP
    };

static const unsigned char trackSimple2[] = {

    //    HALFNOTE

    f2 c3 f3 g3_SHARP d2_SHARP d3 d3_SHARP a3_SHARP c2_SHARP c2_SHARP c3_SHARP c2_SHARP d3_SHARP
        a4_SHARP e3 g4_SHARP f2 f2 f2 f2 d2_SHARP d2_SHARP d2_SHARP d2_SHARP f2 f2 f2 f2 c3_SHARP
            c3_SHARP c3_SHARP c3_SHARP d2_SHARP d2_SHARP d2_SHARP d2_SHARP b2 b2 b2 b2 c2 c4 c2 c4
                a1_SHARP a1_SHARP f2 f2 f2 f2 f2 f2 f3 f3 f2 f2 d2_SHARP d2_SHARP d2_SHARP d2_SHARP
                    d3_SHARP d3_SHARP d2_SHARP d2_SHARP f2 a4 f2 a4_SHARP f3 a4 f2 a4_SHARP d2_SHARP
                        a4 d2_SHARP a4_SHARP d3_SHARP g4 d2_SHARP g4_SHARP f2 f2 f2 c5 f3 f3 f2
                            g4_SHARP d2_SHARP d2_SHARP d2_SHARP d2_SHARP d3_SHARP d3_SHARP d2_SHARP
                                d2_SHARP f2 a4 f2 a4_SHARP f3 a4 f2 a4_SHARP d2_SHARP a4 d2_SHARP
                                    a4_SHARP d3_SHARP g4 d2_SHARP g4_SHARP f4 c4 a3 f3 d4_SHARP
                                        a3_SHARP g3 d2_SHARP c5 a4 f4 c4 a3_SHARP g3 d3_SHARP
                                            d2_SHARP TRACK_LOOP};

// Instrument envelopes have 16 bytes defining the volume multipliers for each "audio tick"

static const unsigned char adsr_Trombone[] = {
    // 200, 180, 160, 140, 96, 95, 94, 93,
    //                                           92,  91,  80,  60,  40, 20, 0,  0

    255, 240, 240, 220, 200, 180, 160, 140,
                                              120,  100,  80,  60,  40, 20, 0,  0

                                              };
static const unsigned char adsr_Rage[] = {

    255, 255, 255, 100, 100, 100, 255, 255,
                                              255,  100,  100,  100,  0, 0, 0,  0

                                              };

static const unsigned char adsr_Trombone2[] = {
    //    100, 200, 255, 255, 200, 150, 75, 75, 100, 100, 100, 100, 75, 75, 75, 75,
    // 10, 20, 25, 25, 20, 15, 7, 7, 10, 10, 10, 10, 7, 7, 7, 7,
    180, 200, 220, 240, 220, 200, 180, 160, 160, 160, 160, 120, 80, 40, 0, 0};

// // static const unsigned char adsr_2[] = {
// //     80, 80, 60, 250, 250, 160, 50, 40, 10, 0, 0, 0, 0, 0, 0, 0
// // };

static const unsigned char *const instrument[] = {
    adsr_Trombone,
    adsr_Trombone2,
    adsr_Rage,
};

// clang-format on

void loadTrack(int priority, const unsigned char *tune, int volume, int dur,
               int instrument) {

  int slot = 0;
  while (music[slot].tune)
    if (++slot >= MUSIC_MAX)
      return;

  // for (int i = 0; i < MUSIC_MAX; i++)
  //     if (!music[i].tune) {
  //         slot = i;
  //         break;
  //     }

  // if (slot < 0) {
  //     // no free tune slots available so grab lowest non-0 priority one
  //     // 0-priority are the repeating BG tune
  //     for (int i = 0; i < MUSIC_MAX; i++)
  //         if (!music[i].priority)
  //             if (slot < 0 || music[i].priority < music[slot].priority)
  //                 slot = i;
  // }

  //    if (slot >= 0) {

  music[slot].priority = priority;
  music[slot].tune = tune;
  music[slot].index = -1;
  music[slot].progress = TRIGGER_NEXT_NOTE;
  music[slot].instrument = instrument;
  music[slot].volume = volume;
  music[slot].noteDurationMultiplier =
      dur; // 0x80 = single note, 0x40 = half-note, etc
}

// static const int systemSpeed[] = {

//     0x11500 * BASE_SPEED / 60,
//     0x11500 * 7 / 60, // 50
//     0x11500 * 7 / 60,
//     0x11500 * 7 / 60, // 50
// };

static const unsigned char renote[] = {1, 4, 6, 12};

// clang-format on

const int multiplier[] = {0, 0x100, 0x200, 0x400};

void processMusic() {

  if (sound_volume < sound_max_volume)
    sound_volume += 2;

  else
    sound_volume -= 3;

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

          else if (!(nextNote & 0b00111111)) {
            track->noteDurationMultiplier = multiplier[nextNote >> 6];
            done = false;
          }
        }

      } while (!done);
    }
  }

  for (int channel = 0; channel < 2; channel++) {

    RAM[_BUF_AUDV + channel] = 0;
    RAM[_BUF_AUDF + channel] = 0;
    RAM[_BUF_AUDC + channel] = 0;
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

      RAM[_BUF_AUDV + channel] = volume[channel] =
          (instrument[best->instrument][envelope_ptr] * sound_volume *
           best->volume) >>
          (4 + 10 + 8);

      // if (note == 0xFF)
      //     RAM[_BUF_AUDV + channel] = 0;

      RAM[_BUF_AUDF + channel] = note;
      RAM[_BUF_AUDC + channel] = renote[note >> 6];
    }

    else
      break;
  }

  for (int i = 0; i < MUSIC_MAX; i++)
    if (music[i].tune)
      music[i].progress += (8273 * music[i].noteDurationMultiplier) >> 8;
}

#endif

// EOF