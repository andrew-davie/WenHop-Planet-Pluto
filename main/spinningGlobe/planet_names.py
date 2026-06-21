"""
Planet name generator — exotic/alien edition.
Usage:
    python planet_names.py                  # 20 random names across all styles
    python planet_names.py --style uvular   # specific style
    python planet_names.py --count 50       # more names
    python planet_names.py --list-styles    # show available styles
    python planet_names.py --all-styles     # demo every style
"""

import random
import argparse

STYLES = {}

def style(name):
    def decorator(fn):
        STYLES[name] = fn
        return fn
    return decorator


# ---------------------------------------------------------------------------
# EXOTIC ALIEN STYLES
# ---------------------------------------------------------------------------

@style("uvular")
def uvular():
    """Uvular + pharyngeal consonants. The back of the throat made alien.
    Inspired by Arabic/Georgian but pushed much further into the uncanny."""
    # Represents sounds like uvular trills, pharyngeal fricatives, epiglottals
    onset = random.choice([
        "Qħ", "Ʀ", "Ɣ", "Xʕ", "Qʕ", "ʔQ", "ħX", "ʕħ",
        "Ɣʕ", "Qr", "Xħ", "Ʀħ", "ʔɣ", "ħq", "Xʔ", "Qɣ",
    ])
    nucleus = random.choice([
        "aʕa", "eħe", "oʔo", "uɣu", "aːe", "eːa", "oːu",
        "ɑːo", "æːi", "øːu", "œːa", "ɯːe", "ɨːo",
    ])
    coda = random.choice([
        "q", "ħ", "ʕ", "Ɣ", "qħ", "ʕq", "ħʔ", "ɣq",
        "qr", "ħr", "ʕr", "ɣħ", "rq", "rħ",
    ])
    return onset + nucleus + coda


@style("vowelless")
def vowelless():
    """No vowels at all. Consonant clusters that feel unpronounceable.
    Like Tlhingan or Welsh run through a nightmare."""
    clusters = [
        "Krthl", "Vxzkr", "Bnthkr", "Ghjxl", "Zvrks", "Thxkl",
        "Krzvn", "Mxlth", "Phtxr", "Wrzkt", "Shtxl", "Grzkh",
        "Tsrvn", "Zlkrth", "Vrzkm", "Nxlth", "Jrzkv", "Qrthx",
        "Bxlkr", "Fztrk", "Gzkrl", "Hxvzt", "Dzthr", "Czrvk",
        "Skrxl", "Pltzk", "Mzthr", "Rktzl", "Snxkr", "Vrthz",
    ]
    suffixes = [
        "", "kh", "zt", "rv", "xl", "nk", "vr", "ths", "xth",
        "krn", "vlz", "thx", "nxl", "zrk",
    ]
    return random.choice(clusters) + random.choice(suffixes)


@style("tonal_diacritic")
def tonal_diacritic():
    """Tonal language feel — diacritics mark pitch contours.
    Like Mandarin or Vietnamese pushed into sci-fi."""
    bases = [
        "Xao", "Qei", "Vhu", "Zha", "Tho", "Khi", "Rha",
        "Vla", "Xue", "Qui", "Zho", "The", "Kha", "Rhu",
        "Vlei", "Xuao", "Qhui", "Zhai", "Rhao", "Khei",
    ]
    # tone diacritics: high, rising, dipping, falling, checked
    tones = ["̄", "́", "̌", "̀", "̂", "̃", "̈", "̊"]
    # Apply 1-3 tones scattered through the name
    base = random.choice(bases)
    result = []
    for ch in base:
        result.append(ch)
        if ch.lower() in "aeiouāēīōū" and random.random() < 0.6:
            result.append(random.choice(tones))
    return "".join(result)


