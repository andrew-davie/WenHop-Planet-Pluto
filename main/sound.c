#include "defines_dasm.h"

#include "cdfjplus.h"

#include <stdbool.h>

#include "random.h"
#include "scroll.h"
#include "sound.h"

#define ENABLE_SOUND 1

// format of sounds:     // AUIDC, AUDF, AUDV, DUR


void loadTrack(int priority, const unsigned char *tune, int volume, int dur, int instrument);
void processMusic();

const unsigned char trackGridLockBase2[];
const unsigned char trackGridLockMelodyIntro[];
const unsigned char trackGridLockBase[];
const unsigned char trackTrophy1[];
const unsigned char trackTrophy2[];

int sound_volume = 128;
int sound_max_volume = 768;
int volume[2];

struct Audio sfx[CONCURRENT_SFX];
struct trackInfo music[MUSIC_MAX];


const unsigned char sampleTick[] = {
    0xF, 0x1F, 1, 2, 0, 0, 0, 6, 0xF, 0x1F, 1, 1, 0, 0, 0, 40, CMD_LOOP,
};

const unsigned char sampleDrip[] = {
    12, 12, 4, 1, CMD_STOP,
};

const unsigned char sampleUncover[] = {
    12, 6, 3, 1, 12, 4, 3, 2, 0, 0, 0, 1, CMD_LOOP,
};

const unsigned char sampleDrip2[] = {
    12, 6, 4, 1, 12, 4, 4, 1, CMD_STOP,
};

const unsigned char sampleMagic[] = {
    0xC, 0x8, 3, 2, CMD_LOOP,
};

const unsigned char sampleMagic2[] = {
    0x5, 0x8, 5, 15, CMD_STOP,
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
    // 8, 18, 9, 1,
    // 8, 18, 6, 1,
    CMD_STOP,
};

const unsigned char sampleZap[] = {
    // clang-format off

    3, 11, 12, 14,
    3, 8, 12, 10,
    CMD_STOP,

    // clang-format on
};

const unsigned char sampleZap2[] = {
    // clang-format off

    0,0,0,3,
    3, 10, 3, 20,

    CMD_STOP,

    // clang-format on
};


const unsigned char sampleRock[] = {
    8, 18, 5, 4, 8, 18, 5, 3, 8, 18, 4, 2, 8, 18, 3, 1, CMD_STOP,
};

const unsigned char sampleRock2[] = {
    8, 18, 4, 4, 8, 18, 3, 3, 8, 18, 2, 3, CMD_STOP,
};

const unsigned char sampleDirt[] = {
    8, 31, 3, 10, CMD_STOP,
};

const unsigned char sampleSpace[] = {
    8, 2, 2, 2, CMD_STOP,
};

const unsigned char sampleBlip[] = {
    4, 18, 5, 1, 4, 18, 4, 2, 4, 18, 3, 4, CMD_STOP,
};

const unsigned char sampleBubbler[] = {
    0xE, 0x8, 1, 30, CMD_STOP,
    //    CMD_LOOP,
    // 12,10,2,1,
    // CMD_LOOP,
};

#if _ENABLE_LAVA2
const unsigned char sampleLava[] = {
    3, 10, 2, 2, CMD_LOOP,
};
#endif


const unsigned char sampleExxtra[] = {
    //clang-format off

    12, 5, 4, 10, 12, 7, 4, 10, 12, 5, 4, 10, 12, 7, 4, 10, 12, 5, 4, 10, 12, 7, 4, 10, CMD_STOP,

    // clang-format
};

const unsigned char sampleUncovered[] = {
    12, 5, 4, 3, 12, 7, 4, 3, 12, 5, 4, 3, 12, 7, 4, 3, CMD_STOP,
};

const unsigned char samplePush[] = {
    8, 4, 4, 12, CMD_STOP,
};

const unsigned char sampleKeypress[] = {
    // clang-format off

     9, 6, 5, 1,
    CMD_STOP,

    // clang-format on
};


