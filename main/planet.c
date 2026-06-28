#include "planet.h"
#include "main.h"

#include "spinningGlobe/blood2.h"
#include "spinningGlobe/bloodworld.h"
#include "spinningGlobe/earth.h"
#include "spinningGlobe/green1.h"
#include "spinningGlobe/lava.h"
#include "spinningGlobe/moon.h"
#include "spinningGlobe/neptune.h"
#include "spinningGlobe/pangea.h"
#include "spinningGlobe/terra.h"
#include "spinningGlobe/titan.h"


const unsigned char neptune_ntsc_palette_override[3] = {
    0x92, /* palette[1] = (28,56,144) */
    0x94, /* palette[2] = (56,84,168) */
    0x96, /* palette[4] = (80,116,188) */
};

const unsigned char jupiter_ntsc_palette_override[3] = {
    0x4C, /* palette[1] = (108,108,108) */
    0x18, /* palette[2] = (144,144,144) */
    0x2A, /* palette[4] = (176,176,176) */
};

const unsigned char earth_ntsc_palette_override[3] = {
    0xA4, /* palette[1] = (28,76,120) */
    0xC6, /* palette[2] = (104,112,52) */
    0x16, /* palette[4] = (212,188,248) */
};

const unsigned char moon_ntsc_palette_override[3] = {
    0x14,
    /* palette[1] = (108,108,108) */ 0xE4, /* palette[2] = (144,144,144) */
    0xE2,                                  /* palette[4] = (176,176,176) */
};
const unsigned char moon_ntsc_palette_override2[3] = {
    0x84, /* palette[1] = (108,108,108) */
    0xC4, /* palette[2] = (144,144,144) */
    0x24, /* palette[4] = (176,176,176) */
};


const unsigned char bloodworld_ntsc_palette_override[3] = {
    0x40, /* palette[1] = (68,40,0) */
    0x42, /* palette[2] = (100,72,24) */
    0xC6, /* palette[4] = (144,144,144) */
};

const unsigned char pangea_ntsc_palette_override[3] = {
    0x84, /* palette[1] = (0,44,92) */
    0xD4, /* palette[2] = (64,64,64) */
    0x32, /* palette[4] = (176,176,176) */
};

const unsigned char blood2_ntsc_palette_override[3] = {
    0xF6, /* palette[1] = (68,40,0) */
    0x94, /* palette[2] = (132,104,48) */
    0x64, /* palette[4] = (176,176,176) */
};

const unsigned char green1_ntsc_palette_override[3] = {
    0x42, /* palette[1] = (0,60,44) */
    0x94, /* palette[2] = (32,92,32) */
    0x16, /* palette[4] = (108,108,108) */
};

const unsigned char ridged_ntsc_palette_override[3] = {
    0x12, /* palette[1] = (0,44,92) */
    0xDA, /* palette[2] = (64,64,64) */
    0xCA, /* palette[4] = (144,144,144) */
};

const unsigned char lava_ntsc_palette_override[3] = {
    0xE2, /* palette[1] = (44,48,0) */
    0x30, /* palette[2] = (132,24,0) */
    0x34, /* palette[4] = (172,80,48) */
};

const unsigned char terra_ntsc_palette_override[3] = {
    0xC6, /* palette[1] = (44,48,0) */
    0x74, /* palette[2] = (132,24,0) */
    0x42, /* palette[4] = (172,80,48) */
};


// TODO: run spinningGlobe/make.sh to re-gen the planet data
//       run python3 spinningGlobe/planet-gen.py to create new planet images

// To add a planet

//  a) put planet texture map in spinningGlobes/textures director (e.g., newplanet.jpg)
//  b) run cset.py from spinningGlobes directory
//     suggested params:
//     --no-dither
//     --trixel-height 10 --trixel-width 5
//      --adaptive-palette --black-threshold 20
//      --brightness 1.0
//     --max-chars 128 textures/titan.png 20 4
//  c) optionally add planet to spinningGlobes/make.sh
//
//  Note: a reconstructed image is placed in spinningGlobes (newplanet_recon.png)

//  1) enter filename in makefile SRCS list (e.g., spinningGlobes/newplanet.c)
//  2) enter map, charset, palette entries in planets[] table below
//     note: the palette can be replaced (copy from newplanet.c to above, and append _override)
//  3) "#include spinningGlobe/newplanet.h" at the top of displayPlanet.c


const struct GLOBE planets[MAX_PLANET] = {

    {">Terror", 8, ">16900 km|1$81 m/s^", 1, terra_map, terra_charset, terra_ntsc_palette_override},           // 0
    {">Earth", 8, ">6900 km|9$81 m/s^", 1, earth_map, earth_charset, earth_ntsc_palette_override},             // 1
    {">Xe'drith", 8, ">1500 km|4$8 m/s^", -1, lava_map, lava_charset, lava_ntsc_palette_override},             // 2
    {">Neptune", 8, ">9874 km|1$5 m/s^", 1, neptune_map, neptune_charset, neptune_ntsc_palette_override},      // 3
    {">WASP-76b", 8, ">14566 km|4$3 m/s^", -1, green1_map, green1_charset, green1_ntsc_palette_override},      // 4
    {">Skumveil", 8, ">42000 km|222$4 m/s^", -1, pangea_map, pangea_charset, pangea_ntsc_palette_override},    // 5
    {">Grunthos", 8, ">89000 km|11$15 m/s^", 1, bloodworld_map, bloodworld_charset,
     bloodworld_ntsc_palette_override},                                                                   // 6
    {">Swilint", 8, ">6800 km|2$3 m/s^", 1, blood2_map, blood2_charset, blood2_ntsc_palette_override},    // 7
    {">Lichoni", 8, ">12400 km|16$8 m/s^", 1, titan_map, titan_charset, titan_ntsc_palette},              // 8
    {">Muckspon", 8, ">4522 km|22$2 m/s^", -1, moon_map, moon_charset, moon_ntsc_palette_override},       // 9
    {">Planet X", 8, ">14530 km|5$2 m/s^", 1, moon_map, moon_charset, moon_ntsc_palette_override2},       // 10
};


// EOF
