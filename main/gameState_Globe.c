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

#if 1

//2,"=\"The humans|share|information|by vibrating|their|face-holes.|Impressive|but needless|given that|telepathy|exists.#",
// 3,"=\"They pay for|water in a|bottle when|it falls free|from the|sky, complain|when it|falls, and|both are|considered|normal.#",
// 3,"=\"A local told|me to take|care when I|left. I asked|of what. She|said just in|general. I|said that is a|lot to take|care of.#",
 //2,"=\"Joined what|I thought|was a guided|walk. It was|a funeral. By|the end I|was a|pallbearer|and did not|know how to|raise this.#",
 2,"=\"I asked what|not to miss.|She said|everything. I|asked what|she liked.|She said she|had not|thought|about it.#",
// 3,"=\"Asked a|ranger which|trail was|best. He said|'It depends'.|I said|'scenery'.|He said 'then|not this|park'.#",

#else
3,"=\"The oceans|are warm|and the|things in|them are|also warm|and|friendly, in|the way that|means you|cannot swim.#",
3,"=\"The planet is|beautiful|from orbit,|and less so|up close, and|the|brochure|was taken|from orbit.#",
2,"=\"The locals|have no word|for no and|express it by|doing the|thing|anyway, but,|sadly, I|missed this|for three|days.#",
2,"=\"The planet|has a smell|that changes|with your|thoughts. I|could not|stop thinking|about the|smell, and it|kept|changing.#",
2,"=\"The locals|shed their|skin each|morning and|leave it in|the hall. I|greeted my|neighbour's|shed for|two days.#",
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
////
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
#endif


    // clang-format on
};


bool seen[sizeof(planetInfo) / sizeof(planetInfo[0])];