const unsigned char sampleExplode[] = {
    // clang-format off

    8, 7, 14, 2,
    8, 10, 13, 2,
    8, 13, 12,  2,
    8, 16, 11,  3,
    8, 19, 10,  4,
    8, 22, 8, 5,
    8, 25, 6,  6,
    8, 28, 4,  10,
    8, 29, 2,  15,
    8, 31, 1,  15,
    CMD_STOP,
    // clang-format on
};

const unsigned char sampleExplodeQuiet[] = {
    8,
    7,
    7,
    2,
    //    8, 10, 8, 2,
    8,
    13,
    5,
    2,
    //   8, 16, 7, 3,
    8,
    19,
    4,
    4,
    //   8, 22, 5, 5,
    8,
    25,
    2,
    6,
    //   8, 28, 3,10,
    8,
    29,
    1,
    15,
    //  8, 31, 1, 15,
    CMD_STOP,
};

const unsigned char sampleWhoosh[] = {
    15, 31, 1, 2,  15, 31, 2, 2,  15, 28, 3,  2,  15, 25, 4, 3,  15, 22, 5, 4,        15,
    19, 6,  5, 15, 16, 7,  6, 15, 13, 7,  10, 15, 10, 4,  2, 15, 7,  2,  2, CMD_STOP,
};

const unsigned char sample10987654321[] = {

    4, 10, 1, 1, 4, 10, 3, 1, 4, 10, 5, 1, 4, 10, 6, 1, 4, 10, 5, 15, 4, 10, 3, 3, 4, 10, 2, 3, 4, 10, 1, 3, CMD_STOP,
};

const unsigned char sampleNone[] = {
    CMD_STOP,
};

const unsigned char sampleSFX[] = {
    // C,F,V,LEN
    5, 14, 4, 1, 5, 12, 6, 1, 5, 11, 7, 1, 5, 3, 10, 2, 5, 4, 8, 1, 5, 5, 6, 1, 5, 6, 4, 1, 5, 7, 1, 1, CMD_STOP,
};

const unsigned char sampleDoge2[] = {
    // C,F,V,LEN
    5, 14, 1, 1, 5, 12, 2, 1, 5, 11, 3, 1, 5, 3, 4, 2, 5, 3,        4,
    2, 5,  4, 3, 1, 5,  5, 1, 1, 5,  4, 3, 1, 5, 5, 1, 1, CMD_STOP,
};

const unsigned char sampleDoge3[] = {
    // C,F,V,LEN
    5, 14, 1, 1, 5, 11, 3, 1, 5, 3, 4, 1, 5, 4, 3, 1, 5, 5, 1, 1, CMD_STOP,
};

const unsigned char sampleExit[] = {
    12, 16, 1, 1, 12, 16, 4, 1, 12, 16, 10, 1, 12, 16, 8, 4, 12, 16, 6, 1, 12, 16, 4, 1, 12, 16, 2, 1, CMD_STOP,
};


