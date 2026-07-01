#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "draw.h"
#include "drawPlanet.h"
#include "drawplanet.h"
#include "main.h"
#include "planet.h"
#include "random.h"
#include "sound.h"

#include "../gfx/fontcompact.h"
#include "../gfx/fontlarge.h"

static int pif;
static int lines;
static int pa_lines;
static int infoPhase;
int wait;
static int planet;

const unsigned char *thePalette;


enum info {

    INFO_FADEUP,
    INFO_NAME,
    INFO_PHYSICS,
    INFO_CLEAR1,
    INFO_PLANETADVISOR,
    INFO_INFO,
    INFO_RATING1,
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

    planet = 0;
    initPlanet(planet);

    infoPhase = INFO_FADEUP;
    wait = 50;

    luminance = -15;
    lumTarget = 0;

    sound_volume = VOLUME_PLAYING;

    const int speed = 0x40; //rangeRandom(80) + 16;

    loadTrack(10, track1b, 40, speed, 1);
    loadTrack(0, track2b, 25, speed, 2);
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


const struct {

    const char *review;
    unsigned char stars;

} planetInfo[] = {

// clang-format off

    // Control characters listed below
    // To print any control character, escape with backslash character (e.g, '\\=') in the string

    // \\  escape next character (a single backslash, but needs 2 inside a c string)
    // =  center following lines
    // >  right-justify following lines
    // <  left-justify following lines
    // #  right-close-quote
    // \"  left-open-quote (required by C)
    // |  next line
    // some non-alpha chars may not have shapes!

#if 0


#else


{"=\"I asked|how far the|viewpoint|was, and he|said %not far&,|and walked|with me for|four hours.|He still|called it|%not far&.#",2},
{"=\"I asked if the|air was safe|and was told|%for some|definitions|of safe&,|which my|species|processes as|a %yes&,|and it is _not_.#",1},
{"=\"I asked if the|haze ever|clears.|%Clears of|what?&, they|asked.|I realised|they had|never seen|their|planet.#",2},
{"=\"I asked|what grows|here.|I was told|%resentment&.|Booked the|cultural tour.|It was about|resentment.#",2},
{"=\"I asked|what not|to miss.|She said|everything.|I asked what|she liked.|She said she|had not|thought|about it.#",2},
{"=\"I asked|what the|locals do|for fun.|The silence|that|followed|was|technically|an answer.#",2},
{"=\"I asked|what the big|pile was, and|was told it|was the|economy.|I did not|follow up.#",1},
{"=\"I asked|where all the|valuable|minerals had|gone, and was|told %away&|which is|technically|correct.#",2},
{"=\"Landed,|explored,|ate|something,|left. Three|of those|four I would|not do again.#",2},
{"=\"Lovely|staff,|wrong|planet, too|late to do|much about|it by the|time I|noticed.#",3},
{"=\"Memory is|shared|here, and I|woke on day|two knowing|private|things about|strangers I|had not met.#",2},
{"=\"Misread|the entry|form, and|declared|myself|as luggage.|I spent day|one in a|storage bay|nicer than|my hotel.#",3},
{"=\"Mistook the|warning|siren for a|welcome|song, and|sang back|for twenty|minutes|before|someone|intervened.#",2},
{"=\"Moisture|stuck to my|outer layer|and I took|this as a|territorial|claim and|panicked.|Better|labelling|would help.#",2},
{"=\"My photos|came out|entirely|grey, and|the locals|said that|was the best|the planet|had looked.#",2},
{"=\"My ship's|navigation|refused to|save this|location to|favourites,|and I|understand|now it was|trying to|help me.#",1},
{"=\"My suit's air|monitor|skipped|numbers and|went|straight to a|small picture|of something|dying.#",1},
{"=\"My translator|rendered|the local|language as|'medium|length sighs'|all visit. I'm|not entirely|sure that was|wrong.#",3},
{"=\"Ordered|breakfast,|it arrived at|dinner,|dinner|arrived the|following|week, I left|before|lunch.#",2},
{"=\"Paid for the|premium|tour.|It was the|same as the|standard tour,|but with an|angry guide.#",1},
{"=\"Roads are in|the sky,|buildings are|underground.|I used|both the|wrong way|for two|days.#",2},
{"=\"Saw a human|walking by a|small animal|on a lead,|and was|unsure who|was in|charge until|the animal|made it|clear.#",3},
{"=\"Silence is|against local|law, and I|was fined|three times|on the first|day before I|grasped the|scale of the|problem.#",1},
{"=\"Stepped off|the ship and|became part|of the local|food chain|within|minutes.#",1},
{"=\"Stepped off|the ship and|joined the|local|ecosystem.|Took effort|to reverse.|Left me|feeling|implicated in|something.#",2},
{"=\"The air here|is edible,|and I had too|much on day|one and had|to lie down|for reasons|I could not|convey to|anyone|nearby.#",1},
{"=\"The beach|looked|perfect in|photos.|Photos were|the right|way to|experience|it.#",1},
{"=\"The cold|here has|texture.|I was not|warned|about this.|I do not|have the|right organs|for it.#",1},
{"=\"The compost|bin in my|room was|sentient and|began making|requests on|day two.|I did not|know how to|refuse.#",1},
{"=\"The days|are eighteen|minutes. I|booked|three nights|and was|there for|under an|hour.#",2},
{"=\"The dig tour|lets you|uncover|layers of|past life,|and each|layer is|worse than|the one|above it.#",2},
{"=\"The exit|sign pointed|at the ocean,|and the ocean|had no exit.|Very poor|planning.#",1},
{"=\"The festival|was|described|as lively.|It involved|twelve|people|standing|near a thing.#",2},
{"=\"The food|here is|invisible and|you eat by|feel. I ate|the table my|first night|and was|charged for|the table.#",1},
{"=\"The gas|here is|sentient in|patches.|One of the|patches was|in my cabin|and had|strong views|about the|lighting.#",1},
{"=\"The ground|changes|colour based|on mood, and|mine kept|showing|things I had|not planned|to share in|public.#",1},
{"=\"The guide|pointed at|something,|said there it|is, and|walked on|before I|could see|what it was.#",1},
{"=\"The guide|was|excellent,|then said|this is where|I turn back,|and did.|I had no map.#",1},
{"=\"The heat|here sits in|the range my|species uses|for food|storage and I|kept|reflexively|trying to put|things in the|locals.#",1},
{"=\"The hot|spring was|listed as|relaxing.|I have a|different|word for|what it was,|and that|word is not|%relaxing&.#",1},
{"=\"The hotel|stars were|awarded by|the hotel,|which is a|system with|a visible|flaw.#",1},
{"=\"The humans|share|information|by vibrating|their|face-holes.|Impressive|but needless|given that|telepathy|exists.#",2},
{"=\"The kids|here have a|look in their|eyes that I|have thought|about every|day since I|left.#",1},
{"=\"The local|dish arrived|alive and|looking at|me, and I|did not know|the correct|response.#",1},
{"=\"The locals|are made of|sound, and I|kept walking|through|them.|They kept|saying sorry,|which made|the problem|louder.#",2},
{"=\"The locals|can see|sound and|asked me to|keep it down.|I said|I was not|making any,|and they|showed me|that I was.#",2},
{"=\"The locals|change|colour to|speak, and I|have no such|ability.|I spent the|trip mute in|a way I have|not been|before.#",1},
{"=\"The locals|exhale what|I breathe,|and I exhale|what they|breathe.|For one|afternoon|we were|very close.#",4},
{"=\"The locals|have many|words for|one type of|regret.|I used most|before|leaving, and|used them|correctly.#",2},
{"=\"The locals|have no _word_|for %no& and|express it by|doing the|thing|anyway, but,|sadly, I|missed this|for three|days.#",2},
{"=\"The locals|have thirty|words for|types of rain|and none for|leaving,|which tells|you|something.#",2},
{"=\"The locals|have|seventeen|senses and|explained|the planet|using all of|them.|I have five|and missed|most of it.#",3},
{"=\"The locals|leave gaps in|speech for|you to fill. By|day one, I|had agreed|to several|things I had|not planned|on.#",2},
{"=\"The locals|said the|smell was|part of the|culture.|I said which|part, and|they said|%all of it&.#",1},
{"=\"The locals|shed opinions|the way|others shed|skin.|Several|stuck to me|by day|three, and I|could not put|them down.#",1},
{"=\"The locals|shed their|skin each|morning and|leave it in|the hall. I|greeted my|neighbour's|%shed& for|two days.#",2},
{"=\"The ocean|here is|technically|swimmable, in|the sense|that you can|enter it and|exit it.|The question|is what you|exit as.#",2},
{"=\"The ocean|laps the|shore in a|way that|sounds like it|is sighing at|you,|personally.#",1},
{"=\"The oceans|are warm|and the|things in|them are|also warm|and|friendly, in|the way that|means you|cannot|swim.#",3},
{"=\"The phone|box looked|like a|transport|pod, and by|the time I|understood|it was not, I|had pressed|everything.#",1},
{"=\"The planet is|beautiful|from orbit,|and less so|up close.|The|brochure|was taken|from orbit.#",3},
{"=\"The planet's|famous|silence is|broken only|by the|wildlife.|The wildlife|is loud and|has not read|the|brochure.#",2},
{"=\"The planet|has a smell|that changes|with your|thoughts.|I could not|stop thinking|about the|smell.|It kept|changing.#",2},
{"=\"The planet|has a|breathable|atmosphere|in the loosest|possible|reading of|the word|'breathable'.#",1},
{"=\"The planet|has no native|predators,|which is|true.|The wildlife|found other|solutions the|brochure|does not|address.#",1},
{"=\"The planet|hums a note|not quite any|note I know,|and my|species has|a word for|that note,|and the|word means|_leave_.#",1},
{"=\"The planet|hums at a|pitch my|species|reads as grief.|I cried for|five days|before|making the|connection.#",1},
{"=\"The planet|leans, and|every local I|told looked|genuinely|unsure if|that was|true.#",2},
{"=\"The planet|smells of|something I|cannot name|but have|smelled|once|before, in a|situation I|do not wish|to revisit.#",2},
{"=\"The ranger|said stay on|the path,|and when I|asked what|was off it,|she said she|would rather|not say.#",1},
{"=\"The scenic|flyby is good|if you like|gas, which I|did not know|I did not like|until I was|inside it.#",2},
{"=\"The shadow|followed me|the whole|time.|When I|complained, I|was told it|was mine.|This only|raised more|questions.#",2},
{"=\"The slow|part of the|day is called|rush hour, a|naming|decision|that has|clearly|never been|reviewed.#",3},
{"=\"The sun|rises from|the ground|and sets|upward.|By day three|my sense of|down was a|rough guess.#",2},
{"=\"The surface|is lovely|from orbit,|which is also|the viewing|distance the|tourism|board|suggests,|buried on|page nine.#",2},
{"=\"The thermal|pool was not|a pool in any|sense I had|prepared|for.#",1},
{"=\"The tour bus|had|seatbelts,|which I|thought was|standard,|until the|driver put|his on and|said right,|here we go.#",1},
{"=\"The tourism|office gave|me a list of|things to do.|One of|them was|the tourism|office.|I was|already|there.#",3},
{"=\"The trail|was marked|safe, and|safe here|means a|different|thing than it|means|where I am|from.#",1},
{"=\"The two|moons orbit|at a|distance|that|suggests|they are|trying to|stay out of|things.#",2},
{"=\"The upgrade|was to a|larger pile,|and I said|thank you|because I|did not know|what else to|say.#",2},
{"=\"The vendor|said the|item was|rare and|local.|I pointed out|every stall|had one, and|she said yes,|rare and|local.#",2},
{"=\"The volcano|tour was|cancelled|due to the|volcano, a|sentence I|could not|have|predicted|needing.#",2},
{"=\"The wildlife|is harmless|if you avoid|eye contact.|It is also|everywhere|and makes a|great deal of|eye contact|first.#",1},
{"=\"The year is|three days|long.|I booked a|week, and|attended|five new-year|celebrations.|A gift was|expected|each time.#",1},
{"=\"The|attendant|said the|wildlife was|friendly,|and when I|asked how|friendly,|she paused|in a way that|answered.#",1},
{"=\"The|checkout|time was|listed as|flexible, and|flexible|here means|fixed, and|also earlier|than I was|told.#",1},
{"=\"The|currency is|based on|something I|will not|name, but I|had more of|it after a|long trip.|That felt|unfair.#",1},
{"=\"The|currency is|smell-based.|I arrived|with nothing|of value, and|left owing a|scent debt I|am still|repaying.#",1},
{"=\"The|fireworks|looked like|an attack,|and I|responded,|and the|response to|my response|was not what|I hoped for.#",2},
{"=\"The|handshake|has a part|where you|pretend not|to know each|other.|I skipped that|part, and|things went|wrong.#",2},
{"=\"The|nightlife|starts at|dusk, ends|shortly|after dusk,|and|everyone|goes home|and sighs.#",2},
{"=\"The|wake-up call|said only it is|time, and|did not say|time for|what.#",2},
{"=\"The|welcome|pack|included a|breathing|guide, which|I assumed|was a joke.|~It was not|a joke.#",1},
{"=\"There is no|word for|stranger,|and I was|given a|family on|arrival.|They had|views on how|I spent my|days.#",2},
{"=\"They keep a|room clean|and ready|for guests,|sit in a|worse room|on all other|days, and|call this|having a|lounge.#",3},
{"=\"They pay for|water in a|bottle when|it falls free|from the|sky, complain|when it|falls.|Both are|considered|normal.#",3},
{"=\"They say|how are you|to each|other as a|greeting, do|not want to|know, and|are alarmed|when told.#",3},
{"=\"They|removed the|stimulant|from their|stimulant|drink, still|drink it, and|call this a|choice they|made|freely.#",3},
{"=\"Things here|exist only in|pairs.|I arrived|alone, and|was quietly|pitied by|everyone|for the|duration of|my stay.#",2},
{"=\"Three|moons, and|all of them|wrong, and I|do not know|what I|expected,|but it was|not this.#",2},
{"=\"Time here is|measured in|moods.|My shuttle|was due at|after the|sadness, and|I waited at|the wrong|feeling.#",1},
{"=\"Travel|insurance|listed this|planet by|name in the|exclusions,|and I booked|anyway, my|mistake.#",1},
{"=\"Tried to buy|someone's|shadow, as it|was the best|one I had|seen.|They said %no&|and did not|seem to|take it as a|kind remark.#",2},
{"=\"Tried to eat|the colour|blue as it|looked|nutritious,|and a local|stopped me,|and I do not|know what|she thought|I was doing.#",2},
{"=\"Tried to eat|the soil as a|starter and|was told this|is not done|here.|Different|culture.|Would have|appreciated|a heads-up.#",2},
{"=\"Tried to|explain to a|human that|their planet|is unusual,|and she said|%we like to|think we are|special&.|I said|%know&.#",3},
{"=\"Was handed|a leaflet of|things to do|here.|The leaflet|had one|page, and|one of the|things listed|was leaving.#",3},
{"=\"Was told the|piles were|'historical'|and I said|'historical|what?'.|Nobody|finished the|sentence.#",3},
{"=\"Was told to|make myself|at home, and|did, and was|asked to|leave, and|learned|there is a|gap between|the two.#",3},
{"=\"Watched a|human argue|with a small|glowing|rectangle|while sitting|next to|someone|they|ignored.#",2},
{"=\"Waved at my|reflection|for two days|before a|local|explained|what it was,|and I had to|rethink|several prior|greetings.#",2},


// 0

// 1
{"=\"I asked a|local for the|best dish,|she named|one.|I ordered it,|she brought|it out, and|seemed to|have planned|all of this.#",3},

// 2
{"=\"Ate some|atmosphere|as a snack.|Two|secondary|digestive|organs have|since lodged|a formal|complaint.#",1},

// 3

// 4
{"=\"Assumed|the dog was|the local|leader.|Spent two|days|following it|before|someone|corrected|me.#",2},
{"=\"Local|timekeeping|uses units|that match|nothing|astronomical.|I said so.|A human said|%yeah&, and|walked|away.#",2},
{"=\"The local|trees finish|your|thoughts.|Some of|mine were|not ones I|would have|chosen to|share with a|tree.#",1},
{"=\"The|humans sleep|for a third|of their lives,|and built all|their|systems|around this,|which|explains the|systems.#",2},
{"=\"Discovered|%customer|service&,|which is|humans|saying %sorry&|for things|they will|immediately|do again.#",2},
{"=\"Checked|in to what I|thought was|my room.|It was a lift,|and I|unpacked|before the|lift|explained|itself.#",2},

// 5
{"=\"Each|room runs|at a|different|speed.|I was|somehow|early and|late for the|same|meal.#",1},
{"=\"A kind word|here causes|mild pain.|I was polite|from day one,|and did not|connect|these two|facts until|well into day|two.#",2},
{"=\"Clapped|after the|lightning, as|is polite at|home.|Several|locals moved|away before|I could|explain.#",2},
{"=\"Checked the|reviews|before|going, they|all said it|was fine.|Those|reviewers|have given|up.#",1},
{"=\"The|guided walk|was described|as gentle.|Nature|had a|different|idea about|what gentle|means.#",1},
{"=\"The|tourist|board slogan|is...|WE~ ARE|~HERE.|That is the|full extent|of the|pitch.#",2},
{"=\"I asked|for|directions|and was told|to follow my|nose, which|I do not|have.|I was told to|follow it|anyway.#",2},

// 6
{"=\"Each object|has a name,|and must be|addressed|when|touched.|I caused|three|scenes|before lunch|on day one.#",2},
{"=\"A human said|I looked|well. I said I|did not feel|well, and it|said no, you|look good. I|said I did|not feel|good, and it|left.#",3},
{"=\"A human|laughed at|me, and I|read this as|a threat|display.|I responded in|kind. The|crowd that|formed did|not help.#",2},
{"=\"Assumed|the|atmosphere|was|decorative|and removed|my suit.|This was my|second|mistake,|after|booking.#",1},
{"=\"Joined|what I|thought was a|guided walk.|It was|a funeral.|By the end I|was a|pallbearer|and did not|know how to|raise this.#",2},
{"=\"Things|here have six|legs except|the things|with eight.|I spent a|week trying|to find the|pattern.|There is not|one.#",3},
{"=\"I asked a|local what|they did|for fun.|They stared|long enough|that I felt I|had said|something in|very poor|taste.#",2},
{"=\"I asked|for an early|wake-up|call.|Their early|and my early|were|several|hours apart.|In the wrong|direction.#",2},

// 7
{"=\"Everyone|here has|three|shadows,|one of them|cold.|Mine kept|touching|theirs, and|this was|rude.#",2},
{"=\"The|emergency|card.|Step|one:|'reconsider'.|The rest:|increasingly|specific|ways to|leave.#",1},
{"=\"A human|told me to|have a nice|day, and I|said I would|try.|It looked|at me as|though trying|was not the|expected|response.#",3},
{"=\"Beautiful|planet,|would|return.|Can't return|for legal|reasons.#",4},
{"=\"Things|are slightly|left of where|they appear.|I adapted on|day four,|went home,|and broke|things I had|owned for|years.#",2},
{"=\"Here,|happiness|is going still.|I kept|asking if|locals were|well, and|upset them,|which they|showed by|moving.#",2},
{"=\"Made an|amazing|discovery|on this planet.|The box|provided is|too small|to detail|it here.#",4},
{"=\"I asked a|local if|the planet|had feelings.|No. Are you|sure? Yes.|It seems sad.|They walked|away. I think|I was right.#",3},
{"=\"I asked a|local|what to see,|and she|began|thinking|about it.|She was still|thinking|when I|left.#",2},

// 8
{"=\"A local|told me to|take care|when I left.|I asked|of what.|She said just|in general.|I said that is|a lot to take|care of.#",3},
{"=\"Did not|realise|breathing was|compulsory|here.|Had to be|reminded|twice.|Signage is|inadequate.#",2},
{"=\"Held my|umbrella the|wrong way|for two days|before|learning the|rain falls|down here.#",2},
{"=\"I asked a|local how|long they had|lived here.|It said|longer than|it meant|to, and|stared at|the horizon.#",2},
{"=\"I asked a|local what|time it was,|and he|told me in a|unit I do not|have the|organs to|perceive.|Useless.#",2},
{"=\"I asked a|ranger which|trail was|best. He said|%It depends&.|I said|%scenery&.|He said %then|not this|planet&.#",3},

// 9
{"=\"I asked a|local if the|_smell_|was natural|or man-made.|She said|%yes&.|I have|thought about|this every|day since.#",3},


// 10


#endif

    // clang-format on
};

#define PIS (sizeof(planetInfo) / sizeof(planetInfo[0]))

bool seen[PIS];


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


void VB_Globe() {


    adjustLuminance();
    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    drawPlanet(5);
    drawStars();

    static int nsv = VOLUME_PLAYING;

    if (!rangeRandom(1000))
        nsv = rangeRandom(sound_max_volume);

    sound_volume = step_toward(sound_volume, nsv);

    if (sound_volume < 0)
        sound_volume = 0;
    if (sound_volume > sound_max_volume)
        sound_volume = sound_max_volume;
}


void OS_Globe() {

#define ADJ 6

    interleaveChronoColour(&roller);

    drawPlanet(0);

    if (!drawNextChar() && --wait < 0) {

        switch (infoPhase++) {

        case INFO_FADEUP:
            if (luminance)
                infoPhase = INFO_FADEUP;
            break;


        case INFO_NAME:

            drawString(FONT_LARGE, 0x8, 10, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                       planets[planet].name, 0,
                       _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - FONTLARGE_FONT_HEIGHT - 25 + ADJ);
            wait = 10;
            break;


        case INFO_PHYSICS:
            drawString(FONT_COMPACT, 0x16, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                       planets[planet].physics, 0,                                  //
                       _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - 25 + 5 + ADJ);
            wait = 200;

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

                pif = 0;    // tmp

                seen[pif] = true;
            }

            lines = FONTCOMPACT_FONT_HEIGHT;
            const char *p = planetInfo[pif].review;
            do
                if (*p == '|') {
                    lines += FONTCOMPACT_FONT_HEIGHT;
                    if (*(p - 1) == '.')
                        lines += FONTCOMPACT_FONT_HEIGHT >> 1;
                }
            while (*++p);

            break;


        case INFO_CLEAR1:
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            ADDAUDIO(SFX_DOGE2);
            wait = 30;
            break;


        case INFO_PLANETADVISOR:

            pa_lines = ((_SCANLINES - lines) >> 1) + ADJ;


            drawString(FONT_COMPACT, 0x8, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                       "=_PlanetAdvisor", 0, pa_lines - 20);
            wait = 25;
            break;


        case INFO_INFO:
            drawString(FONT_COMPACT, 0xD8, 1, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                       planetInfo[pif].review, 0, pa_lines);
            wait = 50;
            break;


        case INFO_RATING1:
            pa_lines += lines + 8;
            drawString(FONT_COMPACT, 0x18, 0, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[0], 0, pa_lines);
            break;


        case INFO_RATING2:
            drawString(FONT_COMPACT, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                       review[planetInfo[pif].stars < 6 ? planetInfo[pif].stars : 5], 0, pa_lines);
            wait = 150;
            break;


        case INFO_CLEAR:
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            wait = 30;
            break;


        case INFO_NEXTPLANET:

            if (scalex > (SCALE_FAR - 0x1000) && planetDir > 0)
                lumTarget = -15;
            else
                infoPhase = INFO_NEXTPLANET;
            break;


        case INFO_FADE_DOWN:

            if (luminance == lumTarget) {

                myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_PF), 0, 6 * _BUFFER_SIZE / 4);
                lumTarget = 0;
                infoPhase = INFO_FADEUP;
                planet = nextPlanet();

            }

            else
                infoPhase = INFO_FADE_DOWN;

            break;
        }
    }
}

// EOF