/*


Earth — Tried to purchase the large yellow sky-fire but was told it was "the sun" and "not for sale" which seems like a
missed revenue opportunity. ★★★☆☆
Bloatworld — Did not realise gravity was optional until my third day, by which point I
had been walking on the ceiling the whole time and nobody said anything.
★★★☆☆ Spite — Attempted to communicate with the
frozen water formations as I do at home but they simply refused, very rude, one star. ★☆☆☆☆
 Pustula — The geysers
erupted on a schedule and I waited politely for them to finish before applauding, which I am told was incorrect but I
stand by my manners. ★★★☆☆
Dross — Brought my own star as is customary but the locals seemed alarmed and I had to leave
it in orbit which meant my accommodation was very dark. ★★☆☆☆
Fetid IX — I consumed the atmosphere as a snack and would
not recommend it, two of my secondary digestive organs have lodged a formal complaint. ★☆☆☆☆
Glumvast — The rocks here
do not respond to conversation no matter how loudly you speak into them, extremely poor infrastructure. ★★☆☆☆
 Noxis —
Tried to fold the planet's weather into my travel bag as a souvenir and my travel bag is still angry with me. ★★☆☆☆
Plod
— Did not understand that the planet moves and spent the whole trip chasing it, will plan better next time. ★★★☆☆
Earth
— The smaller humans kept asking for something called "candy" and when I produced the standard greeting mineral they
cried, very confusing, did not give five stars. ★★☆☆☆ Here we go — 100 reviews in the clueless alien visitor style:
---
**Earth** — Attempted to purchase the ocean as accommodation but was told it was "public" which seems like very poor
estate management for such a large property. ★★☆☆☆
**Bloatworld** — Did not understand why my body kept trying to go downward so I filed a complaint with the front desk
and they said that was "gravity" which is no excuse. ★★☆☆☆
**Spite** — Tried to turn the cold down but could not locate the dial, infrastructure clearly unfinished. ★☆☆☆☆
**Pustula** — Sat on what I thought was a rock and it sat back, gave it three stars for the company but minus two for
the surprise. ★★★☆☆
**Fetid IX** — Ate what I assumed was the local currency as is customary on my home world and was asked to leave the
bank, humiliating. ★☆☆☆☆
**Glumvast** — The sky here is one colour all the time, I tried adjusting it manually from my ship but the controls
don't reach that far, very poor design. ★★☆☆☆
**Dross** — Brought a gift for the planet as is polite and left it on the surface but the planet did not acknowledge
receipt, extremely rude. ★☆☆☆☆
**Noxis** — Did not realise breathing was compulsory on this planet and had to be reminded twice, signage is inadequate.
★★☆☆☆
**Plod** — Tried to speed the rotation up by pushing on the equator and pulled a muscle, no compensation offered. ★☆☆☆☆
**Earth** — The humans communicate by vibrating air with their face-holes and I found this both impressive and
unnecessary given that telepathy exists. ★★☆☆☆
**Skumveil** — Could not see anything due to the haze so I assumed the planet was empty and began filling out the
discovery paperwork, locals were furious. ★☆☆☆☆
**Maggrot** — Tried to eat the soil as a starter before the main meal arrived and was told soil is not a starter here,
very different culture, would have appreciated a heads up. ★★☆☆☆
**Oaf** — Kept bumping into the planet during my approach because it failed to move out of the way, basic orbital
courtesy completely absent. ★☆☆☆☆
**Grunthos** — Asked a local what time it was and he told me in a unit I do not have the organs to perceive, useless.
★★☆☆☆
**Swill** — Attempted to drain the ocean to see what was underneath and several locals became hostile before I had made
significant progress. ★☆☆☆☆
**Tepid** — The temperature here sits in the range my species uses for food storage and I kept reflexively trying to put
things in the locals. ★★☆☆☆
**Rankle** — Tried to purchase one of the smaller mountains as a souvenir and the locals said mountains are "not for
sale" which is exactly what someone who hadn't considered selling them would say. ★★☆☆☆
**Blight** — Nothing growing here so I planted several seeds from home as a courtesy and have since received a strongly
worded message from their government. ★☆☆☆☆
**Vomitron** — Assumed the name was a translation error and it was not. ★☆☆☆☆
**Filchmore** — My belongings were taken at customs for "inspection" and returned to me in smaller quantities, asked for
an explanation and was given one that did not help. ★☆☆☆☆
**Crud** — Tried to communicate with the ground as I do at home and the ground communicated back in a way I was not
prepared for. ★☆☆☆☆
**Dreg-7** — Did not realise gas giants have no surface until I was already committed to the landing approach, the
concept was never adequately explained in the brochure. ★☆☆☆☆
**Blundor** — Thought the locals were a weather formation for the first two days and sheltered from them accordingly, by
the time I understood my mistake it was too late to apologise meaningfully. ★★☆☆☆
**Scab** — Attempted to pick at the surface crust as is customary when inspecting real estate and the planet made a
noise at me. ★☆☆☆☆
**Midden-3** — Was told the piles were "historical" and I said "historical what" and nobody finished the sentence. ★☆☆☆☆
**Wretchard** — Tried to use the star as a light source in my room and was told that is not how rooms work here, very
limiting. ★★☆☆☆
**Earth** — Rented a "car" which is a ground-vehicle that moves slowly and makes noise and I kept waiting for it to fly
and it never flew and when I asked why it doesn't fly the human laughed for a very long time. ★★☆☆☆
**Fug** — The moisture here kept sticking to my outer layer and I assumed I was being claimed as territory and panicked
unnecessarily, better labelling would help. ★★☆☆☆
**Dredmore** — Noticed the planet leaning and attempted to correct it by redistributing weight across the surface, this
did not work and I am tired. ★☆☆☆☆
**Snivvel** — The precipitation here falls downward which is the opposite of home and I spent the entire first day
looking at the wrong sky. ★★☆☆☆
**Glump** — Tried to negotiate with the tectonic plates directly and they did not come to the table, literally, the
table just sank into one of them. ★☆☆☆☆
**Clot** — Got stuck in a traffic system and after three days realised I was going in a circle and after five days
realised everyone knew and nobody had mentioned it. ★☆☆☆☆
**Pustula** — The geysers here operate on a timer and I bought a ticket to the 3pm show and arrived at what I calculated
to be 3pm and was erupted upon, the timer and I disagree about when 3pm is. ★★☆☆☆
**Scum's Landing** — Attempted to claim the planet under exploration rights and was shown a plaque suggesting someone
had beaten me to it several centuries ago, very disappointing, the plaque did not even have my name on it. ★☆☆☆☆
**Drivel** — The locals speak in a language that my translator rendered as "medium-length sighs" for the entire visit
and I'm not entirely sure that was wrong. ★★☆☆☆
**Earth** — The buildings here go upward instead of downward which means their basements are at the top and the
foundations are at the bottom which seems structurally backwards to me. ★★★☆☆
**Bloatworld** — Tried to take some gravity home as a souvenir and packed it very carefully but it seems to have escaped
in transit because my luggage is floating. ★★☆☆☆
**Glumvast** — Asked a local if the planet had feelings and they said no and I said are you sure and they said yes and I
said because it seems sad and they walked away and I think I was right. ★★☆☆☆
**Fetid IX** — Complimented a local on their atmospheric production and they seemed offended, cultural misunderstanding,
I meant it sincerely. ★★★☆☆
**Noxis** — The planet has a breathable atmosphere in the loosest possible interpretation of the word "breathable."
★☆☆☆☆
**Oaf** — The planet rotates but only slowly and I kept having to reposition my chair to stay in the light, no staff
assistance was offered. ★★☆☆☆
**Spite** — Attempted to warm the planet up by reflecting sunlight from my ship's hull toward the surface for six hours
and achieved nothing except mild embarrassment. ★★☆☆☆
**Rankle** — Tried to eat the local starlight as is customary and was informed by my own digestive system that this was
incorrect, the planet's star is weaker than it looks. ★☆☆☆☆
**Gunk Prime** — The liquid here behaves like a solid and the solid here behaves like a liquid and I behaved like
someone who had made a terrible mistake, which was accurate. ★☆☆☆☆
**Maggrot** — Was informed after landing that the surface moves on its own and I said that sounds fine and it was not
fine, it moved a great deal, I am in a different location than I intended. ★☆☆☆☆
**Blight** — Tried to start a small fire for warmth using the local rocks and the local rocks refused on principle, I
didn't know rocks could do that. ★★☆☆☆
**Vomitron** — Assumed the atmosphere was decorative and removed my suit to appreciate the view more directly, this was
my second mistake of the trip after booking it. ★☆☆☆☆
**Earth** — The local timekeeping system divides the day into units that do not correspond to anything astronomical and
when I pointed this out a human said "yeah" and walked away. ★★☆☆☆
**Filchmore** — Left a tip at the restaurant as is polite and the tip was gone before I had stood up, very fast service,
unfortunately applied to the wrong thing. ★★☆☆☆
**Dross** — Asked where all the valuable minerals had gone and was told "away" which is technically an answer. ★★☆☆☆
**Tepid** — The locals described themselves as "warm" people and I took this literally and packed accordingly and was
underdressed in every sense. ★★☆☆☆
**Skumveil** — Navigated entirely by feel for three days before discovering I had been inside a building the whole time
and the "haze" was just the interior of a very large, poorly lit structure. ★☆☆☆☆
**Plod** — Miscalculated the orbital period and arrived 40 years before my booking, the hotel did not honour the early
check-in, unreasonable. ★☆☆☆☆
**Scab** — The surface texture here is described in the brochure as "characterful" which I now understand is a word used
when something is bad but the person saying it lives there. ★★☆☆☆
**Grunthos** — Attempted to pay with standard galactic credit and was asked if I had anything "real" and I said this is
real and they said is it though, which is not a legal question for a shop to ask. ★☆☆☆☆
**Swill** — Believed the ocean was a mirror due to the reflection and spent four hours trying to find the planet on the
other side of it. ★★☆☆☆
**Clot** — The roundabout system here operates on a logic I could not identify even after mapping it, I eventually
concluded there is no logic and the roundabouts are self-governing. ★☆☆☆☆
**Midden-3** — Asked a local if the smell was natural or man-made and she said "yes" and walked away, I have thought
about this every day since. ★☆☆☆☆
**Earth** — Observed a ritual called "rush hour" in which all the humans attempt to travel in the same direction at the
same time and seem surprised every day that this is difficult. ★★★☆☆
**Wretchard** — Bought travel insurance, read the exclusions list, found Wretchard explicitly named as a planet the
policy does not cover, booked anyway, deeply regret it. ★☆☆☆☆
**Glumvast** — The planet emits a low frequency hum that my species processes as sadness and I cried for six days and
only realised the cause on the shuttle home. ★☆☆☆☆
**Dredmore** — Attempted to correct the lean by moving to the high side of the planet but was only one passenger and it
did not make a measurable difference, would need a group booking to fix this. ★★☆☆☆
**Pustula** — Tried to book a geyser for a private event and was told they are not bookable and I said everything is
bookable and they said not the geysers and I said fine two stars. ★★☆☆☆
**Fetid IX** — The atmosphere is listed as "unique" in the brochure and my species has a different word for it and that
word is "evidence." ★☆☆☆☆
**Snivvel** — Tried to collect the rain as a souvenir and filled seventeen containers before a local explained that rain
is not a finite resource here and I had been collecting it for no reason for two days. ★★★☆☆
**Bloatworld** — The gravity here is listed as "strong" and I listed myself as "prepared" and we were both wrong about
something. ★☆☆☆☆
**Noxis** — Asked if the air was safe and was told "for some definitions of safe" which my species processes as a yes
and is not. ★☆☆☆☆
**Oaf** — The planet failed to greet me upon landing which every other planet I have visited has also failed to do but I
keep expecting it and Oaf felt like the most likely candidate. ★★☆☆☆
**Vomitron** — Brought a friend who had not read the reviews and when we landed I watched their face go through five
distinct phases and I have never felt so guilty. ★☆☆☆☆
**Rankle** — Attempted to climb the local star to get a better view and was prevented from doing so by what scientists
call "physics" and what I call "a personal attack." ★☆☆☆☆
**Blight** — Asked if anything grew here and was told "resentment" and I said that sounds promising for a cultural visit
and it was not. ★★☆☆☆
**Earth** — The humans have invented "queuing" which is standing in a line to wait for something and they are extremely
serious about it and will tell you off for doing it wrong and I respect this and this is the only thing I respect about
Earth. ★★★☆☆
**Gunk Prime** — Tried to take a photograph and the camera sank, tried to retrieve the camera and my arm sank, tried to
retrieve my arm and had to make difficult decisions. ★☆☆☆☆
**Crud** — The hotel room had a view of a wall and when I asked for a better room I was moved to a room with a view of a
different wall, both walls were bad. ★★☆☆☆
**Dreg-7** — Thought the gas giant was hollow and attempted to enter it from the top and kept going and am still not
entirely sure where I exited. ★☆☆☆☆
**Scum's Landing** — Asked what the founding crime was and was told it is celebrated every year at a festival and I said
what is the festival called and they said "The Festival" and that was that. ★★☆☆☆
**Filchmore** — Checked my pockets after leaving and found things in them that were not mine, which is either theft in
reverse or the most confusing form of gift-giving I have encountered. ★★☆☆☆
**Fug** — Tried to open a window to let some air in and was told the air outside is the same as the air inside and I
said then what is the window for and nobody answered. ★★☆☆☆
**Skumveil** — Asked a local if the haze ever cleared and they said "cleared of what" and I realised they had never seen
their own planet and did not know what was under the haze and neither do I. ★★☆☆☆
**Tepid** — Tried to start a conversation about the weather, which is my species' standard greeting, and the weather
here is so consistent that the conversation lasted four seconds and then there was silence for an hour. ★★☆☆☆
**Spite** — The planet has one warm spot, a single thermal vent on the southern continent, and the queue for it was
eighteen kilometres long. ★☆☆☆☆
**Maggrot** — Stepped off the ship and immediately became part of the ecosystem in a way that took considerable effort
to reverse and left me feeling implicated in something. ★☆☆☆☆
**Dross** — The museum of natural history contains nothing that is natural, historical, or in any sense a museum, it is
a shed. ★☆☆☆☆
**Blundor** — Ended up here after a navigation error and stayed for three weeks because I could not find the exit, not a
metaphor, there is literally only one way out and it is not marked. ★☆☆☆☆
**Earth** — The humans have a system called "money" which is points they give each other in exchange for things and the
points are not even real but they are extremely serious about the points and will do almost anything for points. ★★★★☆
**Midden-3** — Tried to organise a clean-up as a community service and the locals said the mess is load-bearing and I
said load-bearing what and they said the economy. ★☆☆☆☆
**Wretchard** — The departure lounge had one seat and it was taken and when I asked when it would be free the occupant
said they were also waiting for a departure and had been for eleven years. ★☆☆☆☆
**Glumvast** — Asked a local what they did for fun and they stared at me long enough that I started to feel I had said
something in very poor taste. ★★☆☆☆
**Plod** — The local year is so long that I arrived in spring and left in spring and did not experience any other season
despite being there for what felt like forever. ★★☆☆☆
**Clot** — The traffic system is sentient, I am now certain of this, and it is angry about something specific to me
personally. ★☆☆☆☆
**Pustula** — Attempted to rate the geysers while they were erupting and my reviewer device was destroyed, one star for
the principle of the thing. ★☆☆☆☆
**Fetid IX** — Left a candle burning in my hotel room to improve the ambience and was informed that an open flame in
that atmosphere was an act of war. ★☆☆☆☆
**Oaf** — The planet has a moon that is visibly embarrassed to be there and I felt a kinship with it that I have not
felt with anything else on this trip. ★★★☆☆
**Noxis** — The emergency information card in my room had seventeen steps and step one was "reconsider" and steps two
through seventeen were increasingly specific ways to leave. ★☆☆☆☆
**Earth** — Watched a human argue with a small glowing rectangle for forty minutes while sitting next to another human
they did not speak to and I found this deeply relatable in a way that worries me. ★★★☆☆
**Rankle** — Tried to diplomatically suggest the planet could improve its image with some simple renovations and
received a response from their government that my translator rendered as "how dare you" seventeen times in a row. ★☆☆☆☆
**Scab** — The walking tour covered twelve kilometres of terrain and my guide described all of it as "fine, mostly"
which I now understand was optimistic. ★★☆☆☆
**Swill** — The ocean here is technically swimmable in the sense that you can enter it and exit it, the question is what
you exit as. ★☆☆☆☆



*/

