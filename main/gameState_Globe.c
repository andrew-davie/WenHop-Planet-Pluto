#include "defines_dasm.h"

#include "cdfjplus.h"

#include "caveData.h"
#include "colour.h"
#include "draw.h"
#include "drawPlanet.h"
#include "grid6.h"
#include "main.h"
#include "menuCharacterSet.h"
#include "random.h"
#include "reverseBits.h"
#include "savekey.h"
#include "sound.h"

#include "../gfx/fontcompact.h"

#define CHAMP_VOL 100
#define DURATION_GLOBE 50
#define PRESENTS_LUM 8
#define FADE_SPEED 17000
#define FADE_SHIFT 16

static int id_y, ystep;
const unsigned char *thePalette;
int infoPhase;
int wait;
int planet;

struct STARS {
    unsigned int x;
    unsigned int y;
    unsigned char colour;
} stars[10];


void initStars() {

    for (unsigned int i = 0; i < sizeof(stars) / sizeof(stars[0]); i++) {
        stars[i].x = rangeRandom(40) << 8;
        stars[i].y = rangeRandom(66) << 8;
        stars[i].colour = rangeRandom(7) + 1;
    }
}


void initDataStreams_Globe() {

    static const struct dataStreams streams[] = {

        {_DS_GLOBE_COLUBK, _BUF_GLOBE_COLUBK},

        {_DS_GLOBE_PF0_LEFT, _BUF_GLOBE_PF},
        {_DS_GLOBE_PF1_LEFT, _BUF_GLOBE_PF + 1 * _BUFFER_SIZE},
        {_DS_GLOBE_PF2_LEFT, _BUF_GLOBE_PF + 2 * _BUFFER_SIZE},

        {_DS_GLOBE_PF0_RIGHT, _BUF_GLOBE_PF + 3 * _BUFFER_SIZE},
        {_DS_GLOBE_PF1_RIGHT, _BUF_GLOBE_PF + 4 * _BUFFER_SIZE},
        {_DS_GLOBE_PF2_RIGHT, _BUF_GLOBE_PF + 5 * _BUFFER_SIZE},

        {_DS_GLOBE_AUDV0, _BUF_AUDV},
        {_DS_GLOBE_AUDC0, _BUF_AUDC},
        {_DS_GLOBE_AUDF0, _BUF_AUDF},

        {_DS_GLOBE_COLUPF, _BUF_GLOBE_COLUPF},
        {_DS_GLOBE_COLUP0, _BUF_GLOBE_COLUP0},

        {_DS_GLOBE_GRP0A, _BUF_GLOBE_GRP + 0 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP1A, _BUF_GLOBE_GRP + 1 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP0B, _BUF_GLOBE_GRP + 2 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP1B, _BUF_GLOBE_GRP + 3 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP0C, _BUF_GLOBE_GRP + 4 * _BUFFER_SIZE},
        {_DS_GLOBE_GRP1C, _BUF_GLOBE_GRP + 5 * _BUFFER_SIZE},

        {DSJMP1PTR, _BUF_GLOBE_JUMP},

    };

    initDataStreams(streams, sizeof(streams) / sizeof(struct dataStreams));
}


void initKernel_Globe() {

    setJumpVectors(_BUF_GLOBE_JUMP, _globeLoop, _globeExit, _SCANLINES);
    initDataStreams_Globe();

    killRepeatingAudio();

    planet = 0;
    initPlanet(planet);
    id_y = _SCANLINES << 3;
    ystep = -26;

    infoPhase = 0;
    wait = 100;

    sound_max_volume = VOLUME_MAX;
}


void drawPaletteGlobe(const unsigned char *palette) {

    unsigned char *p = RAM + _BUF_GLOBE_COLUPF;

    unsigned char pal[3];

    int baseRoller = roller;
    for (int i = 0; i < 3; i++) {
        pal[i] = convertColour(palette[baseRoller]);
        if (++baseRoller > 2)
            baseRoller = 0;
    }

    for (int i = 0; i < _SCANLINES; i += 3) {
        p[i] = pal[1];
        p[i + 1] = pal[2];
        p[i + 2] = pal[0];
    }
}