@style("ejective")
def ejective():
    """Ejective consonants (explosive stops). Caucasian/Mayan feel.
    Marked with apostrophes after the stop — each one is a tiny explosion."""
    ejectives = ["K'", "T'", "P'", "Q'", "Ts'", "Ch'", "Tx'", "Kx'", "Qx'"]
    vowels = ["a", "e", "i", "o", "u", "aa", "ii", "oo", "uu", "ae", "ei"]
    plain_c = ["r", "l", "n", "m", "v", "z", "s", "x", "h", "w", "y"]
    plain_stops = ["k", "t", "p", "q", "g", "d", "b"]

    parts = []
    # Start with an ejective
    parts.append(random.choice(ejectives))
    parts.append(random.choice(vowels))

    # Middle section
    n_mid = random.randint(1, 2)
    for _ in range(n_mid):
        parts.append(random.choice(plain_c + plain_stops))
        parts.append(random.choice(vowels))

    # End with another ejective or plain stop
    if random.random() < 0.5:
        parts.append(random.choice(ejectives))
    else:
        parts.append(random.choice(plain_stops))

    return "".join(parts).capitalize()


@style("recursive_reduplication")
def recursive_reduplication():
    """Partial reduplication — a morphological trick that screams alien grammar.
    The name seems to echo itself in a disturbing, non-human way."""
    roots = [
        "keth", "vral", "xono", "thux", "zimi", "qara", "vrex",
        "khol", "tsuu", "zhim", "xaer", "quon", "theel", "vraz",
        "kaan", "xton", "zhaar", "queel", "thom", "vraax",
    ]
    root = random.choice(roots)

    mode = random.randint(0, 3)
    if mode == 0:
        # Full reduplication with mutation
        mutation = random.choice(["v", "kh", "z", "x", "th", "q"])
        return (root + mutation + root).capitalize()
    elif mode == 1:
        # Prefix copy of first consonant cluster
        onset = root[:random.randint(1,2)]
        return (onset + "i" + root).capitalize()
    elif mode == 2:
        # Suffix echo (last syllable repeated with vowel change)
        echo = root[-2:].replace("a","e").replace("o","u").replace("i","a")
        return (root + echo).capitalize()
    else:
        # Triplication with shrinkage
        short = root[:2]
        return (root + short + root[-2:]).capitalize()


@style("retroflex")
def retroflex():
    """Heavy retroflex consonants + unusual vowel inventory.
    Sounds like Sanskrit or Hindi but from a different dimension."""
    # Retroflex markers: ṭ ḍ ṇ ṣ ṛ ḷ  (curled tongue against palate)
    onsets = [
        "Ṭh", "Ḍr", "Ṣkh", "Ṭr", "Ḍh", "Ṇv", "Ṣr", "Ṭv",
        "Ḍkh", "Ṇth", "Ṣvr", "Ṭhv", "Ḍrv", "Ṇkh", "Ṣth",
    ]
    nuclei = [
        "ṛaa", "ḷii", "ṛuu", "ḷee", "ṛoo", "ḷaa",
        "ṛṛa", "ḷḷi", "ṛṛu", "ḷḷe", "aṛa", "iḷi",
        "uṛu", "eḷe", "oṛo",
    ]
    codas = [
        "ṭ", "ḍ", "ṇ", "ṣ", "ṭh", "ḍh", "ṇṭ", "ṣṭ",
        "ṛṭ", "ḷḍ", "ṭṛ", "ḍḷ", "ṇṣ",
    ]
    return random.choice(onsets) + random.choice(nuclei) + random.choice(codas)


@style("long_vowel_horror")
def long_vowel_horror():
    """Grotesquely long vowel sequences interrupted by harsh stops.
    Sounds like something that takes a long time to say and shouldn't be said."""
    stops = ["kx", "tz", "qh", "vx", "bx", "zk", "gx", "dx", "xk", "qt"]
    # Long stretched vowels
    long_v = [
        "aaaa", "eeee", "oooo", "uuuu", "iiii",
        "aaee", "oouu", "iiaa", "uuoo", "eeii",
        "aaooeee", "uuiioo", "eeaauuu",
    ]
    short_v = ["a", "e", "i", "o", "u"]

    parts = []
    # Pattern: STOP + longggg vowel + STOP + short vowel + STOP
    parts.append(random.choice(stops).capitalize())
    parts.append(random.choice(long_v))
    parts.append(random.choice(stops))
    if random.random() < 0.5:
        parts.append(random.choice(short_v))
        parts.append(random.choice(stops))

    return "".join(parts)


