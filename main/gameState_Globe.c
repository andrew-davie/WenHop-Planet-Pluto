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
static int lastpd;

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


    infoPhase = 0;
    wait = 50;

    luminance = -15;
    lumTarget = 0;

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


const struct pi {

    unsigned char stars;
    const char *review;

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


// 2,"=\"Did not|realise|breathing was|compulsory|here and had|to be|reminded|twice,|signage is|inadequate.#",
// 1,"=\"Consumed|the|atmosphere|as a snack.|Two|secondary|digestive|organs have|since lodged|a formal|complaint.#",
//2,"=\"Tried to eat|the soil as a|starter and|was told this|is not done|here.|Different|culture.|Would have|appreciated|a heads up.#",
// 2,"=\"Asked a local|what time it|was and he|told me in a|unit I do not|have the|organs to|perceive.|Useless.#",
// 1,"=\"The heat|here sits in|the range my|species uses|for food|storage and I|kept|reflexively|trying to put|things in the|locals.#",
 //3,"=\"Was told the|piles were|'historical'|and I said|'historical|what' and|nobody|finished the|sentence.#",
//2,"=\"Moisture|stuck to my|outer layer|and I took|this as a|territorial|claim and|panicked.|Better|labelling|would help.#",
 //3,"=\"My translator|rendered|the local|language as|'medium|length sighs'|all visit. I'm|not entirely|sure that was|wrong.#",
//3,"=\"Asked a local|if the planet|had feelings.|No. Are you|sure. Yes. It|seems sad.|They walked|away. I think|I was right.#",
 //1,"=\"The planet|has a|breathable|atmosphere|in the loosest|possible|reading of|the word|'breathable'.#",
//1,"=\"Assumed the|atmosphere|was|decorative|and removed|my suit. This|was my|second|mistake,|after|booking.#",


// 2,"=\"Local|timekeeping|uses units|that match|nothing|astronomical.|I said so. A|human said|'yeah' and|walked away.#",
// 2,"=\"Asked where|all the|valuable|minerals had|gone and was|told 'away'|which is|technically|an answer.#",
// 3,"=\"Asked a local|if the smell|was natural or|man-made.|She said|'yes'. I have|thought about|this every|day since.#",
// 1,"=\"Asked if the|air was safe|and was told|'for some|definitions|of safe'|which my|species|processes as|a yes and is|not.#",
// 2,"=\"Asked what|grew here.|Told|'resentment'.|I said that|sounds|promising for|a cultural|visit. It was|not.#",
// 2,"=\"Asked if the|haze ever|cleared.|'Cleared of|what,' they|said. I|realised they|had never|seen their|planet. Nor|had I.#",
// 2,"=\"Stepped off|the ship and|joined the|local|ecosystem.|Took effort|to reverse.|Left me|feeling|implicated in|something.#",
// 2,"=\"Asked a local|what they|did for fun.|They stared|long enough|that I felt I|had said|something in|very poor|taste.#",
// 1,"=\"Emergency|card,|seventeen|steps. Step|one:|'reconsider'.|The rest:|increasingly|specific|ways to|leave.#",
// 2,"=\"The ocean|here is|technically|swimmable in|the sense|that you can|enter it and|exit it, the|question is|what you exit|as.#",



#else
3,"=\"The oceans|are warm|and the|things in|them are|also warm|and|friendly, in|the way that|means you|cannot|swim.#",
2,"=\"The locals|shed their|skin each|morning and|leave it in|the hall. I|greeted my|neighbour's|%shed& for|two days.#",
2,"=\"The locals|leave gaps in|speech for|you to fill. By|day one, I|had agreed|to several|things I had|not planned|on.#",
2,"=\"The locals|can see|sound and|asked me to|keep it|down. I said|I was not|making any,|and they|showed me|that I was.#",
2,"=\"Happiness|here is going|still. I kept|asking if|locals were|well, and|upset them,|which they|showed by|moving.#",
2,"=\"The locals|have many|words for|one type of|regret. I|used most|before|leaving, and|used them|correctly.#",
2,"=\"The|handshake|has a part|where you|pretend not|to know each|other. I|skipped that|part, and|things went|wrong.#",
2,"=\"Each object|has a name,|and must be|addressed|when|touched. I|caused|three|scenes|before lunch|on day one.#",
4,"=\"The locals|exhale what|I breathe,|and I exhale|what they|breathe, and|for one|afternoon|we were|very close.#",
2,"=\"Memory is|shared|here, and I|woke on day|two knowing|private|things about|strangers I|had not met.#",
1,"=\"The|currency is|smell-based.|I arrived|with nothing|of value, and|left owing a|scent debt I|am still|repaying.#",
1,"=\"Silence is|against local|law, and I|was fined|three times|on the first|day before I|grasped the|scale of the|problem.#",
2,"=\"Everyone|here has|three|shadows,|one of them|cold, and|mine kept|touching|theirs, and|this was|rude.#",
1,"=\"The food|here is|invisible and|you eat by|feel. I ate|the table my|first night|and was|charged for|the table.#",
2,"=\"Roads are in|the sky,|buildings are|underground,|and I used|both the|wrong way|for two|days.#",
1,"=\"The ground|changes|colour based|on mood, and|mine kept|showing|things I had|not planned|to share in|public.#",
2,"=\"The locals|are made of|sound, and I|kept walking|through|them, and|they kept|saying sorry,|which made|the problem|louder.#",
1,"=\"Each room|runs at a|different|speed, and I|was|somehow|early and|late for the|same meal.#",
1,"=\"The year is|three days|long. I|booked a|week,|attended|five new|years, and a|gift was|expected|each time.#",
2,"=\"The sun|rises from|the ground|and sets|upward, and|by day three|my sense of|down was a|rough guess.#",
2,"=\"Everything is|slightly left|of where it|appears. I|adapted on|day four,|went home,|and broke|things I had|owned for|years.#",
1,"=\"The air here|is edible,|and I had too|much on day|one and had|to lie down|for reasons|I could not|convey to|anyone|nearby.#",
3,"=\"The slow|part of the|day is called|rush hour, a|naming|decision|that has|clearly|never been|reviewed.#",
3,"=\"They|removed the|stimulant|from their|stimulant|drink, still|drink it, and|call this a|choice they|made|freely.#",
3,"=\"They say|how are you|to each|other as a|greeting, do|not want to|know, and|are alarmed|when told.#",
3,"=\"They keep a|room clean|and ready|for guests,|sit in a|worse room|on all other|days, and|call this|having a|lounge.#",
1,"=\"The cold|here has|texture, and|I was not|warned|about this,|and I do not|have the|right organs|for it.#",
1,"=\"The gas|here is|sentient in|patches, and|one of the|patches was|in my cabin|and had|strong views|about the|lighting.#",
2,"=\"Waved at my|reflection|for two days|before a|local|explained|what it was,|and I had to|rethink|several prior|greetings.#",
3,"=\"A human said|I looked|well. I said I|did not feel|well, and it|said no, you|look good. I|said I did|not feel|good and it|left.#",
2,"=\"Tried to eat|the colour|blue as it|looked|nutritious,|and a local|stopped me,|and I do not|know what|she thought|I was doing.#",
2,"=\"A human|laughed at|me, and I|read this as|a threat|display and|responded in|kind, and the|crowd that|formed did|not help.#",
2,"=\"Tried to buy|someone's|shadow, as it|was the best|one I had|seen, and|they said no|and did not|seem to|take it as a|kind remark.#",
3,"=\"Saw a human|walking by a|small animal|on a lead,|and was|unsure who|was in|charge until|the animal|made it|clear.#",
2,"=\"The shadow|followed me|the whole|time, and|when I|complained,|was told it|was mine,|which raised|more|questions.#",
1,"=\"The compost|bin in my|room was|sentient and|began making|requests on|day two, and|I did not|know how to|refuse.#",
1,"=\"The|checkout|time was|listed as|flexible, and|flexible|here means|fixed, and|also earlier|than I was|told.#",
1,"=\"The hot|spring was|listed as|relaxing, and|I have a|different|word for|what it was,|and that|word is not|relaxing.#",
3,"=\"A human told|me to have a|nice day,|and I said I|would try,|and it looked|at me as|though trying|was not the|expected|response.#",
1,"=\"The tour bus|had|seatbelts,|which I|thought was|standard,|until the|driver put|his on and|said right,|here we go.#",
3,"=\"Was handed|a leaflet of|things to do|here, and|the leaflet|had one|page, and|one of the|things listed|was leaving.#",
1,"=\"Time here is|measured in|moods, and|my shuttle|was due at|after the|sadness, and|I waited at|the wrong|feeling.#",
2,"=\"There is no|word for|stranger,|and I was|given a|family on|arrival, and|they had|views on how|I spent my|days.#",
1,"=\"The locals|change|colour to|speak, and I|have no such|ability, and|spent the|trip mute in|a way I have|not been|before.#",
2,"=\"A kind word|here causes|mild pain,|and I was|polite on day|one, and did|not connect|these two|facts until|well into day|two.#",
2,"=\"Things here|exist only in|pairs, and I|arrived|alone, and|was quietly|pitied by|everyone|for the|duration of|my stay.#",
1,"=\"The wildlife|is harmless|if you avoid|eye|contact, and|the wildlife|is|everywhere|and makes a|great deal|of it first.#",
2,"=\"The planet's|famous|silence is|broken only|by the|wildlife, and|the wildlife|is loud and|has not read|the|brochure.#",
2,"=\"The days|are eighteen|minutes. I|booked|three nights|and was|there for|under an|hour.#",
2,"=\"Three|moons, and|all of them|wrong, and I|do not know|what I|expected,|but it was|not this.#",
3,"=\"Everything|here has six|legs except|the things|with eight,|and I spent a|week trying|to find the|pattern, and|there is not|one.#",
2,"=\"The planet|smells of|something I|cannot name|but have|smelled|once|before, in a|situation I|do not wish|to revisit.#",
3,"=\"Lovely|staff,|wrong|planet, too|late to do|much about|it by the|time I|noticed.#",
4,"=\"Beautiful|planet,|would|return,|cannot|return, legal|reasons,|four stars.#",
2,"=\"Checked in|to what I|thought was|my room, and|it was a lift,|and I|unpacked|before the|lift|explained|itself.#",
2,"=\"Asked for an|early|wake-up|call, and|their early|and my early|were|several|hours apart|in the wrong|direction.#",
2,"=\"The vendor|said the|item was|rare and|local, and I|pointed out|every stall|had one, and|she said yes,|rare and|local.#",
3,"=\"The tourism|office gave|me a list of|things to do,|and one of|them was|the tourism|office, and|I was|already|there.#",
3,"=\"I asked a|local for the|best dish,|she named|one, I|ordered it,|she brought|it out, and|seemed to|have planned|all of this.#",
2,"=\"Asked what|the locals do|for fun, and|the silence|that|followed|was|technically|an answer.#",
1,"=\"The|welcome|pack|included a|breathing|guide, which|I assumed|was a joke,|and it was|not a joke.#",
1,"=\"The exit sign|pointed at|the ocean,|and the|ocean had no|exit, very|poor planning.#",
2,"=\"Ordered|breakfast,|it arrived at|dinner,|dinner|arrived the|following|week, I left|before|lunch.#",
1,"=\"Stepped off|the ship and|became part|of the local|food chain|within|minutes.#",
2,"=\"Held my|umbrella the|wrong way|for two days|before|learning the|rain falls|down here.#",
2,"=\"The festival|was|described|as lively, and|involved|twelve|people|standing|near a thing.#",
2,"=\"The planet|leans, and|every local I|told looked|genuinely|unsure if|that was|true.#",
2,"=\"The|wake-up call|said only it is|time, and|did not say|time for|what.#",
1,"=\"The local|dish arrived|alive and|looking at|me, and I|did not know|the correct|response.#",
2,"=\"The upgrade|was to a|larger pile,|and I said|thank you|because I|did not know|what else to|say.#",
2,"=\"Asked what|grows here,|was told|resentment,|booked the|culture|tour, it was|about|resentment.#",
1,"=\"The planet|hums at a|pitch my|species|reads as|grief, and I|cried for|five days|before|making the|connection.#",
1,"=\"The hotel|stars were|awarded by|the hotel,|which is a|system with|a visible|flaw.#",
1,"=\"My suit's air|monitor|skipped|numbers and|went|straight to a|small picture|of something|dying.#",
1,"=\"The locals|said the|smell was|part of the|culture, and|I said which|part, and|they said all|of it.#",
2,"=\"Watched a|human argue|with a small|glowing|rectangle|while sitting|next to|someone|they|ignored.#",
1,"=\"The trail|was marked|safe, and|safe here|means a|different|thing than it|means|where I am|from.#",
1,"=\"Paid for the|premium|tour and the|standard|tour, and|they were|the same|tour with a|different|hat on the|guide.#",
2,"=\"The|nightlife|starts at|dusk, ends|shortly|after dusk,|and|everyone|goes home|and sighs.#",
1,"=\"The|currency is|based on|something I|will not|name, but I|had more of|it after a|long trip, and|that felt|unfair.#",
2,"=\"The humans|sleep for a|third of|their lives,|and built all|their|systems|around this,|which|explains the|systems.#",
2,"=\"The dig tour|lets you|uncover|layers of|past life,|and each|layer is|worse than|the one|above it.#",
1,"=\"The guided|walk was|described|as gentle,|and nature|had a|different|idea about|what gentle|means.#",
2,"=\"The locals|have thirty|words for|types of rain|and none for|leaving,|which tells|you|something.#",
1,"=\"Asked if the|haze ever|clears, and|the local|said clears|of what, and|I|understood|she had|never seen|the planet.#",
1,"=\"The thermal|pool was not|a pool in any|sense I had|prepared|for.#",
1,"=\"The beach|looked|perfect in|photos, and|photos were|the right|way to|experience|it.#",
2,"=\"The scenic|flyby is good|if you like|gas, which I|did not know|I did not like|until I was|inside it.#",
1,"=\"The kids|here have a|look in their|eyes that I|have thought|about every|day since I|left.#",
2,"=\"The two|moons orbit|at a|distance|that|suggests|they are|trying to|stay out of|things.#",
2,"=\"Asked a local|what to see,|and she|began|thinking|about it and|was still|thinking|when I left.#",
2,"=\"Asked a local|how long|they had|lived here,|and she said|longer than|she meant|to, and|looked at|the horizon.#",
3,"=\"Tried to|explain to a|human that|their planet|is unusual,|and she said|we like to|think we are|special, and|I said I|know.#",
1,"=\"Asked what|the big pile|was and was|told it was|the|economy,|and I did not|follow up.#",
2,"=\"My photos|came out|entirely|grey, and|the locals|said that|was the best|the planet|had looked.#",
2,"=\"Discovered|customer|service,|which is|humans|saying sorry|for things|they will|immediately|do again.#",
1,"=\"The ocean|laps the|shore in a|way that|sounds like it|is sighing at|you|personally.#",
2,"=\"The volcano|tour was|cancelled|due to the|volcano, a|sentence I|could not|have|predicted|needing.#",
1,"=\"Travel|insurance|listed this|planet by|name in the|exclusions,|and I booked|anyway, my|mistake.#",
1,"=\"Checked the|reviews|before|going, they|all said it|was fine,|those|reviewers|have given|up.#",
2,"=\"The tourist|board slogan|is %WE ARE|HERE&, and|that is the|full extent|of the pitch.#",
1,"=\"The planet|hums a note|not quite any|note I know,|and my|species has|a word for|that note,|and the|word means|leave.#",
1,"=\"A local|sneezed,|and I replied|in kind, and|it looked|alarmed, and|I had said|something I|will not|repeat here.#",
2,"=\"Clapped|after the|lightning, as|is polite at|home, and|several|locals moved|away before|I could|explain.#",
2,"=\"The|fireworks|looked like|an attack,|and I|responded,|and the|response to|my response|was not what|I hoped for.#",
2,"=\"Mistook the|warning|siren for a|welcome|song, and|sang back|for twenty|minutes|before|someone|intervened.#",
2,"=\"Assumed the|dog was the|local leader|based on how|everyone|treated it,|and spent|two days|following it|before being|corrected.#",
1,"=\"The phone|box looked|like a|transport|pod, and by|the time I|understood|it was not, I|had pressed|everything.#",
1,"=\"My ship's|navigation|refused to|save this|location to|favourites,|and I|understand|now it was|trying to|help me.#",
1,"=\"The locals|shed opinions|the way|others shed|skin, and|several|stuck to me|by day|three, and I|could not put|them down.#",
1,"=\"The local|trees finish|your|thoughts,|and some of|mine were|not ones I|would have|chosen to|share with a|tree.#",
2,"=\"The surface|is lovely|from orbit,|which is also|the viewing|distance the|tourism|board|suggests,|buried on|page nine.#",
1,"=\"The planet|has no native|predators,|which is|true, and|the wildlife|found other|solutions the|brochure|does not|address.#",
2,"=\"Asked for|directions|and told to|follow my|nose, which|I do not|have, and|was told to|follow it|anyway.#",
3,"=\"Was told to|make myself|at home, and|did, and was|asked to|leave, and|learned|there is a|gap between|the two.#",
3,"=\"The locals|have|seventeen|senses and|explained|the planet|using all of|them, and I|have five|and missed|most.#",
2,"=\"Landed,|explored,|ate|something,|left. Three|of those|four I would|not do again,|and I will not|say which|three.#",
3,"=\"Misread the|entry form,|declared|myself|luggage, and|spent day|one in a|storage bay|nicer than|my hotel.#",
1,"=\"The ranger|said stay on|the path,|and when I|asked what|was off it,|she said she|would rather|not say.#",
1,"=\"The guide|pointed at|something,|said there it|is, and|walked on|before I|could see|what it was.#",
1,"=\"The guide|was|excellent,|then said|this is where|I turn back,|and did, and|I had no map.#",
1,"=\"The|attendant|said the|wildlife was|friendly,|and when I|asked how|friendly,|she paused|in a way that|answered.#",
2,"=\"Asked how|far the|viewpoint|was, and he|said not far,|and walked|with me for|four hours,|and still|called it not|far.#",
4,"=\"Have a full|account of|this place,|but the box|provided is|too small,|and I have|run out of|time to find|a bigger one.#",
2,"=\"The humans|share|information|by vibrating|their|face-holes.|Impressive|but needless|given that|telepathy|exists.#",
2,"=\"The humans|share|information|by vibrating|their|face-holes.|Impressive|but needless|given that|telepathy|exists.#",
3,"=\"They pay for|water in a|bottle when|it falls free|from the|sky, complain|when it|falls, and|both are|considered|normal.#",
3,"=\"A local told|me to take|care when I|left. I asked|of what. She|said just in|general. I|said that is a|lot to take|care of.#",
2,"=\"Joined what|I thought|was a guided|walk. It was|a funeral. By|the end I|was a|pallbearer|and did not|know how to|raise this.#",
2,"=\"I asked what|not to miss.|She said|everything. I|asked what|she liked.|She said she|had not|thought|about it.#",
3,"=\"Asked a|ranger which|trail was|best. He said|'It depends'.|I said|'scenery'.|He said 'then|not this|planet'.#",

// vetted...

3,"=\"The planet is|beautiful|from orbit,|and less so|up close, and|the|brochure|was taken|from orbit.#",
2,"=\"The locals|have no word|for %no& and|express it by|doing the|thing|anyway, but,|sadly, I|missed this|for three|days.#",
2,"=\"The planet|has a smell|that changes|with your|thoughts. I|could not|stop thinking|about the|smell, and it|kept|changing.#",

#endif

    // clang-format on
};


