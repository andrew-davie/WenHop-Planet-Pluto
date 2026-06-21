"""
Planet name generator with multiple styles.
Usage:
    python planet_names.py                  # 20 random names across all styles
    python planet_names.py --style alien    # specific style
    python planet_names.py --count 50       # more names
    python planet_names.py --list-styles    # show available styles
"""

import random
import argparse

# ---------------------------------------------------------------------------
# Style definitions
# ---------------------------------------------------------------------------

STYLES = {}

def style(name):
    def decorator(fn):
        STYLES[name] = fn
        return fn
    return decorator


@style("classical")
def classical():
    """Latin/Greek-inspired. Familiar, grand-sounding."""
    prefixes = ["Aqu", "Arc", "Bor", "Cal", "Cas", "Cep", "Cor", "Del",
                "Dra", "Erid", "For", "Gem", "Her", "Hyp", "Lep", "Lib",
                "Lyr", "Men", "Mir", "Nep", "Ori", "Per", "Pho", "Reg",
                "Sep", "Ser", "Sol", "Tau", "Tel", "Ven", "Ver", "Vir"]
    middles = ["", "uli", "ari", "ori", "ani", "eli", "ini", "eni",
               "oph", "ula", "eus", "ium"]
    suffixes = ["us", "a", "um", "on", "is", "ix", "ius", "ia",
                "ara", "eon", "ion", "aris", "oris", "anus"]
    return random.choice(prefixes) + random.choice(middles) + random.choice(suffixes)


@style("designation")
def designation():
    """Catalogue-style. Cold, scientific."""
    prefix = random.choice(["HD", "KIC", "TOI", "GJ", "LHS", "PSR",
                             "TrES", "WASP", "Kepler", "TRAPPIST", "K2",
                             "HIP", "NGC", "Wolf"])
    number = random.randint(1, 99999)
    if random.random() < 0.4:
        suffix = "-" + random.choice(list("bcdefghij"))
        return f"{prefix}-{number}{suffix}"
    return f"{prefix}-{number}"


@style("nordic")
def nordic():
    """Norse/Germanic feel. Harsh, ancient."""
    starts = ["Arn", "Bjor", "Drak", "Eld", "Fjar", "Frost", "Gnei",
              "Graa", "Hraak", "Ivar", "Jorn", "Kval", "Mjol", "Nif",
              "Orm", "Ragn", "Skoll", "Thrym", "Ulf", "Val", "Vik",
              "Ygg", "Zorn"]
    endings = ["ar", "en", "ir", "heim", "gard", "dal", "fjord", "helm",
               "mar", "nir", "rath", "stein", "thor", "urd", "vir"]
    return random.choice(starts) + random.choice(endings)


@style("melodic")
def melodic():
    """Flowing, vowel-rich. Sounds like an elven place."""
    v = list("aeiou") + ["ae", "ai", "ao", "ea", "ei", "ia", "io", "oa", "oe", "ui"]
    c = list("lmnrsvwyz") + ["th", "sh", "ll", "nn", "ss", "rr"]
    length = random.randint(2, 4)
    syllables = []
    for _ in range(length):
        if random.random() < 0.5:
            syllables.append(random.choice(c) + random.choice(v))
        else:
            syllables.append(random.choice(v) + random.choice(c))
    name = "".join(syllables)
    return name.capitalize()


@style("guttural")
def guttural():
    """Heavy consonant clusters. Deep, menacing."""
    starts = ["Brak", "Drox", "Groth", "Khor", "Krax", "Kruun", "Morg",
              "Vrax", "Zrak", "Throx", "Grum", "Brox", "Drak", "Gorx",
              "Hrak", "Kroth", "Mrak", "Orbx", "Skrax", "Trox", "Vrox"]
    middles = ["", "ul", "or", "ax", "um", "oth", "ak", "ox"]
    endings = ["", "ar", "ax", "os", "oth", "um", "ur", "on", "ix", "us"]
    return random.choice(starts) + random.choice(middles) + random.choice(endings)


@style("sibilant")
def sibilant():
    """Hissing, whispering. Unsettling alien quality."""
    starts = ["Ss", "Xs", "Zs", "Sh", "Zh", "Xh", "Ssa", "Ssi",
              "Xiss", "Zhis", "Shal", "Xal", "Zseth", "Ssin", "Xsin"]
    cores = ["eth", "ith", "al", "iss", "ess", "ash", "esh", "uss",
             "ix", "ax", "eel", "ael", "aash", "oosh", "iish"]
    tails = ["", "aal", "ix", "iss", "ash", "eth", "ar", "is", "us",
             "el", "il", "ul"]
    return random.choice(starts) + random.choice(cores) + random.choice(tails)