@style("xeno_compound")
def xeno_compound():
    """Two alien morphemes jammed together with a ligature character.
    Feels like a proper noun from a language with a completely alien grammar."""
    ligatures = ["·", "‧", "•", "∘", "◦", "⊙", "⊕", "⊗", "×", "∷"]
    morphemes = [
        "Xhaal", "Vreeth", "Qonth", "Zzrak", "Thoox", "Kheel",
        "Vraan", "Xonn", "Qhith", "Zzael", "Thaak", "Khuun",
        "Vreex", "Xhoon", "Qheel", "Zzaan", "Thoov", "Khaal",
        "Vreel", "Xhaan", "Qonx", "Zztho", "Thaan", "Khuux",
        "Nxeel", "Rhaal", "Ssoon", "Llhaan", "Mmveel", "Nnxaal",
    ]
    m1 = random.choice(morphemes)
    m2 = random.choice([m for m in morphemes if m != m1])
    sep = random.choice(ligatures)
    return f"{m1}{sep}{m2}"


@style("click_extended")
def click_extended():
    """Extended click consonant system — dental, alveolar, palatal, lateral, retroflex.
    Each click type has a radically different mouth position."""
    # IPA-ish click notation
    click_types = [
        "ǀ",   # dental click
        "ǁ",   # alveolar lateral click
        "ǂ",   # palatal click
        "ǃ",   # alveolar click (retroflex)
        "ʘ",   # bilabial click
        "ǀǀ",  # double dental
        "ǃǀ",  # compound
        "ǂǁ",  # compound
    ]
    # Nasal/voiced prefixes
    prefixes = ["N", "G", "Ng", "M", "D", "Gh", "Kh", "Q", "X", "Z", "Ngh"]
    # Vowel nuclei — often long or nasalised
    nuclei = [
        "ã", "õ", "ĩ", "ũ", "ẽ",
        "aː", "oː", "iː", "uː", "eː",
        "ãː", "õː", "ĩː", "ũː",
        "ao", "ei", "ou", "ai",
    ]
    suffix = random.choice(["m", "n", "ng", "x", "q", "b", "d", "g", "s", "r", ""])

    pre = random.choice(prefixes)
    click = random.choice(click_types)
    nuc = random.choice(nuclei)
    return f"{pre}{click}{nuc}{suffix}"


@style("bio_horror")
def bio_horror():
    """Biological + horror. Wet, organic sounds mixed with wrong consonants.
    This planet should not exist and its name should be hard to forget."""
    wet = ["gl", "sl", "ml", "nr", "wr", "lv", "lm", "mn", "rn", "vl"]
    pulses = ["ugh", "ulch", "elgh", "orch", "uelk", "irch", "aelg", "oolm"]
    velar = ["ng", "nk", "ngk", "ngh", "kng", "gng"]
    long = ["oooo", "aaaa", "uuuu", "iiii", "eeee"]
    stops = ["k", "g", "x", "q", "kh", "gh"]

    pattern = random.randint(0, 2)
    if pattern == 0:
        return (random.choice(wet) + random.choice(pulses) + random.choice(velar)).capitalize()
    elif pattern == 1:
        return (random.choice(stops) + random.choice(long) + random.choice(wet) + random.choice(stops)).capitalize()
    else:
        return (random.choice(velar) + random.choice(pulses) + random.choice(wet) + random.choice(pulses)).capitalize()