bool seen[sizeof(planetInfo) / sizeof(planetInfo[0])];


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

    for (unsigned int i = 0; i < sizeof(planetInfo) / sizeof(planetInfo[0]); i++)
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


enum info {

    INFO_FADEUP,
    INFO_NAME,
    INFO_PHYSICS,
    INFO_CLEAR1,
    INFO_PLANETADVISOR,
    INFO_PLANETADVISOR_UNDERLINE,
    INFO_INFO,
    INFO_RATING2,
    INFO_CLEAR,
    INFO_NEXTPLANET,
    INFO_FADE_DOWN,
};


void VB_Globe() {


    adjustLuminance();
    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    drawPlanet(5);

    for (unsigned int i = 0; i < 10; i++)    // sizeof(stars) / sizeof(stars[0]); i++)
        drawBit2(stars[i].x >> 8, stars[i].y >> 8, stars[i].colour);

    // unsigned char *col = RAM + _BUF_GLOBE_COLUP0;
    // for (int i = 0; i < _SCANLINES; i++)
    //     col[i] = convertColour(i);
}


static int pif;
static int lines;
static int pa_lines;

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
                                _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - FONTLARGE_FONT_HEIGHT - 25);

            wait = 10;
            break;

        case INFO_PHYSICS:
            initAsciiStringDraw(FONT_COMPACT, 0x16, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                                planets[planet].physics, 0, _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - 25 + 5);
            wait = 200;
            // pic = ((rangeRandom(15) + 1) << 4) | 8;


            int lastpif = pif;
            pif = rangeRandom(MAX_PLANET);

            int reviews = sizeof(planetInfo) / sizeof(planetInfo[0]);
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
            wait = 30;
            break;

        case INFO_PLANETADVISOR:

            pa_lines = ((_SCANLINES - 1 - lines * FONTCOMPACT_FONT_HEIGHT) >> 1) - 25;


            initAsciiStringDraw(FONT_COMPACT, 0x8, 2, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                                "=PlanetAdvisor", 0, pa_lines);
            wait = 1;
            break;


        case INFO_PLANETADVISOR_UNDERLINE:

            initAsciiStringDraw(FONT_COMPACT, 0x8, 1, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                                "=_____________", 0, pa_lines);
            wait = 1;
            break;


        case INFO_INFO: {
            initAsciiStringDraw(FONT_COMPACT, 0xD8, 4, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planetInfo[pif].review, 0,
                                ((_SCANLINES - 1 - ((lines * FONTCOMPACT_FONT_HEIGHT))) >> 1));
            wait = 50;
            break;
        }

        case INFO_RATING2:
            pa_lines = ((_SCANLINES - 1 + lines * FONTCOMPACT_FONT_HEIGHT) >> 1) + 8;
            initAsciiStringDraw(FONT_COMPACT, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                                review[planetInfo[pif].stars], 0, pa_lines);
            wait = 150;
            break;


        case INFO_CLEAR:
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