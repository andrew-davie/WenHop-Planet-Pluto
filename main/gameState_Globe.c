#include "defines_dasm.h"

#include "cdfjplus.h"

#include "caveData.h"
#include "colour.h"
#include "draw.h"
#include "drawPlanet.h"
#include "drawplanet.h"
#include "grid6.h"
#include "main.h"
#include "menuCharacterSet.h"
#include "planet.h"
#include "random.h"
#include "reverseBits.h"
#include "savekey.h"
#include "sound.h"

#include "../gfx/fontcompact.h"
#include "../gfx/fontlarge.h"

#define CHAMP_VOL 100
#define DURATION_GLOBE 50
#define PRESENTS_LUM 8
#define FADE_SPEED 17000
#define FADE_SHIFT 16

const unsigned char *thePalette;

static int infoPhase;
static int wait;
static int planet;


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

enum info {

    INFO_FADEUP,
    INFO_NAME,
    INFO_PHYSICS,
    INFO_CLEAR1,
    INFO_PLANETADVISOR,
    INFO_INFO,
    INFO_RATING2,
    INFO_CLEAR,
    INFO_NEXTPLANET,
    INFO_FADE_DOWN,
};


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

/* Track 1 — Melody */
const unsigned char track1[] = {HALFNOTE
                                    /* Intro: slow opening, rise and return */
                                    e5 g5 b5 a5 g5 e5 d5 e5 QUARTERNOTE
                                        /* Main theme A — ascending flow, graceful descent */
                                        e5 g5 a5 b5 a5 g5 f5_SHARP e5 d5 e5 g5 a5 b5 c6 b5 a5 g5 f5_SHARP e5 d5 e5 g5 a5
                                            b5 c6 b5 a5 g5 f5_SHARP e5 d5 e5 HALFNOTE
                                                /* Bridge: slower, lifting */
                                                f5_SHARP g5 a5 b5 a5 g5 QUARTERNOTE
                                                    /* Development — reach higher, return through the scale */
                                                    e5 f5_SHARP g5 a5 b5 c6 b5 a5 g5 a5 b5 c6 b5 a5 g5 f5_SHARP e5 d5 e5
                                                        g5 a5 g5 f5_SHARP e5 d5 e5 f5_SHARP g5 a5 b5 a5 g5 HALFNOTE
                                                            /* Cadence: winding down */
                                                            f5_SHARP e5 d5 e5 d5 e5 FULLNOTE
                                                                /* Resolution */
                                                                e5 TRACK_LOOP};

/* Track 2 — Bass (slow harmonic support, drops low for the void) */
const unsigned char track2[] = {
    HALFNOTE
        /* Under intro */
        e4 e4 g4 g4 a4 g4 e4 d4
            /* Under main theme */
            e4 g4 a4 g4 e4 d4 c4 d4 FULLNOTE e3 /* deep drop — the void */
                e4                              /* return */
                    HALFNOTE
                        /* Under bridge */
                        g4 a4 g4 e4 d4 e4 g4 a4
                            /* Under development */
                            e4 g4 a4 g4 e4 d4 e4 g4 FULLNOTE e3 /* second deep drop */
                                g3                              /* low third */
                                    HALFNOTE
                                        /* Final approach */
                                        a4 g4 e4 d4 c4 e4 FULLNOTE e3 /* resolve into the deep */
                                            e4 TRACK_LOOP};


/* Track 1 — Melody */
const unsigned char track1b[] = {HALFNOTE
                                     /* Opening dread — Phrygian minor 2nd crawl */
                                     d5 d5_SHARP d5 c6 a5 g5 f5 d5_SHARP
                                         /* Descent through the void */
                                         d5 c6 b5 a5 g5 f5 d5_SHARP d5 QUARTERNOTE
                                             /* Tension builds — chromatic crawl upward */
                                             d5 d5_SHARP f5 g5 a5 g5 f5 d5_SHARP d5 c6 b5 a5 g5 f5 d5_SHARP d5
                                                 /* Tritone sting at the peak */
                                                 d5_SHARP f5 g5 a5 b5 c6 b5 a5 g5 f5 d5_SHARP d5 c6 a5 g5 f5 HALFNOTE
                                                     /* Heavy, inevitable descent */
                                                     d5 d5_SHARP f5 g5 f5 d5_SHARP d5 c6 FULLNOTE d5 TRACK_LOOP};