const struct AudioTable AudioSamples[] = {

    // audio data, priority, flags

    {sampleNone, 0, 0},    // 0  PLACEHOLDER - NOT USED AS SOUND

    // MUST correspond to AudioID enum ordering/number
    // MUST be in priority order!

    //                                                                     priority order
    //                                                                     --------------
    {sampleUncovered, 201, 0},                                          // SFX_UNCOVERED
    {sample10987654321, 200, AUDIO_LOCKED},                             // SFX_COUNTDOWN2
    {samplePick, 200, 0},                                               // SFX_PICKAXE
    {sampleSFX, 200, 0},                                                // SFX_DOGE2
    {sampleExplode, 128, 0},                                            // SFX_EXPLODE
    {sampleWhoosh, 127, 0},                                             // SFX_WHOOSH
    {sampleBlip, 125, 0},                                               // SFX_BLIP
    {sampleExxtra, 110, 0},                                             // SFX_EXTRA
    {sampleExit, 99, 0},                                                // SFX_EXIT
    {sampleZap, 98, AUDIO_ATTENUATE},                                   // SFX_ZAP
    {sampleZap2, 98, 0},                                                // SFX_ZAP2
    {sampleExplodeQuiet, 97, 0},                                        // SFX_EXPLODE_QUIET
    {sampleMagic, 50, AUDIO_KILL},                                      // SFX_MAGIC
    {sampleMagic2, 50, 0},                                              // SFX_MAGIC2
    {sampleRock, 11, 0},                                                // SFX_ROCK
    {sampleRock2, 10, 0},                                               // SFX_ROCK2
    {sampleDrip2, 10, 0},                                               // SFX_SCORE
    {sampleDoge2, 9, 0},                                                // SFX_DOGE
    {sampleDoge3, 9, 0},                                                // SFX_DOGE3
    {sampleDirt, 9, 0},                                                 // SFX_DIRT
    {samplePush, 8, 0},                                                 // SFX_PUSH
    {sampleSpace, 8, 0},                                                // SFX_SPACE
    {sampleDrip2, 8, 0},                                                // SFX_DRIP
    {sampleBubbler, 7, AUDIO_LOCKED | AUDIO_SINGLETON | AUDIO_KILL},    // SFX_BUBBLER
    {sampleDrip, 5, 0},                                                 // SFX_DRIP2
    {sampleUncover, 2, AUDIO_LOCKED | AUDIO_KILL},                      // SFX_UNCOVER
    {sampleKeypress, 2, 0},                                             // SFX_KEYPRESS

#if _ENABLE_LAVA2
    {sampleLava, 2, true},    // 25 SFX_LAVA
#endif
};

_Static_assert(sizeof(AudioSamples) / sizeof(AudioSamples[0]) == SFX_MAX, "AudioSamples table wrong size");


bool audioRequest[SFX_MAX];