const char *review[] = {
    "=+++++",    // 0
    "=*++++",    // 1
    "=**+++",    // 2
    "=***++",    // 3
    "=****+",    // 4
    "=*****",    // 5
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

    INFO_NAME,
    INFO_PHYSICS,
    INFO_INFO,
    INFO_RATING1,
    INFO_RATING2,
    INFO_CLEAR,
    INFO_NEXTPLANET1,
    INFO_NEXTPLANET,
};


void VB_Globe() {

    initDataStreams_Globe();
    drawPaletteGlobe(thePalette);

    drawPlanet(5);

    for (unsigned int i = 0; i < 10; i++)    // sizeof(stars) / sizeof(stars[0]); i++)
        drawBit2(stars[i].x >> 8, stars[i].y >> 8, stars[i].colour);
}


static int pif;
static int lines;

void OS_Globe() {

    interleaveChronoColour(&roller);

    drawPlanet(0);

    if (!drawNextChar() && !--wait) {
        switch (infoPhase++) {

        case INFO_NAME:
            initAsciiStringDraw(FONT_LARGE, 0xA, 8, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planets[planet].name, 0,
                                _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT - FONTLARGE_FONT_HEIGHT);
            wait = 10;
            break;

        case INFO_PHYSICS:
            initAsciiStringDraw(FONT_COMPACT, 0x16, 3, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,    //
                                planets[planet].physics, 0, _SCANLINES - 2 - 2 * FONTCOMPACT_FONT_HEIGHT);
            wait = 50;
            // pic = ((rangeRandom(15) + 1) << 4) | 8;
            pif = -1;
            break;

        case INFO_INFO: {
            int lastpif = pif;
            pif = pif + 1;    // rangeRandom(MAX_PLANET);

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

            initAsciiStringDraw(FONT_COMPACT, 0xD8, 2, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, planetInfo[pif].review, 0,
                                70 - ((lines * FONTCOMPACT_FONT_HEIGHT) >> 1));
            wait = 50;
            break;
        }

        case INFO_RATING1:

            lines = 68 + (((lines + 1) * FONTCOMPACT_FONT_HEIGHT) >> 1);

            initAsciiStringDraw(FONT_COMPACT, 0x18, 0, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0, review[0], 0, lines);
            wait = 1;
            break;

        case INFO_RATING2:
            initAsciiStringDraw(FONT_COMPACT, 0x18, 20, _BUF_GLOBE_GRP, _BUF_GLOBE_COLUP0,
                                review[planetInfo[pif].stars], 0, lines);
            wait = 400;
            break;


        case INFO_CLEAR:
            myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_GRP), 0, 6 * _BUFFER_SIZE / 4);
            wait = 1;
            break;

        case INFO_NEXTPLANET1:

            if ((scalex < MIDPOINT && planetDir < 0)) {
                // while (true)
                //     ;
                wait = 30;
            } else {
                infoPhase = INFO_NEXTPLANET1;
                wait++;
            }
            break;

        case INFO_NEXTPLANET:

            if (scalex > MIDPOINT && planetDir < 0) {    //(scalex >> 8) == (SCALE_FAR >> 8) && planetDir > 0) {
                planet = nextPlanet();
                myMemsetInt((unsigned int *)(RAM + _BUF_GLOBE_PF), 0, 6 * _BUFFER_SIZE / 4);
                wait = 50;
                infoPhase = INFO_NAME;
            } else {
                //                lastpd = planetDir < 0 ? -1 : 1;
                infoPhase = INFO_NEXTPLANET;
                wait++;
            }
            break;
        }
    }
}

// EOF