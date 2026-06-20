#!/usr/bin/env python3
"""
planet_gen.py — Random planet texture map generator
Outputs equirectangular (2:1) PNG maps ready for sphere-wrapping.

Planet types:
    terran · desert · gas_jupiter · gas_saturn · gas_ember
    gas_neptune · gas_uranus · gas_lilac · gas_teal
    lava · ice · ocean · toxic · barren

Usage:
    python planet_gen.py                         # one random planet
    python planet_gen.py -n 9                    # nine random planets
    python planet_gen.py -t terran               # force a type
    python planet_gen.py -t lava -s 1234         # seeded for reproducibility
    python planet_gen.py -n 20 -o ./textures     # batch to a folder
    python planet_gen.py --all                   # one of every type

Requires:
    pip install numpy Pillow scipy
"""

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter
import random, argparse, os

W, H = 1024, 512      # 2:1 equirectangular — change here if needed

# ── noise ───────────────────────────────────────────────────────────────────────

def snoise(sigma):
    """Smooth, x-tileable Gaussian noise normalised to [0, 1]."""
    r = np.random.randn(H, W).astype(np.float32)
    b = gaussian_filter(r, sigma=sigma, mode=('wrap', 'reflect'))
    lo, hi = b.min(), b.max()
    return (b - lo) / (hi - lo + 1e-9)

# ── colour helpers ──────────────────────────────────────────────────────────────

def cmap(field, stops):
    """
    Interpolate a [0,1] 2-D field through colour stops → float32 (H,W,3).
    stops: sorted list of (threshold, [R, G, B]) pairs.
    """
    xs = np.array([s[0] for s in stops], dtype=np.float32)
    cs = np.array([s[1] for s in stops], dtype=np.float32)   # (n, 3)
    flat = field.ravel()
    out  = np.empty((flat.size, 3), dtype=np.float32)
    for ch in range(3):
        out[:, ch] = np.interp(flat, xs, cs[:, ch])
    return out.reshape(H, W, 3)

def ice_poles(rgb, extent, strength=0.88):
    """Blend polar ice caps into an existing (H,W,3) map."""
    ice = np.array([232, 242, 255], dtype=np.float32)
    y   = np.linspace(0, 1, H, dtype=np.float32)[:, np.newaxis, np.newaxis]  # (H,1,1)
    t   = np.maximum(
              np.clip((extent - y) / (extent + 1e-6), 0, 1),
              np.clip((y - (1 - extent)) / (extent + 1e-6), 0, 1)
          ) ** 1.8 * strength
    return rgb * (1 - t) + ice * t

# ── planet generators ───────────────────────────────────────────────────────────

def terran():
    """Earth-like: ocean + continents + optional polar caps."""
    f  = np.clip(snoise(random.uniform(60, 115)) + snoise(22) * 0.18 - 0.09, 0, 1)
    wl = random.uniform(0.38, 0.60)     # water level
    lr = 1 - wl
    stops = [
        (0.00,         [  6,  18,  74]),   # deep ocean
        (wl * 0.45,    [ 11,  38, 116]),
        (wl * 0.85,    [ 21,  68, 150]),   # shallow water
        (wl,           [ 44, 103, 170]),   # coast
        (wl+lr*0.05,   [190, 170, 120]),   # beach/sand
        (wl+lr*0.22,   [ 70, 116,  46]),   # lowland grass
        (wl+lr*0.52,   [ 50,  86,  30]),   # forest
        (wl+lr*0.74,   [ 92,  78,  58]),   # highland / rock
        (1.00,         [218, 220, 226]),   # snow peaks
    ]
    rgb = cmap(f, stops)
    if random.random() < 0.70:
        rgb = ice_poles(rgb, random.uniform(0.07, 0.22))
    return rgb


