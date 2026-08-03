
#include "characterset.h"
#include "attribute.h"
#include "charset.h"     // auto-gen from gfx/charactersset.png
#include "charset2.h"    // from gfx/0to9.png

static unsigned char char_parallaxBlank[CHAR_Y];


// The teleport tile's 8-frame spinning-spoke cycle (3 bent spokes, no outer rim, 8 rotations 15
// degrees apart -- 3-fold spoke symmetry x 15 degrees/frame = 120 degrees over 8 frames, i.e.
// exactly a third of a full turn, so the loop is seamless) lives as real pixels in
// gfx/characterset.png row 15, columns 0-7 (previously hand-written byte arrays here -- moved
// into the source image and regenerated via tools/cset.py so it follows the same
// image-to-CHAR_MAP pipeline every other character does). Colour 7 (white) for the spokes,
// colour 1 (green) for the hub pixel at the centre (column 2, rows 4-5, where all 3 spokes
// converge in every frame) so the pivot point reads as a distinct colour from the spokes
// themselves. See AnimTeleport (animations.c) for the frame sequence/playback.


const unsigned char *const charSet[] = {


    // see ChName @ attribute.h
    // the CHAR_MAP_*_* defines come from charset.h
    // These are the x,y position in gfx/characterset.png of the character
    // This gives persistant mapping if new characters are added

    char_parallaxBlank,                 // 0 CH_BLANK,
    char_parallaxBlank,                 // 1 CH_PLACEHOLDER,
    CH(CHAR_MAP_characterset_0_2),      // 2 CH_DIRT,
    CH(CHAR_MAP_characterset_4_3),      // 3 CH_BRICKWALL,
    CH(CHAR_MAP_characterset_8_3),      // 4 CH_DOORCLOSED,
    CH(CHAR_MAP_characterset_2_17),     // 5 CH_DOOROPEN_0, -- AnimateDoor's final frame (animations.c)
    CH(CHAR_MAP_characterset_0_0),      // 6 CH_EXITBLANK,
    CH(CHAR_MAP_characterset_6_3),      // 7 CH_STEELWALL,
    CH(CHAR_MAP_characterset_1_2),      // 8 CH_PEBBLE1,
    CH(CHAR_MAP_characterset_2_2),      // 9 CH_PEBBLE2,
    CH(CHAR_MAP_characterset_2_11),     // 10 CH_ROCK,
    CH(CHAR_MAP_characterset_2_11),     // 11 CH_ROCK_FALLING,
    CH(CHAR_MAP_characterset_6_12),     // 12 CH_DOGE_00,
    CH(CHAR_MAP_characterset_6_12),     // 13 CH_DOGE_FALLING,
    CH(CHAR_MAP_characterset_0_0),      // 14 CH_MELLON_HUSK_BIRTH,
    CH(CHAR_MAP_characterset_0_0),      // 15 CH_LAVA_BLANK,
    CH(CHAR_MAP_characterset_8_2),      // 16 CH_LAVA_SMALL,
    CH(CHAR_MAP_characterset_7_2),      // 17 CH_LAVA_MEDIUM,
    CH(CHAR_MAP_characterset_6_2),      // 18 CH_LAVA_LARGE,
    CH(CHAR_MAP_characterset_0_0),      // 19 CH_MELLON_HUSK,
    CH(CHAR_MAP_characterset_6_12),     // 20 CH_DOGE_STATIC,
    CH(CHAR_MAP_characterset_9_12),     // 21 CH_PEBBLE_ROCK,
    CH(CHAR_MAP_characterset_6_12),     // 22 CH_ROCK_PEBBLE,
    CH(CHAR_MAP_characterset_0_7),      // 23 CH_ROCK_PEBBLE_1,
    CH(CHAR_MAP_characterset_3_5),      // 24 CH_DUST_0,
    CH(CHAR_MAP_characterset_4_5),      // 25 CH_DUST_1,
    CH(CHAR_MAP_characterset_5_5),      // 26 CH_DUST_2,
    CH(CHAR_MAP_characterset_0_7),      // 27 CH_GEODOGE,
    CH(CHAR_MAP_characterset_3_5),      // 28 CH_DUST_ROCK_0,
    CH(CHAR_MAP_characterset_5_5),      // 29 CH_DUST_ROCK_1,
    CH(CHAR_MAP_characterset_5_5),      // 30 CH_DUST_ROCK_2,
    CH(CHAR_MAP_characterset_6_12),     // 31 CH_CONVERT_GEODE_TO_DOGE,
    CH(CHAR_MAP_characterset_5_1),      // 32 CH_HORIZONTAL_BAR,
    CH(CHAR_MAP_characterset_6_1),      // 33 CH_PUSH_LEFT,
    CH(CHAR_MAP_characterset_6_1),      // 34 CH_PUSH_LEFT_REVERSE,
    CH(CHAR_MAP_characterset_0_1),      // 35 CH_PUSH_RIGHT,
    CH(CHAR_MAP_characterset_0_1),      // 36 CH_PUSH_RIGHT_REVERSE,
    CH(CHAR_MAP_characterset_1_1),      // 37 CH_VERTICAL_BAR,
    CH(CHAR_MAP_characterset_2_1),      // 38 CH_PUSH_UP,
    CH(CHAR_MAP_characterset_2_1),      // 39 CH_PUSH_UP_REVERSE,
    CH(CHAR_MAP_characterset_3_1),      // 40 CH_PUSH_DOWN,
    CH(CHAR_MAP_characterset_3_1),      // 41 CH_PUSH_DOWN_REVERSE,
    CH(CHAR_MAP_characterset_12_10),    // 42 CH_WYRM_BODY,
    CH(CHAR_MAP_characterset_13_10),    // 43 CH_WYRM_VERT_BODY,
    CH(CHAR_MAP_characterset_3_10),     // 44 CH_WYRM_CORNER_LD,
    CH(CHAR_MAP_characterset_2_10),     // 45 CH_WYRM_CORNER_RD,
    CH(CHAR_MAP_characterset_5_10),     // 46 CH_WYRM_CORNER_LU,
    CH(CHAR_MAP_characterset_4_10),     // 47 CH_WYRM_CORNER_RU,
    CH(CHAR_MAP_characterset_6_10),     // 48 CH_WYRM_HEAD_U,
    CH(CHAR_MAP_characterset_1_10),     // 49 CH_WYRM_HEAD_R,
    CH(CHAR_MAP_characterset_7_10),     // 50 CH_WYRM_HEAD_D,
    CH(CHAR_MAP_characterset_0_10),     // 51 CH_WYRM_HEAD_L,
    CH(CHAR_MAP_characterset_0_7),      // 52 CH_GEODOGE_FALLING,
    CH(CHAR_MAP_characterset_0_4),      // 53 CH_GRAVITY,
    CH(CHAR_MAP_characterset_1_4),      // 54 CH_GRAVITY,
    CH(CHAR_MAP_characterset_2_4),      // 55 CH_GRAVITY,
    CH(CHAR_MAP_characterset_2_3),      // 56 CH_BLOCK,
    CH(CHAR_MAP_characterset_0_13),     // 57 CH_GRINDER_0,
    CH(CHAR_MAP_characterset_1_13),     // 58 CH_GRINDER_1
    CH(CHAR_MAP_characterset_11_1),     // 59 CH_HUB,
    CH(CHAR_MAP_characterset_0_0),      // 60 CH_WATER_0,
    CH(CHAR_MAP_characterset_0_0),      // 61 CH_WATERFLOW_0 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 62 CH_WATERFLOW_1 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 63 CH_WATERFLOW_2 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 64 CH_WATERFLOW_3 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 65 CH_WATERFLOW_4 (disabled)
    CH(CHAR_MAP_characterset_10_1),     // 66 CH_HUB_1
    CH(CHAR_MAP_characterset_3_1),      // 67 CH_OUTLET
    CH(CHAR_MAP_characterset_2_13),     // 68 CH_BELT_0
    CH(CHAR_MAP_characterset_3_13),     // 69 CH_BELT_1
    CH(CHAR_MAP_characterset_4_1),      // 70 CH_PUSH_DOWN2,
    CH(CHAR_MAP_characterset_0_7),      // 71 CH_GEODOGE_CONVERT
    CH(CHAR_MAP_characterset_0_0),      // 72 CH_CONVERT_PIPE
    CH(CHAR_MAP_characterset_8_10),     // 73 CH_WYRM_TAIL_U,
    CH(CHAR_MAP_characterset_9_10),     // 74 CH_WYRM_TAIL_R,
    CH(CHAR_MAP_characterset_10_10),    // 75 CH_WYRM_TAIL_D,
    CH(CHAR_MAP_characterset_11_10),    // 76 CH_WYRM_TAIL_L,
    CH(CHAR_MAP_characterset_4_12),     // 77 CH_DOGE_FALLING_TOP,
    CH(CHAR_MAP_characterset_5_12),     // 78 CH_DOGE_FALLING_BOTTOM,
    CH(CHAR_MAP_characterset_0_11),     // 79 CH_ROCK_FALLING_TOP,
    CH(CHAR_MAP_characterset_1_11),     // 80 CH_ROCK_FALLING_BOTTOM,
    CH(CHAR_MAP_characterset_0_8),      // 81 CH_GEODOGE_FALLING_TOP,
    CH(CHAR_MAP_characterset_1_8),      // 82 CH_GEODOGE_FALLING_BOTTOM,
    CH(CHAR_MAP_characterset_4_12),     // 83 CH_DOGE_FALLING_TOP2,
    CH(CHAR_MAP_characterset_5_12),     // 84 CH_DOGE_FALLING_BOTTOM2,
    CH(CHAR_MAP_characterset_0_12),     // 85 CH_DOGE_SIDE_1,
    CH(CHAR_MAP_characterset_1_12),     // 86 CH_DOGE_SIDE_3
    CH(CHAR_MAP_characterset_2_12),     // 87 CH_DOGE_SIDE_2
    CH(CHAR_MAP_characterset_3_12),     // 88 CH_DOGE_SIDE_4
    CH(CHAR_MAP_characterset_1_0),      // 89 CH_ELECTRIC_0
    CH(CHAR_MAP_characterset_2_0),      // 90 CH_ELECTRIC_1
    CH(CHAR_MAP_characterset_3_0),      // 91 CH_ELECTRIC_2
    CH(CHAR_MAP_characterset_4_0),      // 92 CH_ELECTRIC_3
    CH(CHAR_MAP_characterset_1_5),      // 93 CH_BROKEN_DIRT
    CH(CHAR_MAP_characterset_6_0),      // 94 CH_INSULATOR_TOP
    CH(CHAR_MAP_characterset_7_0),      // 95 CH_INSULATOR_BOTTOM
    CH(CHAR_MAP_characterset_8_11),     // 96 CH_STAR
    CH(CHAR_MAP_characterset_9_11),     // 97 CH_STAR_TOP
    CH(CHAR_MAP_characterset_10_11),    // 98 CH_STAR_BOTTOM
    CH(CHAR_MAP_characterset_4_11),     // 99 CH_ROCK_BONUS
    CH(CHAR_MAP_characterset_8_11),     // 100 CH_STAR_EXPLODE
    CH(CHAR_MAP_characterset_13_0),     // 101 CH_INSULATOR_L
    CH(CHAR_MAP_characterset_14_0),     // 102 CH_INSULATOR_R
    CH(CHAR_MAP_characterset_8_0),      // 103 CH_ELECTRIC_H0
    CH(CHAR_MAP_characterset_9_0),      // 104 CH_ELECTRIC_H1
    CH(CHAR_MAP_characterset_10_0),     // 105 CH_ELECTRIC_H2
    CH(CHAR_MAP_characterset_11_0),     // 106 CH_ELECTRIC_H3
    CH(CHAR_MAP_characterset_12_0),     // 107 CH_CROSSED_STREAMS
    CH(CHAR_MAP_characterset_13_2),     // 108 CH_MOUNT_U
    CH(CHAR_MAP_characterset_14_2),     // 109 CH_MOUNT_D
    CH(CHAR_MAP_characterset_14_1),     // 110 CH_MOUNT_L
    CH(CHAR_MAP_characterset_13_1),     // 111 CH_MOUNT_R
    CH(CHAR_MAP_characterset_4_4),      // 112 CH_BOMB
    CH(CHAR_MAP_characterset_8_4),      // 113 CH_CRACKED_BRICK
    CH(CHAR_MAP_characterset_0_3),      // 114 CH_CONCRETE
    CH(CHAR_MAP_characterset_0_15),      // 115 CH_TELEPORT (spinning-spoke cycle, frame 0)
    CH(CHAR_MAP_characterset_3_2),       // 116 CH_KEY
    CH(CHAR_MAP_characterset_2_17),      // 117 CH_DOOROPEN_STATIC -- same graphic as CH_DOOROPEN_0
    CH(CHAR_MAP_characterset_4_6),       // 118 CH_IMMOVABLE
    CH(CHAR_MAP_characterset_4_6),       // 119 CH_IMMOVABLE_FALLING -- same graphic as CH_IMMOVABLE, mirrors CH_ROCK/CH_ROCK_FALLING
    CH(CHAR_MAP_characterset_5_6),       // 120 CH_IMMOVABLE_FALLING_TOP
    CH(CHAR_MAP_characterset_6_6),       // 121 CH_IMMOVABLE_FALLING_BOTTOM
    CH(CHAR_MAP_characterset_11_11),    // 122 CH_ROCK_SIDE_1
    CH(CHAR_MAP_characterset_13_11),    // 123 CH_ROCK_SIDE_2
    CH(CHAR_MAP_characterset_12_11),    // 124 CH_ROCK_SIDE_3
    CH(CHAR_MAP_characterset_14_11),    // 125 CH_ROCK_SIDE_4
    0,                                   // 126 (unused)
    0,                                   // 127 (unused)


    // "Animated" chars that do not appear on the board but are displayed
    // We do not need these to be < 128

    CH(CHAR_MAP_characterset_7_12),    // 128 CH_DOGE_01
    CH(CHAR_MAP_characterset_8_12),    // 129 CH_DOGE_02
    CH(CHAR_MAP_characterset_9_5),     // 130 CH_DOGE_03
    CH(CHAR_MAP_characterset_10_5),    // 131 CH_DOGE_04
    CH(CHAR_MAP_characterset_14_5),    // 132 CH_DOGE_05

    CH(CHAR_MAP_characterset_9_4),     // 133 CH_CRACKED_BRICK_1
    CH(CHAR_MAP_characterset_10_4),    // 134 CH_CRACKED_BRICK_2
    CH(CHAR_MAP_characterset_11_4),    // 135 CH_CRACKED_BRICK_3
    CH(CHAR_MAP_characterset_12_4),    // 136 CH_CRACKED_BRICK_4
    CH(CHAR_MAP_characterset_13_4),    // 137 CH_CRACKED_BRICK_5
    CH(CHAR_MAP_characterset_14_4),    // 138 CH_CRACKED_BRICK_6
    CH(CHAR_MAP_characterset_15_4),    // 139 CH_CRACKED_BRICK_7

    CH2(CHAR_MAP_0to9_0_1),    // 140 CH_0
    CH2(CHAR_MAP_0to9_1_1),    // 141 CH_1
    CH2(CHAR_MAP_0to9_2_1),    // 142 CH_2
    CH2(CHAR_MAP_0to9_3_1),    // 143 CH_3
    CH2(CHAR_MAP_0to9_4_1),    // 144 CH_4
    CH2(CHAR_MAP_0to9_5_1),    // 145 CH_5
    CH2(CHAR_MAP_0to9_6_1),    // 146 CH_6
    CH2(CHAR_MAP_0to9_7_1),    // 147 CH_7
    CH2(CHAR_MAP_0to9_8_1),    // 148 CH_8
    CH2(CHAR_MAP_0to9_9_1),    // 149 CH_9

    CH2(CHAR_MAP_0to9_0_3),    // 150 CH_A
    CH2(CHAR_MAP_0to9_1_3),    // 151 CH_B
    CH2(CHAR_MAP_0to9_2_3),    // 152 CH_C
    CH2(CHAR_MAP_0to9_3_3),    // 153 CH_D
    CH2(CHAR_MAP_0to9_4_3),    // 154 CH_E
    CH2(CHAR_MAP_0to9_5_3),    // 155 CH_F
    CH2(CHAR_MAP_0to9_6_3),    // 156 CH_G
    CH2(CHAR_MAP_0to9_7_3),    // 157 CH_H
    CH2(CHAR_MAP_0to9_8_3),    // 158 CH_I
    CH2(CHAR_MAP_0to9_9_3),    // 159 CH_J
    CH2(CHAR_MAP_0to9_0_4),    // 160 CH_K
    CH2(CHAR_MAP_0to9_1_4),    // 161 CH_L
    CH2(CHAR_MAP_0to9_2_4),    // 162 CH_M
    CH2(CHAR_MAP_0to9_3_4),    // 163 CH_N
    CH2(CHAR_MAP_0to9_4_4),    // 164 CH_O
    CH2(CHAR_MAP_0to9_5_4),    // 165 CH_P
    CH2(CHAR_MAP_0to9_6_4),    // 166 CH_Q
    CH2(CHAR_MAP_0to9_7_4),    // 167 CH_R
    CH2(CHAR_MAP_0to9_8_4),    // 168 CH_S
    CH2(CHAR_MAP_0to9_9_4),    // 169 CH_T
    CH2(CHAR_MAP_0to9_0_5),    // 170 CH_U
    CH2(CHAR_MAP_0to9_1_5),    // 171 CH_V
    CH2(CHAR_MAP_0to9_2_5),    // 172 CH_W
    CH2(CHAR_MAP_0to9_3_5),    // 173 CH_X
    CH2(CHAR_MAP_0to9_4_5),    // 174 CH_Y
    CH2(CHAR_MAP_0to9_5_5),    // 175 CH_Z

    CH2(CHAR_MAP_0to9_0_7),    // 176 CH_A
    CH2(CHAR_MAP_0to9_1_7),    // 177 CH_B
    CH2(CHAR_MAP_0to9_2_7),    // 178 CH_C
    CH2(CHAR_MAP_0to9_3_7),    // 179 CH_D
    CH2(CHAR_MAP_0to9_4_7),    // 180 CH_E
    CH2(CHAR_MAP_0to9_5_7),    // 181 CH_F
    CH2(CHAR_MAP_0to9_6_7),    // 182 CH_G
    CH2(CHAR_MAP_0to9_7_7),    // 183 CH_H
    CH2(CHAR_MAP_0to9_8_7),    // 184 CH_I
    CH2(CHAR_MAP_0to9_9_7),    // 185 CH_J
    CH2(CHAR_MAP_0to9_0_8),    // 186 CH_K
    CH2(CHAR_MAP_0to9_1_8),    // 187 CH_L
    CH2(CHAR_MAP_0to9_2_8),    // 188 CH_M
    CH2(CHAR_MAP_0to9_3_8),    // 189 CH_N
    CH2(CHAR_MAP_0to9_4_8),    // 190 CH_O
    CH2(CHAR_MAP_0to9_5_8),    // 191 CH_P
    CH2(CHAR_MAP_0to9_6_8),    // 192 CH_Q
    CH2(CHAR_MAP_0to9_7_8),    // 193 CH_R
    CH2(CHAR_MAP_0to9_8_8),    // 194 CH_S
    CH2(CHAR_MAP_0to9_9_8),    // 195 CH_T
    CH2(CHAR_MAP_0to9_0_9),    // 196 CH_U
    CH2(CHAR_MAP_0to9_1_9),    // 197 CH_V
    CH2(CHAR_MAP_0to9_2_9),    // 198 CH_W
    CH2(CHAR_MAP_0to9_3_9),    // 199 CH_X
    CH2(CHAR_MAP_0to9_4_9),    // 200 CH_Y
    CH2(CHAR_MAP_0to9_5_9),    // 201 CH_Z

    CH2(CHAR_MAP_0to9_0_2),    // 202 CH_PLUS

    CH2(CHAR_MAP_0to9_0_0),    // 203 CH_BIG_0
    CH2(CHAR_MAP_0to9_1_0),    // 204 CH_BIG_1
    CH2(CHAR_MAP_0to9_2_0),    // 205 CH_BIG_2
    CH2(CHAR_MAP_0to9_3_0),    // 206 CH_BIG_3
    CH2(CHAR_MAP_0to9_4_0),    // 207 CH_BIG_4
    CH2(CHAR_MAP_0to9_5_0),    // 208 CH_BIG_5
    CH2(CHAR_MAP_0to9_6_0),    // 209 CH_BIG_6
    CH2(CHAR_MAP_0to9_7_0),    // 210 CH_BIG_7
    CH2(CHAR_MAP_0to9_8_0),    // 211 CH_BIG_8
    CH2(CHAR_MAP_0to9_9_0),    // 212 CH_BIG_9

    CH(CHAR_MAP_characterset_1_15),    // 213 CH_TELEPORT_1
    CH(CHAR_MAP_characterset_2_15),    // 214 CH_TELEPORT_2
    CH(CHAR_MAP_characterset_3_15),    // 215 CH_TELEPORT_3
    CH(CHAR_MAP_characterset_4_15),    // 216 CH_TELEPORT_4
    CH(CHAR_MAP_characterset_5_15),    // 217 CH_TELEPORT_5
    CH(CHAR_MAP_characterset_6_15),    // 218 CH_TELEPORT_6
    CH(CHAR_MAP_characterset_7_15),    // 219 CH_TELEPORT_7

    CH(CHAR_MAP_characterset_0_17),    // 220 CH_DOORSLIDE_1
};

_Static_assert(sizeof(charSet) / sizeof(charSet[0]) == CH_MAX, "charSet table wrong size");

// EOF