void killRepeatingAudio() {

    for (int channel = 0; channel < 2; channel++) {
        RAM[_BUF_AUDC + channel] = 0;
        RAM[_BUF_AUDF + channel] = 0;
        RAM[_BUF_AUDV + channel] = 0;
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

            audioRequest[id] = false;

#if ENABLE_SOUND

            if (AudioSamples[id].flags & AUDIO_SINGLETON) {
                bool dup = false;
                for (int i = 0; i < CONCURRENT_SFX; i++)
                    if (sfx[i].id == id) {
                        dup = true;
                        break;
                    }
                if (dup) {
                    audioRequest[id] = false;
                    continue;
                }
            }

            int lowest = -1;
            for (int i = 0; i < CONCURRENT_SFX; i++) {

                int idx = sfx[i].id;

                if (!idx) {    // empty slot
                    lowest = i;
                    break;
                }

                if ((!(AudioSamples[idx].flags & AUDIO_LOCKED) &&    // not locked, and...
                     (lowest < 0 ||                                  // either we haven't found a lowest yet
                      AudioSamples[idx].priority < AudioSamples[sfx[lowest].id].priority)
                     // or the priority of this track is lower than the lowest
                     // found so far...
                     ))

                    lowest = i;
            }

            // we've now found the lowest priority sound in our current batch...
            // if the lowest slot is lower priority than new sound, replace it
            if (lowest >= 0 &&
                (!sfx[lowest].id || AudioSamples[id].priority >= AudioSamples[sfx[lowest].id].priority)) {

                sfx[lowest].index = 0;
                sfx[lowest].id = id;
                sfx[lowest].delay = AudioSamples[id].sample[3];

                sfx[lowest].attenuation = (AudioSamples[id].flags & AUDIO_ATTENUATE) ? rangeRandom(128) | 128 : 255;
            }
#endif
        }
    }

    // process the sfx segments' commands

    for (int i = 0; i < CONCURRENT_SFX; i++) {
        struct Audio *sfxx = &sfx[i];
        sfxx->handled = false;

        if (sfxx->id && !sfxx->delay--) {

            const struct AudioTable *s = &AudioSamples[sfxx->id];
            sfxx->index += 4;
            unsigned char cmd = s->sample[sfxx->index];

            if (cmd == CMD_STOP)
                sfxx->id = 0;

            else {

                if (cmd == CMD_LOOP) {
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
                if (!best || AudioSamples[best->id].priority < AudioSamples[sfx[i].id].priority)
                    best = &sfx[i];
        }

        if (best) {

            best->handled = true;

            audC = AudioSamples[best->id].sample[best->index];
            audF = AudioSamples[best->id].sample[(best->index) + 1];
            audV = (AudioSamples[best->id].sample[(best->index) + 2] * ((int)best->attenuation + 1)) >> 8;

            switch (best->id) {
            case SFX_MAGIC:
                audF = getRandom32() & 0xF;
                audV = 2;
                break;
            }
        }

#if ENABLE_SOUND

        if (audC && audV >= volume[channel]) {
            RAM[_BUF_AUDC + channel] = audC;
            RAM[_BUF_AUDF + channel] = audF;
            RAM[_BUF_AUDV + channel] = audV;
        }
#endif
    }
}

void playAudio() {

    volume[0] = volume[1] = 0;

    processMusic();
    processSoundEffects();
}

#if 1

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



const unsigned char trackGridLockMelody2[] = {
    FULLNOTE
    e4 g4 c5 b4 d5 g4 a4 c5 e4 g4 c5 b4 d5 f4 a4 c5
    e4 g4 c5 b4 d5 g4 a4 c5 e5 d5 b4 a4 g4 e4 g4 b4
    // HALFNOTE
    c5 b4 a4 g4 f4 e4 d4 c4
    // FULLNOTE
    e4 g4 b4 a4 c5 e5 d5 b4 c5 e5 g5 f5 a5 g5 e5 d5
    c5 e5 d5 b4 a4 g4 f4 e4 g4 b4 d5 c5 e5 g5 e5 c5

    TRACK_LOOP
};

const unsigned char trackGridLockBase2[] = {
    FULLNOTE
    c2 c2 e3 e3 a2_SHARP a2_SHARP f2 f2
    c2 c2 e3 e3 g3 g3 c3 c3
    c2 c2 e3 e3 a2_SHARP a2_SHARP f2 f2
    d3 d3 b2 b2 g3 g3 c3 c3
    // HALFNOTE
    c3 b2 a2_SHARP g3 f2 e3 d3 c2
    // FULLNOTE
    c2 c2 e3 e3 a2_SHARP a2_SHARP f2 f2
    c2 c2 g3 g3 d3 d3 a2_SHARP a2_SHARP
    e3 e3 c3 c3 g3 g3 c2 c2
    f2 f2 c3 c3 g3 g3 c2 c2

    TRACK_LOOP
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

const unsigned char adsr_Trombone[] = {

    0, 100, 200, 250,
    200, 160, 160, 160,
    160,160,160,160,
    80,40,0,0,

};

const unsigned char adsr_Rage[] = {

    100, 200, 255, 255, 255, 255, 255, 255, 255, 255, 125, 75, 50, 25, 0, 0};

const unsigned char adsr_mmh[] = {

    255,255,255,255,
    255,255,255,255,
    255,255,255,255,
    200,100,50,0,
};


const unsigned char *const instrument[] = {
    adsr_Trombone,
    adsr_Rage,
    adsr_mmh
};

// clang-format on

void loadTrack(int priority, const unsigned char *tune, int volume, int dur, int instrument) {

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
    music[best].noteDurationMultiplier = dur;    // 0x80 = single note, 0x40 = half-note, etc
    music[best].baseSpeed = dur;
}

const int multiplier[] = {0, 0x100, 0x200, 0x400};
const unsigned char RENOTE[] = {1, 4, 6, 12};


void processMusic() {

    sound_volume = approach(sound_volume, sound_max_volume, 2);

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
                        track->noteDurationMultiplier = (track->baseSpeed * multiplier[nextNote >> 6]) >> 8;
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
                (instrument[best->instrument][envelope_ptr] * sound_volume * best->volume) >> (4 + 10 + 8);

            RAM[_BUF_AUDF + channel] = note;
            RAM[_BUF_AUDC + channel] = RENOTE[note >> 6];
        }

        else
            break;
    }

    for (int i = 0; i < MUSIC_MAX; i++)
        if (music[i].tune)
            music[i].progress += (8273 * music[i].noteDurationMultiplier) >> 8;    // WTF BOO?!
}

#endif

// EOF