/* Track 2 — Bass */
const unsigned char track2b[] = {FULLNOTE
                                     /* Low rumble — root and third */
                                     d4 f3 d4 HALFNOTE
                                         /* Chromatic crawl */
                                         f3 g3 f3 d4 c4 d4 c4 f3 FULLNOTE e3 /* abyss drop */
                                             g3 HALFNOTE
                                                 /* Development underpinning */
                                                 d4 c4 f3 g3 d4 c4 d4 f3 FULLNOTE e3 /* second drop, deeper dread */
                                                     d4 HALFNOTE f3 g3 c4 d4 TRACK_LOOP};


// clang-format off

/* Track 1 — Melody */
const unsigned char track1c[] = {
    HALFNOTE
    /* Gentle fall */
    a5 g5 f5 e5
    FULLNOTE
    d5
    HALFNOTE
    /* Rise, settle */
    e5 g5 a5 g5
    FULLNOTE
    f5
    HALFNOTE
    /* Searching */
    e5 d5 e5 g5
    FULLNOTE
    a5
    HALFNOTE
    /* Fall again, unexpected dip */
    g5 f5 e5 d5
    c5 d5
    FULLNOTE
    d5
    TRACK_LOOP
};

/* Track 2 — Sparse harmonic pillars (raised) */
const unsigned char track2c[] = {
    FULLNOTE
    d4
    g4
    a4
    d4
    f4
    g4
    d4
    a4
    g4
    d4
    TRACK_LOOP
};
/* Track 1 — 3 note loop: slow ascending crawl */
const unsigned char track1d[] = {FULLNOTE e3 f3 g3 TRACK_LOOP};

/* Track 2 — 5 note loop: oscillates across the same cluster differently */
const unsigned char track2d[] = {FULLNOTE g3 f3 e3 f3 g3_SHARP TRACK_LOOP};