const char *planetInfo[] = {
    // clang-format off

    // =  center following lines
    // >  right-justify following lines
    // <  left-justify following lines
    // #  right-close-quote
    // \"  left-open-quote
    // |  next line
    // some non-alpha chars may not have shapes!

    "=\"Squabbling|rock whose|dominant|species spent|millennia|figuring out|how to leave|and, mostly,|didn't|bother.#",
    "=\"The name|was chosen|by committee,|and somehow|it was the|kindest|option on|the table.#",
    "=\"Dim flatulent|gas giant that|smells like|regret and|exists solely|to remind|nearby|systems|what failure|looks like.#",
    "=\"Permanently|shrouded in|a haze that|scientists|politely call|%organic|particulate&|and everyone|else calls|filth.#",
    "=\"Wet grey|lump that|even its own|moons try|to stay|away from.#",
    "=\"I cannot|stress this|enough:|do not touch|the ocean.#",
    "=\"A liquid|world and,|trust me,|you do NOT|want to know|what the|liquid is.#",
    "=\"The fertility|festival they|forgot to|mention in|the booking|details has|permanently|changed me|as a being.#",
    "=\"Locals kept|insisting the|smell was|%cultural&|and I kept|insisting I|wanted|to leave.#",
    "=\"My travel|insurance|specifically|excluded this|planet and in|hindsight that|was a sign.#",
    "=\"Checked in,|checked out,|hosed down,|filed a report|with my|government.#",
    "=\"Left feeling|worse about|the universe|than when|I arrived,|which I|genuinely|did not|think was|possible.#",
    "=\"Met the|locals. Deeply|wish I hadn't.|They feel|the same way|and were|very clear|about it.#",
    "=\"Asked the|concierge|what that pile|was, and|he said|%which one?&|and I said|%any of them!&|and he just|shrugged.#",
    "=\"Warm, close,|humid,|intimate in|ways I did not|consent to.|Still finding|spores in my|luggage.#",
    "=\"I asked what|happened|to the|first three|and nobody|would meet|my eyes.#",
    "=\"The locals|described the|cuisine as|%foraged& and|I described|the|subsequent|evacuation of|my three|stomachs as|%thorough&.#",
    "=\"The spa|promised|%gravitational|therapy&|which means|they just let|you get|fatter and|charge you|for it.#",
    "=\"Four days of|locals|explaining|their ancient|culture|and not one|of them|noticed|I was trying|to leave.#",
    "=\"Still|furious,|have been|furious,|my offspring|will be|furious,|this is my|legacy now.#",
    "=\"The %scenic|overlook& was|a ledge above|a trench full|of something|slow-moving|and the guide|seemed|proud.#",
    "=\"Asked for a|local|delicacy|and was|handed|something|that looked|back at me.#",
    "=\"Breathed|through my|filter the|whole time|and STILL|came home|with|something|my doctor|called|%a new one&.#",
    "=\"My ship|refused to|land and in|retrospect|I should have|listened to it.|It doesn't|have feelings,|and even|it knew.#",
    "=\"The welcome|sign says|YOU'LL GET|USED TO IT|and that is|both the|tourism|slogan...|and a threat.#",
    "=\"The|air quality|sensor in my|suit just said|%NO&|and powered|itself down|in protest.#",
    "=\"The whole|planet has|a slight lean|to it that|nobody|mentions|until you've|already|unpacked.#",
    "=\"Paid for|the premium|tour, which|covered the|same ground|as the|free tour|but with a|guide who|was angrier|about it.#",
    "=\"The locals|have a saying:|%what's yours|is yours|until it isn't&|and that is|also their|legal|system.#",
    "=\"Asked for a|local map and|was handed a|piece of|something,|that wasn't|paper, with|nothing on it,|and that was|apparently|correct.#",
    "=\"The survival|suit rental|place had a|sign saying|%no refunds|if deceased&|and I thought|it was a joke|until I saw|the queue.#",
    "=\"Tried the|local|nightlife,|which runs|from dusk|until about|half past|dusk, then|everyone|goes home|and sighs.#",
    "=\"Genuinely|cannot tell|if they are|at war|or just|always|like this.#",
    "=\"The|%historical|district&|is just the|part of town|where things|broke earlier|than|everywhere|else.#",
    "=\"The|%five-star&|resort had|four stars|missing|from the sign|and I should|have taken|that literally.#",
    "=\"Asked a|local the|population|and he said|%too many&|while looking|directly|at me.#",
    "=\"My photos|came out|completely|grey and the|locals said|that was the|best the|planet had|ever looked.#",
    // clang-format on
};


const char *review[] = {
    ">+++++",    // 0
    ">*++++",    // 1
    ">**+++",    // 2
    ">***++",    // 3
    ">****+",    // 4
    ">*****",    // 5
};

const char *planetInfoName[10] = {
    ">EARTH",       // 0
    ">SPUTUM",      // 1
    ">NEPTUNE",     // 2
    ">BRIMSTON",    // 3
    ">SKUMVEIL",    // 4
    ">GRUNTHOS",    // 5
    ">SWILL",       // 6
    ">LICHONI",     // 7
    ">MUCKSPON",    // 8
    ">PLANET X",    // 9
};

unsigned char planetNameColour[10] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

unsigned char pic;