def desert():
    """Arid rocky world: Mars-ish reds, tans, or alien mauve."""
    f   = np.clip(snoise(random.uniform(55, 105)) + snoise(16) * 0.14 - 0.07, 0, 1)
    pal = random.choice([
        # Mars-red
        [(0,[96,30,12]),(0.25,[140,56,26]),(0.55,[180,96,50]),(0.80,[210,140,86]),(1,[232,185,135])],
        # Tan ochre
        [(0,[136,86,36]),(0.3,[175,126,60]),(0.6,[210,166,90]),(0.85,[232,196,126]),(1,[248,226,180])],
        # Alien mauve
        [(0,[66,20,50]),(0.25,[106,40,70]),(0.55,[150,70,90]),(0.80,[190,110,116]),(1,[220,160,150])],
    ])
    rgb = cmap(f, pal)
    if random.random() < 0.25:
        rgb = ice_poles(rgb, random.uniform(0.04, 0.12), strength=0.32)
    return rgb


def gas_banded(style):
    """Jupiter/Saturn-style: turbulent horizontal bands with optional oval storm."""
    warp = (snoise(45) - 0.5) * H * random.uniform(0.06, 0.14)   # (H,W) pixel offsets
    Y    = np.arange(H, dtype=np.float32)[:, np.newaxis]           # (H,1)
    Yw   = np.clip(Y + warp, 0, H - 1)                             # (H,W) warped y
    freq = random.uniform(5, 13)
    band = (np.sin(Yw / H * np.pi * freq) + 1) * 0.5
    band = band * 0.72 + snoise(50) * 0.28

    pals = {
        'jupiter': [
            (0.00, [191, 120,  71]), (0.20, [226, 172, 116]),
            (0.40, [200, 146,  86]), (0.60, [236, 196, 150]),
            (0.80, [170,  91,  50]), (1.00, [210, 160, 106])],
        'saturn':  [
            (0.00, [190, 170, 116]), (0.25, [222, 200, 150]),
            (0.50, [202, 180, 126]), (0.75, [240, 226, 180]),
            (1.00, [180, 156, 100])],
        'ember':   [
            (0.00, [163,  50,  20]), (0.25, [210, 110,  50]),
            (0.50, [190,  70,  30]), (0.75, [230, 150,  80]),
            (1.00, [148,  40,  10])],
    }
    rgb = cmap(band, pals[style])

    # occasional oval storm (Great Red Spot style)
    if random.random() < 0.55:
        cx  = random.randint(W // 4, 3 * W // 4)
        cy  = random.randint(H // 3, 2 * H // 3)
        rx  = random.randint(50, 115)
        ry  = random.randint(22, 55)
        sc  = np.array(random.choice([[170,56,32],[210,130,66],[60,35,87]]), dtype=np.float32)
        gY, gX = np.mgrid[0:H, 0:W].astype(np.float32)
        t   = np.clip(1 - ((gX - cx) / rx)**2 - ((gY - cy) / ry)**2, 0, 1)[:, :, np.newaxis]
        rgb = rgb * (1 - t) + sc * t
    return rgb


def gas_smooth(style):
    """Neptune/Uranus-style: softer banding with more noise blended in."""
    warp = (snoise(60) - 0.5) * H * 0.07
    Y    = np.arange(H, dtype=np.float32)[:, np.newaxis]
    Yw   = np.clip(Y + warp, 0, H - 1)
    freq = random.uniform(3, 7)
    band = (np.sin(Yw / H * np.pi * freq) + 1) * 0.5
    band = band * 0.62 + snoise(65) * 0.38

    pals = {
        'neptune': [(0,[8,24,126]),(0.3,[17,53,175]),(0.6,[30,90,210]),(0.85,[56,130,230]),(1,[96,170,246])],
        'uranus':  [(0,[50,150,170]),(0.3,[70,180,196]),(0.6,[96,206,210]),(0.85,[130,224,220]),(1,[170,240,233])],
        'lilac':   [(0,[56, 9, 96]),(0.3,[96,28,150]),(0.6,[140,60,190]),(0.85,[176,100,216]),(1,[210,150,233])],
        'teal':    [(0,[6,70,73]),(0.3,[14,110,104]),(0.6,[30,150,136]),(0.85,[62,186,165]),(1,[104,216,193])],
    }
    return cmap(band, pals[style])


def lava():
    """Volcanic world: glowing lava rivers through dark cooling rock."""
    f  = np.clip(snoise(random.uniform(42, 78)) * 0.82 + snoise(14) * 0.28, 0, 1)
    lv = random.uniform(0.20, 0.34)    # lava threshold (low values = hot)
    lr = 1 - lv
    stops = [
        (0.00,         [255, 210,  36]),   # incandescent
        (lv * 0.35,    [255, 115,  12]),   # hot orange
        (lv * 0.75,    [186,  40,   4]),   # cooling red
        (lv,           [ 24,  10,   6]),   # dark rock edge
        (lv+lr*0.08,   [ 38,  20,  10]),
        (lv+lr*0.42,   [ 50,  27,  15]),
        (1.00,         [ 70,  56,  42]),   # cold rock
    ]
    return cmap(f, stops)


def ice_world():
    """Frozen world: pale blue/white with heavy polar caps and crevasse shadows."""
    f   = np.clip(snoise(random.uniform(65, 130)) - snoise(12) * 0.18, 0, 1)
    pal = random.choice([
        [(0,[90,132,190]),(0.3,[136,170,216]),(0.6,[186,206,234]),(0.85,[220,232,244]),(1,[244,249,254])],
        [(0,[50,115,176]),(0.3,[96,155,206]),(0.6,[150,196,228]),(0.85,[206,226,242]),(1,[244,249,254])],
        [(0,[170,190,206]),(0.3,[196,210,224]),(0.6,[216,226,236]),(0.85,[234,238,244]),(1,[249,252,254])],
    ])
    rgb = cmap(f, pal)
    return ice_poles(rgb, random.uniform(0.22, 0.46), strength=0.93)


def ocean_world():
    """Mostly ocean: deep-to-shallow gradients, scattered shallow banks."""
    f   = np.clip(snoise(random.uniform(55, 100)) * 0.75 + snoise(40) * 0.28, 0, 1)
    pal = random.choice([
        [(0,[3,8,56]),(0.2,[10,24,106]),(0.5,[16,58,152]),(0.75,[30,94,186]),(0.9,[60,133,210]),(1,[100,172,230])],
        [(0,[3,38,44]),(0.2,[10,70,78]),(0.5,[20,108,114]),(0.75,[40,144,143]),(0.9,[76,178,173]),(1,[120,208,201])],
        [(0,[26,3,54]),(0.2,[52,10,94]),(0.5,[82,22,138]),(0.75,[112,44,170]),(0.9,[148,80,196]),(1,[187,128,218])],
    ])
    return cmap(f, pal)


def toxic():
    """Alien world: weird-coloured seas and land — acid, purple brine, or sulphur."""
    f  = np.clip(snoise(random.uniform(52, 95)) + snoise(18) * 0.17 - 0.09, 0, 1)
    wl = random.uniform(0.32, 0.57)
    lr = 1 - wl
    pal = random.choice([
        # acid seas, orange-green land
        [(0,[15,78,6]),(wl*0.5,[24,120,14]),(wl,[34,142,24]),
         (wl+lr*0.08,[44,34,14]),(wl+lr*0.35,[88,104,18]),(wl+lr*0.70,[142,162,30]),(1,[182,205,50])],
        # purple ocean, rust land
        [(0,[24,6,54]),(wl*0.5,[50,15,94]),(wl,[82,24,138]),
         (wl+lr*0.08,[92,44,44]),(wl+lr*0.35,[137,74,54]),(wl+lr*0.70,[182,114,64]),(1,[210,154,83])],
        # ink sea, sulphur land
        [(0,[6,44,15]),(wl*0.5,[13,74,24]),(wl,[18,94,34]),
         (wl+lr*0.08,[73,54,6]),(wl+lr*0.35,[112,88,12]),(wl+lr*0.70,[152,132,19]),(1,[184,172,50])],
    ])
    return cmap(f, pal)


def barren():
    """Airless rock: grey moon, rust, dark volcanic, or pale tan."""
    f   = np.clip(snoise(random.uniform(42, 92)) + snoise(13) * 0.14 - 0.07, 0, 1)
    pal = random.choice([
        [(0,[53,48,44]),(0.25,[83,78,71]),(0.55,[113,108,101]),(0.80,[146,141,135]),(1,[190,185,180])],
        [(0,[74,24,15]),(0.25,[108,44,24]),(0.55,[148,74,44]),(0.80,[183,108,66]),(1,[222,163,116])],
        [(0,[14,12,16]),(0.25,[34,29,38]),(0.55,[58,52,64]),(0.80,[88,81,94]),(1,[132,125,142])],
        [(0,[94,91,84]),(0.25,[123,119,111]),(0.55,[152,148,140]),(0.80,[182,178,170]),(1,[212,208,202])],
    ])
    return cmap(f, pal)


# ── registry ────────────────────────────────────────────────────────────────────
#  (generator_fn, relative_weight)

TYPES = {
    'terran':      (terran,                         3),
    'desert':      (desert,                         2),
    'gas_jupiter': (lambda: gas_banded('jupiter'),  2),
    'gas_saturn':  (lambda: gas_banded('saturn'),   1),
    'gas_ember':   (lambda: gas_banded('ember'),    1),
    'gas_neptune': (lambda: gas_smooth('neptune'),  2),
    'gas_uranus':  (lambda: gas_smooth('uranus'),   1),
    'gas_lilac':   (lambda: gas_smooth('lilac'),    1),
    'gas_teal':    (lambda: gas_smooth('teal'),     1),
    'lava':        (lava,                           1),
    'ice':         (ice_world,                      2),
    'ocean':       (ocean_world,                    2),
    'toxic':       (toxic,                          2),
    'barren':      (barren,                         2),
}


def make_planet(ptype=None):
    """Return (uint8 H×W×3 array, type_name)."""
    if ptype:
        fn = TYPES[ptype][0]
    else:
        keys    = list(TYPES)
        weights = [TYPES[k][1] for k in keys]
        ptype   = random.choices(keys, weights=weights, k=1)[0]
        fn      = TYPES[ptype][0]
    rgb = fn()
    return np.clip(rgb, 0, 255).astype(np.uint8), ptype


# ── CLI ─────────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('-n', '--count',  type=int, default=1,       help='Planets to generate (default 1)')
    ap.add_argument('-t', '--type',   choices=list(TYPES),       help='Force a specific planet type')
    ap.add_argument('-s', '--seed',   type=int,                  help='RNG seed (increments for -n > 1)')
    ap.add_argument('-o', '--output', default='.',               help='Output directory (default .)')
    ap.add_argument('--all',          action='store_true',       help='Generate one of every type')
    args = ap.parse_args()

    os.makedirs(args.output, exist_ok=True)

    jobs = []
    if args.all:
        base = args.seed if args.seed is not None else random.randint(0, 0xFFFFFF)
        for i, ptype in enumerate(TYPES):
            jobs.append((base + i, ptype))
    else:
        for i in range(args.count):
            seed  = (args.seed + i) if args.seed is not None else random.randint(0, 0xFFFFFF)
            jobs.append((seed, args.type))   # args.type may be None → random

    total = len(jobs)
    for i, (seed, ptype) in enumerate(jobs):
        random.seed(seed)
        np.random.seed(seed)
        pixels, ptype_out = make_planet(ptype)
        fname = f'planet_{ptype_out}_{seed:06x}.png'
        fpath = os.path.join(args.output, fname)
        Image.fromarray(pixels, 'RGB').save(fpath)
        print(f'[{i+1}/{total}]  {ptype_out:14s}  seed={seed:06x}  →  {fpath}')

    print('Done.')


if __name__ == '__main__':
    main()