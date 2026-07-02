#include "planetInfo.h"


const struct planetReviews planetInfo[] = {

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

#if 1

{"=\"The|planet hums a|note not|quite any|note I know,|and my|species has a|word for that|note, and the|word means|_leave_.#",1}, // 10

#else

// unsorte

// 0
{"=\"The|currency is|based on|something I|will not|name, but I|had more of|it after a|long trip.|That felt|unfair.#",1}, // 5
{"=\"Checked|in to what I|thought was|my room. It|was a lift,|and I|unpacked|before the|lift|explained|itself.#",2}, // 6 — "in to" → "into"
{"=\"Each|room runs at a|different|speed. I was|somehow|early and|late for the|same|meal.#",1}, // 8
{"=\"A kind|word here|causes mild|pain. I was|polite from|day one. Did|not connect|these facts|until day|two.#",2}, // 8
{"=\"The|tourist board|slogan is... WE|ARE HERE.|That is the|full extent|of the|pitch.#",2}, // 7
{"=\"A human|laughed at|me, and I|read this as a|threat|display. I|responded in|kind. The|crowd that|formed did|not help.#",2}, // 6 — "A human" inconsistent with "local"
{"=\"Assumed|the|atmosphere|was|decorative.|Removed my|suit. Second|mistake,|after|booking.#",1}, // 9
{"=\"Joined|what I|thought was a|guided walk.|It was a|funeral. By|the end I|was a|pallbearer.#",2}, // 8
{"=\"I asked|a local what|they did for|fun. They|stared long|enough that I|felt I had|said|something in|very poor|taste.#",2}, // 7
{"=\"Everyone|here has|three|shadows, one|of them|cold. Mine|kept touching|theirs, and|this was|rude.#",2}, // 9
{"=\"A human|told me to|have a nice|day, and I|said I would|try. It looked|at me as|though trying|was not the|expected|response.#",3}, // 5 — "A human"/"It" pronoun mismatch
{"=\"Beautiful|planet, would|return. Can't|return for|legal|reasons.#",4}, // 7 — "Can't" → "cannot"
{"=\"Made an|amazing|discovery on|this planet.|The box|provided is|too small to|detail it|here.#",4}, // 7
{"=\"Paid for|the premium|tour. It was|the same as|the standard|tour, but with|an angry|guide.#",1}, // 8
{"=\"Stepped|off the ship|and joined|the local|ecosystem.|Took effort|to reverse.|Left me|feeling|implicated in|something.#",2}, // 7
{"=\"The exit|sign pointed|at the ocean,|and the|ocean had no|exit. Very|poor|planning.#",1}, // 8
{"=\"The|locals can|see sound|and asked|me to keep it|down. I said|I was not|making any.|They showed|me.#",2}, // 9
{"=\"The|gravity|reverses|briefly at|noon. This is|in a footnote.|The|footnote is at|the bottom|of the|page.#",2}, // 8

// 8
{"=\"The|handshake|has a part|where you|pretend not|to know each|other. I|skipped that|part, and|things went|wrong.#",2}, // 8
{"=\"The|nightlife|starts at|dusk, ends|shortly after|dusk, and|everyone|goes home|and|sighs.#",2}, // 8
{"=\"The|welcome|pack|included a|breathing|guide, which|I assumed|was a joke. It|was not a|joke.#",1}, // 8
{"=\"There is|no word for|stranger, and|I was given a|family on|arrival. They|had views on|how I spent|my days.#",2}, // 8
{"=\"They|keep a room|clean and|ready for|guests, sit in|a worse room|on all other|days, and|call this|having a|lounge.#",3}, // 6
{"=\"They|pay for|water in a|bottle when|it falls free|from the sky,|complain|when it falls.|Both are|considered|normal.#",3}, // 6 — "falls" repeated
{"=\"They|removed the|stimulant|from their|stimulant|drink, still|drink it, and|call this a|choice they|made|freely.#",3}, // 6 — "stimulant" repeated
{"=\"Time|here is|measured in|moods. My|shuttle was|due at after|the sadness,|and I waited|at the wrong|feeling.#",1}, // 6 — "due at after" → "due after"
{"=\"Travel|insurance|listed this|planet by|name in the|exclusions,|and I booked|anyway, my|mistake.#",1}, // 9
{"=\"Tried to|explain to a|human that|their planet is|unusual, and|she said %we|like to think|we are|special&. I|said|%know&.#",3}, // 6
{"=\"Was|handed a|leaflet of|things to do|here. The|leaflet had|one page,|and one of|the things|listed was|leaving.#",3}, // 9
{"=\"Was told|the piles|were|'historical'|and I said|'historical|what?'.|Nobody|finished the|sentence.#",3}, // 6 — single quotes → %&
{"=\"The|year is three|days long. I|booked a|week. Five|new-year|celebrations.|A gift was|expected|each|time.#",1}, // 8
{"=\"The slow|part of the|day is called|rush hour, a|naming|decision that|has clearly|never been|reviewed.#",3}, // 5
{"=\"The|local|cemetery is|peaceful|and|well-kept.|The dates on|the stones|are all in the|future.#",1}, // 9
{"=\"A local|told me to|take care|when I left.|I asked of|what. She|said just in|general. I|said that is a|lot to take|care of.#",3}, // 5
{"=\"Did not|realise|breathing was|compulsory|here. Had to|be reminded|twice.|Signage is|inadequate.#",2}, // 8
{"=\"Held my|umbrella the|wrong way|for two days|before|learning the|rain falls|down|here.#",2}, // 8
{"=\"I asked|a local how|long they had|lived here. It|said longer|than it meant|to, and|stared at the|horizon.#",2}, // 9
{"=\"I asked|a local what|time it was,|and he told|me in a unit I|do not have|the organs to|perceive.|Useless.#",2}, // 8
{"=\"I asked|a ranger|which trail|was best. He|said %It|depends&. I|said|%scenery&.|He said %then|not this|planet&.#",3}, // 9
{"=\"I asked|if the air was|safe. %For|some|definitions|of safe&,|they said. My|species|reads this as|yes. It is|_not_.#",1}, // 8
{"=\"I asked|if the haze|ever clears.|%Clears of|what?&, they|asked. I|realised they|had never|seen their|planet.#",2}, // 9
{"=\"I asked|where all the|valuable|minerals had|gone, and|was told|%away&.#",2}, // 8
{"=\"Mistook|the warning|siren for a|welcome|song, and|sang for|twenty|minutes|before|someone|intervened.#",2}, // 7
{"=\"Moisture|stuck to my|outer layer. I|took this as|territorial|and|panicked.|Better|labelling|would|help.#",2}, // 7
{"=\"Photos|came out|entirely|grey, and the|locals said|that was the|best the|planet had|looked.#",2}, // 9
{"=\"My ship's|navigation|refused to|save this|location to|favourites. I|now|understand it|was trying to|help me.#",1}, // 9
{"=\"The air|here is|edible. I had|too much on|day one and|had to lie|down. For|reasons I|could not|convey to|anyone.#",1}, // 8
{"=\"The|beach looked|perfect in|photos.|Photos were|the right way|to|experience|it.#",1}, // 8
{"=\"The|guide|pointed at|something,|said %there it|is&, and|walked on|before I|could see|what it|was.#",1}, // 8
{"=\"The|locals change|colour to|speak, and I|have no such|ability. I|spent the trip|mute in a way|I have not|been|before.#",1}, // 9
{"=\"Locals|exhale what|I breathe. I|exhale what|they breathe.|For one|afternoon|we were|very|close.#",4}, // 10
{"=\"The|locals shed|their skin|each morning|and leave it|in the hall. I|greeted my|neighbour's|%shed& for|two|days.#",2}, // 8
{"=\"The|ocean here is|technically|swimmable,|in the sense|that you can|enter it and|exit it. The|question is|what you exit|as.#",2}, // 9
{"=\"The|wildlife|ignored me|until it did|not. The|transition was|not|announced.#",1}, // 9
{"=\"The|local drink|was an|%acquired|taste&. I|acquired it. I|would like to|return it.#",2}, // 8
{"=\"The|locals have a|greeting of|twelve|gestures. I|learned|eleven. The|twelfth is|apparently|the important|one.#",2}, // 8
{"=\"I left a|review on|arrival as is|my custom.|The locals|found it. It|affected|the rest of|the stay.#",2}, // 9
{"=\"Lovely|from orbit.|The tourism|board|recommends|orbit. This is|on page|nine.#",3}, // 9
{"=\"There|was a|festival. I|was not told|what it was|for. I was|told to stand|here.~ I stood|here. Nobody|explained.#",2}, // 8
{"=\"Said|goodbye on|leaving. The|local said see|you soon. I|have not|returned.~ I|think about|that.#",2}, // 9
{"=\"The|local told me|to enjoy my|stay. I asked|how. He said|however I|liked. I did.~|He later told|me that was|not what he|meant.#",2}, // 8
{"=\"I make a|sound when I|breathe. This|species|associates|that sound|with grief.|The staff|were very|kind.#",2}, // 9
{"=\"Pointing|is considered|rude here.|The|guidebook|has pictures|you point at|to order|food. This is|not|addressed.#",1}, // 8
{"=\"The|local|children ask|how many|times you|have died. I|said none.|They|conferred|and moved|away.#",1}, // 9
{"=\"On day|five my|translator|became|philosophical.|It rendered|everything as|%but why?& I|have not|found good|answers.#",2}, // 8
{"=\"There|are no|shadows|here. The|light comes|from|everywhere.|After three|days I missed|somewhere|to hide.#",1}, // 9
{"=\"The|locals do not|finish|sentences.|They stop|where the|important|part begins.|My translator|was not built|for this.#",2}, // 9
{"=\"There is|no word for|yesterday|here. All|complaints|must be about|the future. I|filed|seven.#",2}, // 9
{"=\"I was|warned the|locals read|thoughts. I|was very|careful. I|would prefer|not to say|about|what.#",1}, // 9
{"=\"The|guidebook|says locals|value|directness.|One told me|the|guidebook|was not|written|here.#",1}, // 9
{"=\"The|locals sleep|in shifts. The|shifts are|unlabelled. I|got into|someone|else's bed on|day two.|They were|polite.#",2}, // 9
{"=\"The|guidebook|says locals|are friendly|to visitors. A|second|guidebook|clarifies|which|visitors. I had|the first.#",1}, // 9
{"=\"I glow|slightly in|cold. It was|cold here. By|day three|several|people had|begun|following me|at night.#",2}, // 8
{"=\"The|planet|rotates the|wrong way. I|cannot|explain why|this bothers|me. I have|felt slightly|wrong all|week.#",1}, // 8
{"=\"The|locals use|descriptions,|not names. I|arrived tired|and|confused.|This is now|my|name.#",2}, // 9
{"=\"A local|ritual. Six|hours. Nothing|happened.|The locals|were very|pleased. I|asked. They|said|%everything&.#",2}, // 8
{"=\"The|smell here is|called|%distinctive&|in the|brochure.|Calling a fire|warm is also|not|wrong.#",1}, // 8
{"=\"The|locals have|perfect|recall. I had|forgotten|what I said|on day one.|They had|not. Day|three was|difficult.#",1}, // 9
{"=\"The|locals point|with their|attention, not|their fingers.|I spent three|days not|knowing|where|anything|was.#",2}, // 8
{"=\"By day|four a plant|near my|window had|moved. I|mentioned|this to the|staff. They|said yes.#",2}, // 9
{"=\"The|planet hums a|note not|quite any|note I know,|and my|species has a|word for that|note, and the|word means|_leave_.#",1}, // 10
{"=\"The|planet smells|of something|I cannot|name but|have smelled|once|before, in a|situation I do|not wish to|revisit.#",2}, // 8
{"=\"The|scenic flyby|is good if you|like gas,|which I did|not know I|did not like|until I was|inside it.#",2}, // 6
{"=\"Hello|and an|explicit|proposal|differ here|only by tone.|I have poor|tone in this|language. I|was very|popular.#",1}, // 8
{"=\"The|tourism|office gave|me a list of|things to do.|One of them|was the|tourism|office. I|was already|there.#",3}, // 8
{"=\"The two|moons orbit at|a distance|that suggests|they are|trying to stay|out of|things.#",2}, // 9
{"=\"I was|measured on|arrival. My|dimensions|were noted.|Several|locals came|to look.#",2}, // 7
{"=\"The|attendant|said the|wildlife was|friendly, and|when I asked|how|friendly, she|paused in a|way that|answered.#",1}, // 8

// 9
{"=\"I asked|a local if the|_smell_ was|natural or|man-made.|She said|%yes&. I have|thought about|this every|day|since.#",3}, // 9
{"=\"I asked|what grows|here. I was|told|%resentment&.|Booked the|cultural tour.|It was about|resentment.#",2}, // 8
{"=\"Landed,|explored,|ate|something,|left. Three|of those four|I would not|do again.#",2}, // 8
{"=\"Misread|the entry|form, and|declared|myself as|luggage. I|spent day|one in a|storage bay|nicer than my|hotel.#",3}, // 9
{"=\"My suit's|air monitor|skipped|numbers and|went straight|to a small|picture of|something|dying.#",1}, // 9
{"=\"The|cold here has|texture. I|was not|warned about|this. I do not|have the|right organs|for it.#",1}, // 8
{"=\"The gas|here is|sentient in|patches. One|of the|patches was|in my cabin|and had|strong views|about the|lighting.#",1}, // 9
{"=\"The|heat here sits|in the range|my species|uses for|food storage.|I kept|reflexively|trying to put|things in the|locals.#",1}, // 10
{"=\"The|local dish|arrived alive|and looking at|me. I did not|know the|correct|response.#",1}, // 8
{"=\"The|ocean laps|the shore in a|way that|sounds like it|is sighing ~at|you,|~personally.#",1}, // 7
{"=\"The|oceans are|warm and the|things in them|are also warm|and|%friendly&, in|a way that|means you|cannot|swim.#",3}, // 8
{"=\"The|locals keep a|small animal|in their|dwelling.|The animal|has no duties.|The animal|knows|this.#",2}, // 9
{"=\"A local|said %have a|good one& as|I left. I did|not know|what one. I|have been|thinking about|the one.#",2}, // 8
{"=\"Asked if|there was|anything I|should not do.|He listed|seven things.|I had done|four of them|before he|finished.#",2}, // 8
{"=\"I smiled|at a local. It|stopped.~ I|stopped|smiling. It|resumed.~ I|have thought|about this|every day|since.#",2}, // 9
{"=\"I do not|sleep the|way this|species does.|I stood in a|corner each|night. The|cleaner|stopped|coming in on|day two.#",2}, // 8
{"=\"The|entry form|asked what I|was willing to|lose. I did|not read this|carefully. I|have since|checked.|Something is|missing.#",1}, // 9
{"=\"The|locals|reproduce by|arguing. I|made several|points at|dinner. I am|told there|are now nine|more locals. I|apologise.#",2}, // 10
{"=\"This|planet smells|like|something I|recognise but|cannot name.|On day four I|placed it. I|cut the trip|short.#",1}, // 8
{"=\"The sky|has no colour|name. My|translator|called it %the|colour|before|something|stops being|true&.#",2}, // 9
{"=\"This|planet has a|return policy|on|experiences.|I tried to use|it. There is a|lot of|paperwork.#",2}, // 8
{"=\"I asked|what the|smell was. A|local said it|was the|planet|thinking. I|asked what|about. She|said %us&.#",2}, // 9
{"=\"The|locals|measure|distance in|feelings.|The hotel|was %not far&|and %worth|it&. It took|me three|days.#",2}, // 8
{"=\"There is|a saying here:|%the second|time is always|worse&. My|translator|offered this|unprompted.|Several|times.#",2}, // 8
{"=\"The|scenery is|%once seen,|not|forgotten&,|says the|brochure.|This is|accurate. I|wish it were|not.#",2}, // 8
{"=\"The pool|had a sign I|could not|read. The|staff said it|was advice. I|asked what|advice. They|said not|swimming.#",1}, // 9
{"=\"The|reviews all|said %you will|see&. They|were all five|stars. I have|seen. I|understand.#",2}, // 8
{"=\"The|wake-up call|came at|seven. A|voice said %it|is seven&.|Then %we|are sorry&.|Then|nothing.#",1}, // 9
{"=\"The|guest book|entries were|all addressed|to the next|visitor. They|all said the|same thing. I|added|mine.#",2}, // 9
{"=\"The|guidebook|said the|locals value|intimacy with|visitors. I|thought this|meant being|open. I was|half|right.#",2}, // 8
{"=\"The|planet hums|at a pitch my|species|reads as|grief. I|cried for|five days.#",1}, // 9
{"=\"The|upgrade was|to a larger|pile, and I|said thank|you because|I did not|know what|else to|say.#",2}, // 8
{"=\"The|volcano tour|was|cancelled|due to the|volcano, a|sentence I|could not|have|predicted|needing.#",2}, // 7
{"=\"I asked|about the|length of the|tour. The|guide said it|depended on|interest. She|said I|seemed very|interested.#",2}, // 7

// 10
{"=\"My|translator|rendered|the local|language as|'medium|length sighs'|all visit. I'm|not entirely|sure that was|wrong.#",3}, // 7 — 'medium...' → %medium length sighs&; "I'm" → "I am"; "all visit" → "all visit long"
{"=\"Stepped|off the ship|and became|part of the|local food|chain within|minutes.#",1}, // 9
{"=\"The|compost bin|in my room|was sentient|and began|making|requests on|day two. I|did not know|how to|refuse.#",1}, // 8
{"=\"The|days are|eighteen|minutes. I|booked three|nights and|was there|for under an|hour.#",2}, // 8
{"=\"The|festival was|described as|%lively&. It|involved|twelve|people|standing near|a thing.#",2}, // 9
{"=\"They|exchange|information|by vibrating|their|face-holes.|Impressive,|but needless|given that|telepathy|exists.#",2}, // 9
{"=\"The kids|here have a|look in their|eyes that I|have thought|about every|day since I|left.#",1}, // 8
{"=\"The|locals are|made of|sound, and I|kept walking|through them.|They kept|saying sorry,|which made|the problem|louder.#",2}, // 10
{"=\"The|locals have|no word for|%no& and|express it by|doing the|thing anyway.|Sadly, I|missed this|for three|days.#",2}, // 9
{"=\"The|locals leave|gaps in|speech for|you to fill. By|day one, I|had agreed|to several|things I had|not planned|on.#",2}, // 9
{"=\"The|locals said|the smell was|part of the|culture. I|said which|part, and|they said %all|of it&.#",1}, // 8
{"=\"My room|had a card|explaining|local|etiquette. I|read it after|day three.|This explains|days one|through|three.#",2}, // 9
{"=\"Paid|extra for the|room with a|view. The|view was of|the room with|the better|view.#",2}, // 9
{"=\"The|planet has|one season.|The locals|have twelve|words for it|and argue|about which|word|applies.#",2}, // 8
{"=\"The|signs use a|language I do|not read.|The locals|also do not|read it.~ This|was not in the|brochure.#",2}, // 8
{"=\"There|was a gift|shop at the|exit. The|gifts were|images of|the|entrance. I|bought one.~ I|do not know|why.#",3}, // 8
{"=\"My|translator|gave up on|day four and|began|summarising.|%They are|speaking&, it|would say. %It|is about|you.&#",2}, // 9
{"=\"The|currency|here is|apologies. I|had several|to spend. I|ran out on|day|three.#",2}, // 8
{"=\"The|mountain is|said to watch|visitors. I|dismissed|this. On day|four it was|closer. I did|not check on|day|five.#",2}, // 9
{"=\"The|local|greeting is|something I|will not|describe. I|thought it was|a handshake.|We are|married|now.#",1}, // 9
{"=\"The|gesture for|thank you|here is very|intimate. I|thanked|everyone I|met. I have|been invited|back.#",3}, // 8
{"=\"The|planet leans,|and every|local I told|looked|genuinely|unsure if that|was true.#",2}, // 8
{"=\"The|shadow|followed me|the whole|time. When I|complained,|I was told it|was mine.|This only|raised more|questions.#",2}, // 8
{"=\"I asked|about the|noise. She|said|%relaxing&. I|asked if|always this|loud. Only|when it goes|well.#",2}, // 8
{"=\"The|currency is|smell-based.|I arrived|with nothing|of value, and|left owing a|scent debt I|am still|repaying.#",1}, // 9
#endif

    // clang-format on
};

// EOF