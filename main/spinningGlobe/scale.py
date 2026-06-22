import math

def make_table(n_entries, texture_height, char_height, step=3, name="line85", linear=False):
    """
    n_entries:      number of valid table entries (= max equiv value + 1)
    texture_height: total pixel rows in the texture
    char_height:    scanlines per character cell (e.g. 30)
    step:           sub-position quantisation (keep at 3)
    linear:         if True, map linearly; if False, use spherical (arccos) mapping
    """
    print(f"\nconst short int {name}[] = {{")
    for i in range(n_entries):
        t = i / (n_entries - 1)                      # 0.0 (top) to 1.0 (bottom)
        if linear:
            v = t
        else:
            theta = math.acos(1.0 - 2.0 * t)        # 0 (north pole) to pi (south pole)
            v = theta / math.pi                      # texture V coordinate
        tex_row = v * texture_height
        char_row = int(tex_row) // char_height
        sub = (round(int(tex_row) % char_height / step)) * step
        sub = min(sub, char_height - step)
        print(f"    {(char_row << 5) | sub},\t// {i}")
    # sentinel padding
    pad = (8 - n_entries % 8) % 8 or 8
    print("    " + ",  ".join(["-1"] * pad) + ",")
    print("};\n")

n_entries      = int(input("Number of table entries (57 for original): ") or "57")
texture_height = int(input("Texture height in pixels:                  "))
char_height    = int(input("Character cell height in scanlines:        "))
step           = int(input("Sub-position step [3]:                     ") or "3")
name           = input("Array name [line85]:                       ").strip() or "line85"
mode           = input("Mapping - (s)pherical or (l)inear [s]:     ").strip().lower() or "s"

make_table(n_entries, texture_height, char_height, step, name, linear=(mode == "l"))