void initKernel_Globe() {

    setJumpVectors(_BUF_GLOBE_JUMP, _globeLoop, _globeExit, _SCANLINES);
    initDataStreams_Globe();

    killRepeatingAudio();

    planet = 1;
    initPlanet(planet);


    infoPhase = INFO_FADEUP;
    wait = 50;

    luminance = -15;
    lumTarget = 0;

    sound_volume = VOLUME_PLAYING;

    int speed = 0x40; //rangeRandom(80) + 16;

    loadTrack(10, track1b, 50, speed, 1);
    loadTrack(0, track2b, 35, speed, 2);
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


const struct pi {

    const char *review;
    unsigned char stars;

} planetInfo[] = {

// clang-format off

    // =  center following lines
    // >  right-justify following lines
    // <  left-justify following lines
    // #  right-close-quote
    // \"  left-open-quote
    // |  next line
    // some non-alpha chars may not have shapes!

#if 0



#else

{"=\"A human said|I looked|well. I said I|did not feel|well, and it|said no, you|look good. I|said I did|not feel|good and it|left.#",3},
{"=\"A human told|me to have a|nice day,|and I said I|would try,|and it looked|at me as|though trying|was not the|expected|response.#",3},
{"=\"A human|laughed at|me, and I|read this as|a threat|display and|responded in|kind, and the|crowd that|formed did|not help.#",2},
{"=\"A kind word|here causes|mild pain,|and I was|polite on day|one, and did|not connect|these two|facts until|well into day|two.#",2},
{"=\"A local told|me to take|care when I|left. I asked|of what. She|said just in|general. I|said that is a|lot to take|care of.#",3},
{"=\"A local|sneezed,|and I replied|in kind, and|it looked|alarmed, and|I had said|something I|will not|repeat here.#",1},
{"=\"Asked a local|how long|they had|lived here,|and she said|longer than|she meant|to, and|looked at|the horizon.#",2},
{"=\"Asked a local|if the planet|had feelings.|No. Are you|sure. Yes. It|seems sad.|They walked|away. I think|I was right.#",3},
{"=\"Asked a local|if the smell|was natural or|man-made.|She said|'yes'. I have|thought about|this every|day since.#",3},
{"=\"Asked a local|what they|did for fun.|They stared|long enough|that I felt I|had said|something in|very poor|taste.#",2},
{"=\"Asked a local|what time it|was and he|told me in a|unit I do not|have the|organs to|perceive.|Useless.#",2},
{"=\"Asked a local|what to see,|and she|began|thinking|about it and|was still|thinking|when I left.#",2},
{"=\"Asked a|ranger which|trail was|best. He said|'It depends'.|I said|'scenery'.|He said 'then|not this|planet'.#",3},
{"=\"Asked for an|early|wake-up|call, and|their early|and my early|were|several|hours apart|in the wrong|direction.#",2},
{"=\"Asked for|directions|and told to|follow my|nose, which|I do not|have, and|was told to|follow it|anyway.#",2},
{"=\"Asked how|far the|viewpoint|was, and he|said not far,|and walked|with me for|four hours,|and still|called it not|far.#",2},
{"=\"Asked if the|air was safe|and was told|'for some|definitions|of safe'|which my|species|processes as|a yes and is|not.#",1},
{"=\"Asked if the|haze ever|cleared.|'Cleared of|what,' they|said. I|realised they|had never|seen their|planet. Nor|had I.#",2},
{"=\"Asked if the|haze ever|clears, and|the local|said clears|of what, and|I|understood|she had|never seen|the planet.#",1},
{"=\"Asked what|grew here.|Told|'resentment'.|I said that|sounds|promising for|a cultural|visit. It was|not.#",2},
{"=\"Asked what|grows here,|was told|resentment,|booked the|culture|tour, it was|about|resentment.#",2},
{"=\"Asked what|the big pile|was and was|told it was|the|economy,|and I did not|follow up.#",1},
{"=\"Asked what|the locals do|for fun, and|the silence|that|followed|was|technically|an answer.#",2},
{"=\"Asked|where all the|valuable|minerals had|gone and was|told 'away'|which is|technically|an answer.#",2},
{"=\"Assumed the|atmosphere|was|decorative|and removed|my suit. This|was my|second|mistake,|after|booking.#",1},
{"=\"Assumed the|dog was the|local leader|based on how|everyone|treated it,|and spent|two days|following it|before being|corrected.#",2},
{"=\"Beautiful|planet,|would|return,|cannot|return, legal|reasons,|four stars.#",4},
{"=\"Checked in|to what I|thought was|my room, and|it was a lift,|and I|unpacked|before the|lift|explained|itself.#",2},
{"=\"Checked the|reviews|before|going, they|all said it|was fine,|those|reviewers|have given|up.#",1},
{"=\"Clapped|after the|lightning, as|is polite at|home, and|several|locals moved|away before|I could|explain.#",2},
{"=\"Consumed|the|atmosphere|as a snack.|Two|secondary|digestive|organs have|since lodged|a formal|complaint.#",1},
{"=\"Did not|realise|breathing was|compulsory|here and had|to be|reminded|twice,|signage is|inadequate.#",2},
{"=\"Discovered|customer|service,|which is|humans|saying sorry|for things|they will|immediately|do again.#",2},
{"=\"Each object|has a name,|and must be|addressed|when|touched. I|caused|three|scenes|before lunch|on day one.#",2},
{"=\"Each room|runs at a|different|speed, and I|was|somehow|early and|late for the|same meal.#",1},
{"=\"Emergency|card,|seventeen|steps. Step|one:|'reconsider'.|The rest:|increasingly|specific|ways to|leave.#",1},
{"=\"Everyone|here has|three|shadows,|one of them|cold, and|mine kept|touching|theirs, and|this was|rude.#",2},
{"=\"Everything is|slightly left|of where it|appears. I|adapted on|day four,|went home,|and broke|things I had|owned for|years.#",2},
{"=\"Everything|here has six|legs except|the things|with eight,|and I spent a|week trying|to find the|pattern, and|there is not|one.#",3},
{"=\"Happiness|here is going|still. I kept|asking if|locals were|well, and|upset them,|which they|showed by|moving.#",2},
{"=\"Have a full|account of|this place,|but the box|provided is|too small,|and I have|run out of|time to find|a bigger one.#",4},
{"=\"Held my|umbrella the|wrong way|for two days|before|learning the|rain falls|down here.#",2},
{"=\"I asked a|local for the|best dish,|she named|one, I|ordered it,|she brought|it out, and|seemed to|have planned|all of this.#",3},
{"=\"I asked what|not to miss.|She said|everything. I|asked what|she liked.|She said she|had not|thought|about it.#",2},
{"=\"Joined what|I thought|was a guided|walk. It was|a funeral. By|the end I|was a|pallbearer|and did not|know how to|raise this.#",2},
{"=\"Landed,|explored,|ate|something,|left. Three|of those|four I would|not do again,|and I will not|say which|three.#",2},
{"=\"Local|timekeeping|uses units|that match|nothing|astronomical.|I said so. A|human said|'yeah' and|walked away.#",2},
{"=\"Lovely|staff,|wrong|planet, too|late to do|much about|it by the|time I|noticed.#",3},
{"=\"Memory is|shared|here, and I|woke on day|two knowing|private|things about|strangers I|had not met.#",2},
{"=\"Misread the|entry form,|declared|myself|luggage, and|spent day|one in a|storage bay|nicer than|my hotel.#",3},
{"=\"Mistook the|warning|siren for a|welcome|song, and|sang back|for twenty|minutes|before|someone|intervened.#",2},
{"=\"Moisture|stuck to my|outer layer|and I took|this as a|territorial|claim and|panicked.|Better|labelling|would help.#",2},
{"=\"My photos|came out|entirely|grey, and|the locals|said that|was the best|the planet|had looked.#",2},
{"=\"My ship's|navigation|refused to|save this|location to|favourites,|and I|understand|now it was|trying to|help me.#",1},
{"=\"My suit's air|monitor|skipped|numbers and|went|straight to a|small picture|of something|dying.#",1},
{"=\"My translator|rendered|the local|language as|'medium|length sighs'|all visit. I'm|not entirely|sure that was|wrong.#",3},
{"=\"Ordered|breakfast,|it arrived at|dinner,|dinner|arrived the|following|week, I left|before|lunch.#",2},
{"=\"Paid for the|premium|tour and the|standard|tour, and|they were|the same|tour with a|different|hat on the|guide.#",1},
{"=\"Roads are in|the sky,|buildings are|underground,|and I used|both the|wrong way|for two|days.#",2},
{"=\"Saw a human|walking by a|small animal|on a lead,|and was|unsure who|was in|charge until|the animal|made it|clear.#",3},
{"=\"Silence is|against local|law, and I|was fined|three times|on the first|day before I|grasped the|scale of the|problem.#",1},
{"=\"Stepped off|the ship and|became part|of the local|food chain|within|minutes.#",1},
{"=\"Stepped off|the ship and|joined the|local|ecosystem.|Took effort|to reverse.|Left me|feeling|implicated in|something.#",2},
{"=\"The air here|is edible,|and I had too|much on day|one and had|to lie down|for reasons|I could not|convey to|anyone|nearby.#",1},
{"=\"The beach|looked|perfect in|photos, and|photos were|the right|way to|experience|it.#",1},
{"=\"The cold|here has|texture, and|I was not|warned|about this,|and I do not|have the|right organs|for it.#",1},
{"=\"The compost|bin in my|room was|sentient and|began making|requests on|day two, and|I did not|know how to|refuse.#",1},
{"=\"The days|are eighteen|minutes. I|booked|three nights|and was|there for|under an|hour.#",2},
{"=\"The dig tour|lets you|uncover|layers of|past life,|and each|layer is|worse than|the one|above it.#",2},
{"=\"The exit sign|pointed at|the ocean,|and the|ocean had no|exit, very|poor planning.#",1},
{"=\"The festival|was|described|as lively, and|involved|twelve|people|standing|near a thing.#",2},
{"=\"The food|here is|invisible and|you eat by|feel. I ate|the table my|first night|and was|charged for|the table.#",1},
{"=\"The gas|here is|sentient in|patches, and|one of the|patches was|in my cabin|and had|strong views|about the|lighting.#",1},
{"=\"The ground|changes|colour based|on mood, and|mine kept|showing|things I had|not planned|to share in|public.#",1},
{"=\"The guide|pointed at|something,|said there it|is, and|walked on|before I|could see|what it was.#",1},
{"=\"The guide|was|excellent,|then said|this is where|I turn back,|and did, and|I had no map.#",1},
{"=\"The guided|walk was|described|as gentle,|and nature|had a|different|idea about|what gentle|means.#",1},
{"=\"The heat|here sits in|the range my|species uses|for food|storage and I|kept|reflexively|trying to put|things in the|locals.#",1},
{"=\"The hot|spring was|listed as|relaxing, and|I have a|different|word for|what it was,|and that|word is not|relaxing.#",1},
{"=\"The hotel|stars were|awarded by|the hotel,|which is a|system with|a visible|flaw.#",1},
{"=\"The humans|share|information|by vibrating|their|face-holes.|Impressive|but needless|given that|telepathy|exists.#",2},
{"=\"The humans|share|information|by vibrating|their|face-holes.|Impressive|but needless|given that|telepathy|exists.#",2},
{"=\"The humans|sleep for a|third of|their lives,|and built all|their|systems|around this,|which|explains the|systems.#",2},
{"=\"The kids|here have a|look in their|eyes that I|have thought|about every|day since I|left.#",1},
{"=\"The local|dish arrived|alive and|looking at|me, and I|did not know|the correct|response.#",1},
{"=\"The local|trees finish|your|thoughts,|and some of|mine were|not ones I|would have|chosen to|share with a|tree.#",1},
{"=\"The locals|are made of|sound, and I|kept walking|through|them, and|they kept|saying sorry,|which made|the problem|louder.#",2},
{"=\"The locals|can see|sound and|asked me to|keep it|down. I said|I was not|making any,|and they|showed me|that I was.#",2},
{"=\"The locals|change|colour to|speak, and I|have no such|ability, and|spent the|trip mute in|a way I have|not been|before.#",1},
{"=\"The locals|exhale what|I breathe,|and I exhale|what they|breathe, and|for one|afternoon|we were|very close.#",4},
{"=\"The locals|have many|words for|one type of|regret. I|used most|before|leaving, and|used them|correctly.#",2},
{"=\"The locals|have no word|for %no& and|express it by|doing the|thing|anyway, but,|sadly, I|missed this|for three|days.#",2},
{"=\"The locals|have thirty|words for|types of rain|and none for|leaving,|which tells|you|something.#",2},
{"=\"The locals|have|seventeen|senses and|explained|the planet|using all of|them, and I|have five|and missed|most.#",3},
{"=\"The locals|leave gaps in|speech for|you to fill. By|day one, I|had agreed|to several|things I had|not planned|on.#",2},
{"=\"The locals|said the|smell was|part of the|culture, and|I said which|part, and|they said all|of it.#",1},
{"=\"The locals|shed opinions|the way|others shed|skin, and|several|stuck to me|by day|three, and I|could not put|them down.#",1},
{"=\"The locals|shed their|skin each|morning and|leave it in|the hall. I|greeted my|neighbour's|%shed& for|two days.#",2},
{"=\"The ocean|here is|technically|swimmable in|the sense|that you can|enter it and|exit it, the|question is|what you exit|as.#",2},
{"=\"The ocean|laps the|shore in a|way that|sounds like it|is sighing at|you|personally.#",1},
{"=\"The oceans|are warm|and the|things in|them are|also warm|and|friendly, in|the way that|means you|cannot|swim.#",3},
{"=\"The phone|box looked|like a|transport|pod, and by|the time I|understood|it was not, I|had pressed|everything.#",1},
{"=\"The planet is|beautiful|from orbit,|and less so|up close, and|the|brochure|was taken|from orbit.#",3},
{"=\"The planet's|famous|silence is|broken only|by the|wildlife, and|the wildlife|is loud and|has not read|the|brochure.#",2},
{"=\"The planet|has a smell|that changes|with your|thoughts. I|could not|stop thinking|about the|smell, and it|kept|changing.#",2},
{"=\"The planet|has a|breathable|atmosphere|in the loosest|possible|reading of|the word|'breathable'.#",1},
{"=\"The planet|has no native|predators,|which is|true, and|the wildlife|found other|solutions the|brochure|does not|address.#",1},
{"=\"The planet|hums a note|not quite any|note I know,|and my|species has|a word for|that note,|and the|word means|leave.#",1},
{"=\"The planet|hums at a|pitch my|species|reads as|grief, and I|cried for|five days|before|making the|connection.#",1},
{"=\"The planet|leans, and|every local I|told looked|genuinely|unsure if|that was|true.#",2},
{"=\"The planet|smells of|something I|cannot name|but have|smelled|once|before, in a|situation I|do not wish|to revisit.#",2},
{"=\"The ranger|said stay on|the path,|and when I|asked what|was off it,|she said she|would rather|not say.#",1},
{"=\"The scenic|flyby is good|if you like|gas, which I|did not know|I did not like|until I was|inside it.#",2},
{"=\"The shadow|followed me|the whole|time, and|when I|complained,|was told it|was mine,|which raised|more|questions.#",2},
{"=\"The slow|part of the|day is called|rush hour, a|naming|decision|that has|clearly|never been|reviewed.#",3},
{"=\"The sun|rises from|the ground|and sets|upward, and|by day three|my sense of|down was a|rough guess.#",2},
{"=\"The surface|is lovely|from orbit,|which is also|the viewing|distance the|tourism|board|suggests,|buried on|page nine.#",2},
{"=\"The thermal|pool was not|a pool in any|sense I had|prepared|for.#",1},
{"=\"The tour bus|had|seatbelts,|which I|thought was|standard,|until the|driver put|his on and|said right,|here we go.#",1},
{"=\"The tourism|office gave|me a list of|things to do,|and one of|them was|the tourism|office, and|I was|already|there.#",3},
{"=\"The tourist|board slogan|is %WE ARE|HERE&, and|that is the|full extent|of the pitch.#",2},
{"=\"The trail|was marked|safe, and|safe here|means a|different|thing than it|means|where I am|from.#",1},
{"=\"The two|moons orbit|at a|distance|that|suggests|they are|trying to|stay out of|things.#",2},
{"=\"The upgrade|was to a|larger pile,|and I said|thank you|because I|did not know|what else to|say.#",2},
{"=\"The vendor|said the|item was|rare and|local, and I|pointed out|every stall|had one, and|she said yes,|rare and|local.#",2},
{"=\"The volcano|tour was|cancelled|due to the|volcano, a|sentence I|could not|have|predicted|needing.#",2},
{"=\"The wildlife|is harmless|if you avoid|eye|contact, and|the wildlife|is|everywhere|and makes a|great deal|of it first.#",1},
{"=\"The year is|three days|long. I|booked a|week,|attended|five new|years, and a|gift was|expected|each time.#",1},
{"=\"The|attendant|said the|wildlife was|friendly,|and when I|asked how|friendly,|she paused|in a way that|answered.#",1},
{"=\"The|checkout|time was|listed as|flexible, and|flexible|here means|fixed, and|also earlier|than I was|told.#",1},
{"=\"The|currency is|based on|something I|will not|name, but I|had more of|it after a|long trip, and|that felt|unfair.#",1},
{"=\"The|currency is|smell-based.|I arrived|with nothing|of value, and|left owing a|scent debt I|am still|repaying.#",1},
{"=\"The|fireworks|looked like|an attack,|and I|responded,|and the|response to|my response|was not what|I hoped for.#",2},
{"=\"The|handshake|has a part|where you|pretend not|to know each|other. I|skipped that|part, and|things went|wrong.#",2},
{"=\"The|nightlife|starts at|dusk, ends|shortly|after dusk,|and|everyone|goes home|and sighs.#",2},
{"=\"The|wake-up call|said only it is|time, and|did not say|time for|what.#",2},
{"=\"The|welcome|pack|included a|breathing|guide, which|I assumed|was a joke,|and it was|not a joke.#",1},
{"=\"There is no|word for|stranger,|and I was|given a|family on|arrival, and|they had|views on how|I spent my|days.#",2},
{"=\"They keep a|room clean|and ready|for guests,|sit in a|worse room|on all other|days, and|call this|having a|lounge.#",3},
{"=\"They pay for|water in a|bottle when|it falls free|from the|sky, complain|when it|falls, and|both are|considered|normal.#",3},
{"=\"They say|how are you|to each|other as a|greeting, do|not want to|know, and|are alarmed|when told.#",3},
{"=\"They|removed the|stimulant|from their|stimulant|drink, still|drink it, and|call this a|choice they|made|freely.#",3},
{"=\"Things here|exist only in|pairs, and I|arrived|alone, and|was quietly|pitied by|everyone|for the|duration of|my stay.#",2},
{"=\"Three|moons, and|all of them|wrong, and I|do not know|what I|expected,|but it was|not this.#",2},
{"=\"Time here is|measured in|moods, and|my shuttle|was due at|after the|sadness, and|I waited at|the wrong|feeling.#",1},
{"=\"Travel|insurance|listed this|planet by|name in the|exclusions,|and I booked|anyway, my|mistake.#",1},
{"=\"Tried to buy|someone's|shadow, as it|was the best|one I had|seen, and|they said no|and did not|seem to|take it as a|kind remark.#",2},
{"=\"Tried to eat|the colour|blue as it|looked|nutritious,|and a local|stopped me,|and I do not|know what|she thought|I was doing.#",2},
{"=\"Tried to eat|the soil as a|starter and|was told this|is not done|here.|Different|culture.|Would have|appreciated|a heads up.#",2},
{"=\"Tried to|explain to a|human that|their planet|is unusual,|and she said|we like to|think we are|special, and|I said I|know.#",3},
{"=\"Was handed|a leaflet of|things to do|here, and|the leaflet|had one|page, and|one of the|things listed|was leaving.#",3},
{"=\"Was told the|piles were|'historical'|and I said|'historical|what' and|nobody|finished the|sentence.#",3},
{"=\"Was told to|make myself|at home, and|did, and was|asked to|leave, and|learned|there is a|gap between|the two.#",3},
{"=\"Watched a|human argue|with a small|glowing|rectangle|while sitting|next to|someone|they|ignored.#",2},
{"=\"Waved at my|reflection|for two days|before a|local|explained|what it was,|and I had to|rethink|several prior|greetings.#",2},
#endif

    // clang-format on
};

#define PIS (sizeof(planetInfo) / sizeof(planetInfo[0]))

bool seen[PIS];


/*

Did not realise breathing was compulsory on this planet and had to be reminded twice, signage is inadequate.
I consumed the atmosphere as a snack and would not recommend it, two of my secondary digestive organs have lodged a
formal complaint. Tried to eat the soil as a starter before the main meal arrived and was told soil is not a starter
here, very different culture, would have appreciated a heads up. Asked a local what time it was and he told me in a unit
I do not have the organs to perceive, useless. The temperature here sits in the range my species uses for food storage
and I kept reflexively trying to put things in the locals. Was told the piles were "historical" and I said "historical
what" and nobody finished the sentence. The moisture here kept sticking to my outer layer and I assumed I was being
claimed as territory and panicked unnecessarily, better labelling would help. The locals speak in a language that my
translator rendered as "medium-length sighs" for the entire visit and I'm not entirely sure that was wrong. Asked a
local if the planet had feelings and they said no and I said are you sure and they said yes and I said because it seems
sad and they walked away and I think I was right. The planet has a breathable atmosphere in the loosest possible
interpretation of the word "breathable. Assumed the atmosphere was decorative and removed my suit to appreciate the view
more directly, this was my second mistake of the trip after booking it. The local timekeeping system divides the day
into units that do not correspond to anything astronomical and when I pointed this out a human said "yeah" and walked
away. Asked where all the valuable minerals had gone and was told "away" which is technically an answer. Asked a local
if the smell was natural or man-made and she said "yes" and walked away, I have thought about this every day since.
Asked if the air was safe and was told "for some definitions of safe" which my species processes as a yes and is not.
Asked if anything grew here and was told "resentment" and I said that sounds promising for a cultural visit and it was
not. Asked a local if the haze ever cleared and they said "cleared of what" and I realised they had never seen their own
planet and did not know what was under the haze and neither do I. Stepped off the ship and immediately became part of
the ecosystem in a way that took considerable effort to reverse and left me feeling implicated in something. Asked a
local what they did for fun and they stared at me long enough that I started to feel I had said something in very poor
taste. The emergency information card in my room had seventeen steps and step one was "reconsider" and steps two through
seventeen were increasingly specific ways to leave. The ocean here is technically swimmable in the sense that you can
enter it and exit it, the question is what you exit as.



*/

const char *review[] = {
    ">+++++",    // 0
    ">*++++",    // 1
    ">**+++",    // 2
    ">***++",    // 3
    ">****+",    // 4
    ">*****",    // 5
};


void initGameState_Globe() {

    myMemsetInt((unsigned int *)(RAM + _GLOBE_BUFFERS_START), 0, _GLOBE_BUFFERS_SIZE / 4);
    myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_COLUP0), 0x58585858, _BUFFER_SIZE / 4);
    frame = 0;

    for (unsigned int i = 0; i < PIS; i++)
        seen[i] = false;
}