const char *planetPhysics[10] = {
    ">6900 km|9.81 m/s^",      // 0
    ">1500 km|4.8 m/s^",       // 1
    ">9874 km|1.5 m/s^",       // 2
    ">14566 km|4.3 m/s^",      // 3
    ">42000 km|222.4 m/s^",    // 4
    ">89000 km|11.15 m/s^",    // 5
    ">6800 km|2.3 m/s^",       // 6
    ">12400 km|16.8 m/s^",     // 7
    ">4522 km|22.2 m/s^",      // 8
    ">4522 km|22.2 m/s^",      // 9
};


void initGameState_Globe() {

    myMemsetInt((unsigned int *)(RAM + _GLOBE_BUFFERS_START), 0, _GLOBE_BUFFERS_SIZE / 4);
    myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_COLUP0), 0x58585858, _BUFFER_SIZE / 4);
    frame = 0;
}


void drawBit2(int x, int y, unsigned char colour) {

    colour | colour << 3;
    colour >>= roller;

    int line = y * 3;
    if (line < 0 || line >= _SCANLINES - 3)
        return;

    int col = x;
    if (col < 0 || col > SCREEN_TRIX_X - 1)
        return;

    unsigned char *basex = _BUF_GLOBE_PF + RAM + line;

    if (col >= 20) {
        col = 39 - col;
        basex += 3 * _BUFFER_SIZE;
    }

    basex += ((col + 4) >> 3) * _BUFFER_SIZE;

    int shift;
    if (col < 4)
        shift = col + 4;
    else if (col < 12)
        shift = 11 - col;
    else
        shift = (col - 12);

    int bit = 1 << shift;

    unsigned char mask0 = (colour) << shift;
    unsigned char mask1 = (colour >> 1) << shift;
    unsigned char mask2 = (colour >> 2) << shift;

    if (!((basex[0] | basex[1] | basex[2]) & bit)) {
        basex[0] = (basex[0] & ~bit) | (bit & mask0);
        basex[1] = (basex[1] & ~bit) | (bit & mask1);
        basex[2] = (basex[2] & ~bit) | (bit & mask2);
    }
}

int lastpd;

enum info {

    INFO_NAME,
    INFO_PHYSICS,
    INFO_INFO,
    INFO_RATING1,
    INFO_RATING2,
    INFO_CLEAR,
    INFO_NEXTPLANET,
};


void VB_Globe() {

    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    if (frame > DURATION_GLOBE && (RAM[_SWCHB] != 0x3F))
        setGameState(GS_MENU);

    drawPlanet(5);

    for (unsigned int i = 0; i < sizeof(stars) / sizeof(stars[0]); i++) {
        drawBit2(stars[i].x >> 8, stars[i].y >> 8, stars[i].colour);
    }

    if (!drawNextChar() && !--wait) {
        switch (infoPhase++) {

        case INFO_NAME:
            initAsciiStringDraw(FONT_LARGE, planetNameColour[planet] + 2, 8, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                                planetInfoName[planet], 0, 158 - 5);
            wait = 10;
            break;

        case INFO_PHYSICS:
            initAsciiStringDraw(FONT_COMPACT, 0x14, 1, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planetPhysics[planet], 0,
                                175 - 4);
            wait = 50;
            pic = ((rangeRandom(15) + 1) << 4) | 8;

            break;

        case INFO_INFO: {

            int pif = rangeRandom(sizeof(planetInfo) / sizeof(planetInfo[0]));

            int lines = 1;
            const char *p = planetInfo[pif];
            do
                if (*p == '|')
                    lines++;
            while (*++p);

            initAsciiStringDraw(FONT_COMPACT, 0xD8, 2, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planetInfo[pif], 0,
                                70 - ((lines * FONTCOMPACT_FONT_HEIGHT) >> 1));
            wait = 50;
            break;
        }

        case INFO_RATING1:
            initAsciiStringDraw(FONT_COMPACT, 0x18, 1, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[0], 0, 142 - 4);
            wait = 20;
            break;

        case INFO_RATING2:
            initAsciiStringDraw(FONT_COMPACT, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[rangeRandom(6)], 0,
                                142 - 4);
            wait = 250;
            break;


        case INFO_CLEAR:
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            wait = 150;
            lastpd = planetDir < 0 ? -1 : 1;
            break;

        case INFO_NEXTPLANET:
            if (lastpd != (planetDir < 0 ? -1 : 1) && planetDir < 0) {
                planet = nextPlanet();
                wait = 200;
                infoPhase = INFO_NAME;
            } else {
                lastpd = planetDir < 0 ? -1 : 1;
                infoPhase = INFO_NEXTPLANET;
                wait++;
            }
            break;
        }
    }
}


void OS_Globe() {

    interleaveChronoColour(&roller);
    myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_PF), 0, 6 * _BUFFER_SIZE / 4);

    drawPlanet(0);

    id_y += ystep;

    if ((ystep < 0 && id_y < (60 << 3)) || (ystep > 0 && id_y > (65 << 3)))
        ystep = -(ystep >> 2);
}

// EOF