@style("silence_gaps")
def silence_gaps():
    """Names with literal silence markers — gaps that imply sounds outside human range.
    The underscore or dash represents a pause/infrasound no human throat can make."""
    gaps = ["_", "__", "—", "·", " "]
    pre_elements = [
        "Vx", "Qh", "Zr", "Th", "Kx", "Nx", "Xr", "Gh",
        "Vr", "Qx", "Zk", "Thx", "Kq", "Nz", "Xk", "Gz",
    ]
    post_elements = [
        "aal", "oox", "eel", "uun", "iim", "aath", "ookh", "eelx",
        "uunz", "iikx", "aaxh", "oozr", "eekh", "uunx", "iizr",
    ]
    n = random.randint(2, 3)
    parts = [random.choice(pre_elements) + random.choice(post_elements) for _ in range(n)]
    gap = random.choice(gaps)
    return gap.join(parts)


@style("harmonic")
def harmonic():
    """Vowel harmony — all vowels in the word belong to one 'class'.
    Feels like Turkish or Finnish but alien. The vowels resonate together."""
    # Front vowels: e, i, ö, ü  (mapped as e, i, oe, ue)
    # Back vowels: a, o, u, ı   (mapped as a, o, u, ih)
    front_v = ["e", "i", "oe", "ue", "ei", "ie", "oei", "uei"]
    back_v  = ["a", "o", "u", "ih", "ao", "ua", "oua", "iha"]
    # Consonants that don't affect harmony
    consonants = [
        "kh", "vr", "zth", "xn", "qr", "thv", "nzk", "vxr",
        "zk", "xth", "qn", "thx", "nv", "vz", "zkh", "xvr",
        "k", "v", "z", "x", "q", "th", "n", "r", "l", "s",
    ]

    harmony_class = random.choice(["front", "back"])
    vowels = front_v if harmony_class == "front" else back_v

    n_syllables = random.randint(2, 4)
    parts = []
    for i in range(n_syllables):
        if random.random() < 0.4 and i > 0:
            parts.append(random.choice(consonants))
        parts.append(random.choice(consonants))
        parts.append(random.choice(vowels))

    return "".join(parts).capitalize()


@style("designation")
def designation():
    """Cold scientific catalogue ID — real astronomers do this."""
    prefix = random.choice([
        "HD", "KIC", "TOI", "GJ", "LHS", "PSR", "WASP",
        "Kepler", "TRAPPIST", "K2", "HIP", "Wolf", "EPIC",
        "TIC", "CoRoT", "OGLE", "HAT-P", "XO",
    ])
    number = random.randint(1, 99999)
    if random.random() < 0.5:
        suffix = "-" + random.choice(list("bcdefghij"))
        return f"{prefix}-{number}{suffix}"
    return f"{prefix}-{number}"


# ---------------------------------------------------------------------------
# Generator + CLI
# ---------------------------------------------------------------------------

def generate(style_name=None, count=1):
    if style_name:
        if style_name not in STYLES:
            raise ValueError(
                f"Unknown style '{style_name}'. "
                f"Available: {', '.join(STYLES)}"
            )
        return [STYLES[style_name]() for _ in range(count)]
    style_fns = list(STYLES.values())
    return [random.choice(style_fns)() for _ in range(count)]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Exotic planet name generator")
    parser.add_argument("--style", help="Name generation style")
    parser.add_argument("--count", type=int, default=20, help="Number of names (default: 20)")
    parser.add_argument("--list-styles", action="store_true")
    parser.add_argument("--all-styles", action="store_true", help="Show 4 examples per style")
    args = parser.parse_args()

    if args.list_styles:
        print("Available styles:")
        for name, fn in STYLES.items():
            doc = fn.__doc__.strip().split("\n")[0]
            print(f"  {name:<26} {doc}")

    elif args.all_styles:
        for name, fn in STYLES.items():
            doc = fn.__doc__.strip().split("\n")[0]
            examples = "  |  ".join(generate(name, 4))
            print(f"\033[1m{name}\033[0m — {doc}")
            print(f"  {examples}\n")

    else:
        for name in generate(args.style, args.count):
            print(name)