@style("alien_glottal")
def alien_glottal():
    """Apostrophes mark glottal stops. Very non-human."""
    c1 = ["K", "G", "T", "V", "X", "Z", "Kh", "Gh", "Th", "Vr",
          "Xr", "Zr", "Kr", "Gr", "Tr"]
    v1 = ["a", "e", "i", "o", "u", "aa", "ee", "ii", "oo", "uu"]
    c2 = ["th", "kh", "gh", "vr", "xl", "zr", "kr", "gr", "tr",
          "nk", "ng", "sk", "sp", "st"]
    v2 = ["ul", "al", "el", "il", "ol", "aal", "eel", "iil", "ool"]
    parts = [random.choice(c1), random.choice(v1), "'",
             random.choice(c2), random.choice(v2)]
    return "".join(parts).capitalize()


@style("numeric_alien")
def numeric_alien():
    """Alien designations with embedded numbers — like a machine language."""
    alpha = ["Ax", "Bx", "Cx", "Dx", "Fx", "Gx", "Hx", "Jx",
             "Kx", "Lx", "Mx", "Nx", "Px", "Qx", "Rx", "Tx",
             "Vx", "Wx", "Yx", "Zx"]
    sep = random.choice(["-", ".", "~", ":"])
    n1 = random.randint(0, 9)
    n2 = random.randint(100, 999)
    tail = random.choice(["", "a", "b", "α", "β", "γ", "δ", "ε"])
    return f"{random.choice(alpha)}{n1}{sep}{n2}{tail}"


@style("agglutinative")
def agglutinative():
    """Long compound words. Feels like a real alien grammar."""
    roots = ["keth", "vala", "mori", "xenu", "drax", "soli", "thex",
             "vori", "kala", "meth", "xala", "dori", "soth", "thala",
             "veth", "kori", "mora", "xeth", "drala", "soxa"]
    connectors = ["", "-", "a", "i", "o", "u", "el", "al", "or"]
    n = random.randint(2, 3)
    parts = []
    for i in range(n):
        parts.append(random.choice(roots))
        if i < n - 1:
            parts.append(random.choice(connectors))
    return "".join(parts).capitalize()


@style("whisper")
def whisper():
    """Soft fricatives and nasals. Eerie, ethereal."""
    starts = ["Mh", "Nh", "Wh", "Fh", "Vh", "Sh", "Zh", "Mw",
              "Nw", "Fw", "Hw", "Sw", "Zw", "Mn", "Fn", "Wn"]
    vowels = ["ae", "ai", "au", "ea", "ei", "eu", "ia", "ie",
              "io", "oa", "oe", "ua", "ue", "ui", "ao", "oi"]
    endings = ["m", "n", "ng", "nf", "mf", "nw", "mw", "wn",
               "fn", "hn", "mn", "fw", "hw", "sw", "wm", "nm"]
    mid = random.choice(["", "l", "r", "n", "m", "w", "h", "wl", "mr"])
    return random.choice(starts) + random.choice(vowels) + mid + random.choice(endings)


@style("harsh_clicks")
def harsh_clicks():
    """Bantu/Khoisan-inspired. Uses click notation."""
    clicks = ["!", "|", "||", "≠"]
    starts = ["Kx", "Tx", "Nx", "Gx", "Dx", "Hx", "Qx", "Cx"]
    cores = ["hom", "aab", "eis", "uur", "oom", "aak", "iib", "ees",
             "aam", "uuk", "iin", "ool", "aar", "uun", "iil", "eem"]
    click = random.choice(clicks)
    start = random.choice(starts)
    core = random.choice(cores)
    return f"{start}{click}{core}"


# ---------------------------------------------------------------------------
# Generator
# ---------------------------------------------------------------------------

def generate(style_name=None, count=1):
    if style_name:
        if style_name not in STYLES:
            raise ValueError(f"Unknown style '{style_name}'. Use --list-styles to see options.")
        fn = STYLES[style_name]
        return [fn() for _ in range(count)]
    else:
        style_list = list(STYLES.values())
        return [random.choice(style_list)() for _ in range(count)]


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Random planet name generator")
    parser.add_argument("--style", help="Name generation style")
    parser.add_argument("--count", type=int, default=20, help="Number of names (default: 20)")
    parser.add_argument("--list-styles", action="store_true", help="Show available styles")
    parser.add_argument("--all-styles", action="store_true",
                        help="Show 3 examples from every style")
    args = parser.parse_args()

    if args.list_styles:
        print("Available styles:")
        for name, fn in STYLES.items():
            doc = fn.__doc__.strip().split("\n")[0]
            print(f"  {name:<18} {doc}")

    elif args.all_styles:
        for name, fn in STYLES.items():
            doc = fn.__doc__.strip().split("\n")[0]
            examples = ", ".join(generate(name, 3))
            print(f"{name:<18} {doc}")
            print(f"  → {examples}\n")

    else:
        names = generate(args.style, args.count)
        for name in names:
            print(name)