void drawBit2(int x, int y, unsigned char colour) {

    // colour | colour << 3;
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


void VB_Globe() {


    adjustLuminance();
    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    drawPlanet(5);

    for (unsigned int i = 0; i < 10; i++)    // sizeof(stars) / sizeof(stars[0]); i++)
        drawBit2(stars[i].x >> 8, stars[i].y >> 8, stars[i].colour);


    static int nsv = VOLUME_PLAYING;

    if (!rangeRandom(1000))
        nsv = rangeRandom(sound_max_volume);

    extern int step_toward(int current, int target);
    sound_volume = step_toward(sound_volume, nsv);

    if (sound_volume < 0)
        sound_volume = 0;
    if (sound_volume > sound_max_volume)
        sound_volume = sound_max_volume;
}


static int pif;
static int lines;
static int pa_lines;

#define ADJ 6

void OS_Globe() {

    interleaveChronoColour(&roller);

    drawPlanet(0);

    if (!drawNextChar() && !--wait) {
        switch (infoPhase++) {

        case INFO_FADEUP:

            wait = 1;
            if (luminance)
                infoPhase = INFO_FADEUP;
            break;


        case INFO_NAME:
            initAsciiStringDraw(FONT_LARGE, 0x8, 8, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planets[planet].name, 0,
                                _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - FONTLARGE_FONT_HEIGHT - 25 + ADJ, false);

            wait = 10;
            break;

        case INFO_PHYSICS:
            initAsciiStringDraw(FONT_COMPACT, 0x16, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                                planets[planet].physics, 0, _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - 25 + 5 + ADJ,
                                false);
            wait = 200;
            // pic = ((rangeRandom(15) + 1) << 4) | 8;


            int lastpif = pif;
            pif = rangeRandom(PIS);

            int reviews = PIS;
            if (reviews > 2) {
                int pif2 = pif;

                while (seen[pif]) {
                    pif++;
                    if (pif >= reviews)
                        pif = 0;

                    if (pif == pif2) {
                        for (int i = 0; i < reviews; i++)
                            seen[i] = false;
                        seen[lastpif] = true;
                    }
                }

                seen[pif] = true;
            }


            lines = 1;
            const char *p = planetInfo[pif].review;
            do
                if (*p == '|')
                    lines++;
            while (*++p);


            break;

        case INFO_CLEAR1:
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            ADDAUDIO(SFX_DOGE2);
            wait = 30;
            break;

        case INFO_PLANETADVISOR:

            pa_lines = ((_SCANLINES - 1 - lines * FONTCOMPACT_FONT_HEIGHT) >> 1) - 20 + ADJ;


            initAsciiStringDraw(FONT_COMPACT, 0x8, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                                "=PlanetAdvisor", 0, pa_lines, true);
            wait = 25;
            break;

        case INFO_INFO: {
            initAsciiStringDraw(FONT_COMPACT, 0xD8, 4, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planetInfo[pif].review, 0,
                                ((_SCANLINES - 1 - ((lines * FONTCOMPACT_FONT_HEIGHT))) >> 1) + ADJ, false);
            wait = 50;
            break;
        }

        case INFO_RATING2:
            pa_lines = ((_SCANLINES - 1 + lines * FONTCOMPACT_FONT_HEIGHT) >> 1) + 8 + ADJ;
            initAsciiStringDraw(FONT_COMPACT, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                                review[planetInfo[pif].stars], 0, pa_lines, false);
            wait = 150;
            break;


        case INFO_CLEAR:
            killRepeatingAudio();
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            wait = 30;
            break;

        case INFO_NEXTPLANET:

            // #define SCALE_FAR 0x20000
            // #define SCALE_NEAR 0x0C000
            // #define MIDPOINT ((SCALE_FAR + SCALE_NEAR) / 2)    // ~131180
            // planetdir + === away


            if (scalex > (SCALE_FAR - 0x4000) && planetDir > 0) {
                lumTarget = -15;
                // if (scalex > MIDPOINT && planetDir < 0) {    //(scalex >> 8) == (SCALE_FAR >> 8) && planetDir >
                // 0) {
                wait++;
            } else {
                infoPhase = INFO_NEXTPLANET;
                wait++;
            }
            break;

        case INFO_FADE_DOWN:


            if (luminance == lumTarget) {

                myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_PF), 0, 6 * _BUFFER_SIZE / 4);
                lumTarget = 0;
                wait = 1;
                infoPhase = INFO_FADEUP;
                planet = nextPlanet();
            }

            else {
                infoPhase = INFO_FADE_DOWN;
                wait++;
            }

            break;
        }
    }
}

// EOF