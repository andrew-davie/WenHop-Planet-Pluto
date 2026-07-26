
#include "characterset.h"
#include "attribute.h"
#include "charset.h"     // auto-gen from gfx/charactersset.png
#include "charset2.h"    // from gfx/0to9.png

static unsigned char char_parallaxBlank[CHAR_Y];


const unsigned char *const charSet[] = {


    // see ChName @ attribute.h
    // the CHAR_MAP_*_* defines come from charset.h
    // These are the x,y position in gfx/characterset.png of the character
    // This gives persistant mapping if new characters are added

    char_parallaxBlank,                 // 000 CH_BLANK,
    char_parallaxBlank,                 // 001 CH_PLACEHOLDER,
    CH(CHAR_MAP_characterset_0_2),      // 002 CH_DIRT,
    CH(CHAR_MAP_characterset_4_3),      // 003 CH_BRICKWALL,
    CH(CHAR_MAP_characterset_8_3),      // 004 CH_DOORCLOSED,
    CH(CHAR_MAP_characterset_8_3),      // 005 CH_DOOROPEN_0,
    CH(CHAR_MAP_characterset_0_0),      // 006 CH_EXITBLANK,
    CH(CHAR_MAP_characterset_6_3),      // 007 CH_STEELWALL,
    CH(CHAR_MAP_characterset_1_2),      // 008 CH_PEBBLE1,
    CH(CHAR_MAP_characterset_2_2),      // 009 CH_PEBBLE2,
    CH(CHAR_MAP_characterset_2_11),     // 010 CH_ROCK,
    CH(CHAR_MAP_characterset_2_11),     // 011 CH_ROCK_FALLING,
    CH(CHAR_MAP_characterset_6_12),     // 012 CH_DOGE_00,
    CH(CHAR_MAP_characterset_6_12),     // 013 CH_DOGE_FALLING,
    CH(CHAR_MAP_characterset_0_0),      // 014 CH_MELLON_HUSK_BIRTH,
    CH(CHAR_MAP_characterset_0_0),      // 015 CH_LAVA_BLANK,
    CH(CHAR_MAP_characterset_8_2),      // 016 CH_LAVA_SMALL,
    CH(CHAR_MAP_characterset_7_2),      // 017 CH_LAVA_MEDIUM,
    CH(CHAR_MAP_characterset_6_2),      // 018 CH_LAVA_LARGE,
    CH(CHAR_MAP_characterset_0_0),      // 019 CH_MELLON_HUSK,
    CH(CHAR_MAP_characterset_6_12),     // 020 CH_DOGE_STATIC,
    CH(CHAR_MAP_characterset_9_12),     // 021 CH_PEBBLE_ROCK,
    CH(CHAR_MAP_characterset_6_12),     // 022 CH_ROCK_PEBBLE,
    CH(CHAR_MAP_characterset_0_7),      // 023 CH_ROCK_PEBBLE_1,
    CH(CHAR_MAP_characterset_3_5),      // 024 CH_DUST_0,
    CH(CHAR_MAP_characterset_4_5),      // 025 CH_DUST_1,
    CH(CHAR_MAP_characterset_5_5),      // 026 CH_DUST_2,
    CH(CHAR_MAP_characterset_0_7),      // 027 CH_GEODOGE,
    CH(CHAR_MAP_characterset_3_5),      // 028 CH_DUST_ROCK_0,
    CH(CHAR_MAP_characterset_5_5),      // 029 CH_DUST_ROCK_1,
    CH(CHAR_MAP_characterset_5_5),      // 030 CH_DUST_ROCK_2,
    CH(CHAR_MAP_characterset_6_12),     // 031 CH_CONVERT_GEODE_TO_DOGE,
    CH(CHAR_MAP_characterset_5_1),      // 032 CH_HORIZONTAL_BAR,
    CH(CHAR_MAP_characterset_6_1),      // 033 CH_PUSH_LEFT,
    CH(CHAR_MAP_characterset_6_1),      // 034 CH_PUSH_LEFT_REVERSE,
    CH(CHAR_MAP_characterset_0_1),      // 035 CH_PUSH_RIGHT,
    CH(CHAR_MAP_characterset_0_1),      // 036 CH_PUSH_RIGHT_REVERSE,
    CH(CHAR_MAP_characterset_1_1),      // 037 CH_VERTICAL_BAR,
    CH(CHAR_MAP_characterset_2_1),      // 038 CH_PUSH_UP,
    CH(CHAR_MAP_characterset_2_1),      // 039 CH_PUSH_UP_REVERSE,
    CH(CHAR_MAP_characterset_3_1),      // 040 CH_PUSH_DOWN,
    CH(CHAR_MAP_characterset_3_1),      // 041 CH_PUSH_DOWN_REVERSE,
    CH(CHAR_MAP_characterset_12_10),    // 042 CH_WYRM_BODY,
    CH(CHAR_MAP_characterset_13_10),    // 043 CH_WYRM_VERT_BODY,
    CH(CHAR_MAP_characterset_3_10),     // 044 CH_WYRM_CORNER_LD,
    CH(CHAR_MAP_characterset_2_10),     // 045 CH_WYRM_CORNER_RD,
    CH(CHAR_MAP_characterset_5_10),     // 046 CH_WYRM_CORNER_LU,
    CH(CHAR_MAP_characterset_4_10),     // 047 CH_WYRM_CORNER_RU,
    CH(CHAR_MAP_characterset_6_10),     // 048 CH_WYRM_HEAD_U,
    CH(CHAR_MAP_characterset_1_10),     // 049 CH_WYRM_HEAD_R,
    CH(CHAR_MAP_characterset_7_10),     // 050 CH_WYRM_HEAD_D,
    CH(CHAR_MAP_characterset_0_10),     // 051 CH_WYRM_HEAD_L,
    CH(CHAR_MAP_characterset_0_7),      // 052 CH_GEODOGE_FALLING,
    CH(CHAR_MAP_characterset_0_4),      // 053 CH_GRAVITY,
    CH(CHAR_MAP_characterset_1_4),      // 054 CH_GRAVITY,
    CH(CHAR_MAP_characterset_2_4),      // 055 CH_GRAVITY,
    CH(CHAR_MAP_characterset_2_3),      // 056 CH_BLOCK,
    CH(CHAR_MAP_characterset_0_13),     // 057 CH_GRINDER_0,
    CH(CHAR_MAP_characterset_1_13),     // 058 CH_GRINDER_1
    CH(CHAR_MAP_characterset_11_1),     // 059 CH_HUB,
    CH(CHAR_MAP_characterset_0_0),      // 060 CH_WATER_0,
    CH(CHAR_MAP_characterset_0_0),      // 061 CH_WATERFLOW_0 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 062 CH_WATERFLOW_1 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 063 CH_WATERFLOW_2 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 064 CH_WATERFLOW_3 (disabled)
    CH(CHAR_MAP_characterset_0_0),      // 065 CH_WATERFLOW_4 (disabled)
    CH(CHAR_MAP_characterset_10_1),     // 066 CH_HUB_1
    CH(CHAR_MAP_characterset_3_1),      // 067 CH_OUTLET
    CH(CHAR_MAP_characterset_2_13),     // 068 CH_BELT_0
    CH(CHAR_MAP_characterset_3_13),     // 069 CH_BELT_1
    CH(CHAR_MAP_characterset_4_1),      // 070 CH_PUSH_DOWN2,
    CH(CHAR_MAP_characterset_0_7),      // 071 CH_GEODOGE_CONVERT
    CH(CHAR_MAP_characterset_0_0),      // 072 CH_CONVERT_PIPE
    CH(CHAR_MAP_characterset_8_10),     // 073 CH_WYRM_TAIL_U,
    CH(CHAR_MAP_characterset_9_10),     // 074 CH_WYRM_TAIL_R,
    CH(CHAR_MAP_characterset_10_10),    // 075 CH_WYRM_TAIL_D,
    CH(CHAR_MAP_characterset_11_10),    // 076 CH_WYRM_TAIL_L,
    CH(CHAR_MAP_characterset_4_12),     // 077 CH_DOGE_FALLING_TOP,
    CH(CHAR_MAP_characterset_5_12),     // 078 CH_DOGE_FALLING_BOTTOM,
    CH(CHAR_MAP_characterset_0_11),     // 079 CH_ROCK_FALLING_TOP,
    CH(CHAR_MAP_characterset_1_11),     // 080 CH_ROCK_FALLING_BOTTOM,
    CH(CHAR_MAP_characterset_0_8),      // 081 CH_GEODOGE_FALLING_TOP,
    CH(CHAR_MAP_characterset_1_8),      // 082 CH_GEODOGE_FALLING_BOTTOM,
    CH(CHAR_MAP_characterset_4_12),     // 083 CH_DOGE_FALLING_TOP2,
    CH(CHAR_MAP_characterset_5_12),     // 084 CH_DOGE_FALLING_BOTTOM2,
    CH(CHAR_MAP_characterset_0_12),     // 085 CH_DOGE_SIDE_1,
    CH(CHAR_MAP_characterset_1_12),     // 086 CH_DOGE_SIDE_3
    CH(CHAR_MAP_characterset_2_12),     // 087 CH_DOGE_SIDE_2
    CH(CHAR_MAP_characterset_3_12),     // 088 CH_DOGE_SIDE_4
    CH(CHAR_MAP_characterset_1_0),      // 089 CH_ELECTRIC_0
    CH(CHAR_MAP_characterset_2_0),      // 090 CH_ELECTRIC_1
    CH(CHAR_MAP_characterset_3_0),      // 091 CH_ELECTRIC_2
    CH(CHAR_MAP_characterset_4_0),      // 092 CH_ELECTRIC_3
    CH(CHAR_MAP_characterset_1_5),      // 093 CH_BROKEN_DIRT
    CH(CHAR_MAP_characterset_6_0),      // 094 CH_INSULATOR_TOP
    CH(CHAR_MAP_characterset_7_0),      // 095 CH_INSULATOR_BOTTOM
    CH(CHAR_MAP_characterset_8_11),     // 096 CH_STAR
    CH(CHAR_MAP_characterset_9_11),     // 097 CH_STAR_TOP
    CH(CHAR_MAP_characterset_10_11),    // 098 CH_STAR_BOTTOM
    CH(CHAR_MAP_characterset_4_11),     // 099 CH_ROCK_BONUS
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
    CH(CHAR_MAP_characterset_0_9),      // 112 CH_PIT_L0
    CH(CHAR_MAP_characterset_1_9),      // 113 CH_PIT_R0
    CH(CHAR_MAP_characterset_4_4),      // 114 CH_BOMB
    CH(CHAR_MAP_characterset_8_4),      // 115 CH_CRACKED_BRICK
    CH(CHAR_MAP_characterset_0_3),      // 116 CH_CONCRETE
    0,                                  // 117
    0,                                  // 118
    0,                                  // 119
    0,                                  // 120
    0,                                  // 121
    0,                                  // 122
    0,                                  // 123
    0,                                  // 124
    0,                                  // 125
    0,                                  // 126
    0,                                  // 127


    // "Animated" chars that do not appear on the board but are displayed
    // We do not need these to be < 128

    CH(CHAR_MAP_characterset_7_12),    // 128 CH_DOGE_01
    CH(CHAR_MAP_characterset_8_12),    // 129 CH_DOGE_02
    CH(CHAR_MAP_characterset_9_5),     // 130 CH_DOGE_03
    CH(CHAR_MAP_characterset_10_5),    // 131 CH_DOGE_04
    CH(CHAR_MAP_characterset_14_5),    // 132 CH_DOGE_05
                                       //
    CH(CHAR_MAP_characterset_3_9),     // 133 CH_PIT_L1
    CH(CHAR_MAP_characterset_4_9),     // 134 CH_PIT_R1
    CH(CHAR_MAP_characterset_6_9),     // 135 CH_PIT_L2
    CH(CHAR_MAP_characterset_7_9),     // 136 CH_PIT_R2
    CH(CHAR_MAP_characterset_9_9),     // 137 CH_PIT_L3
    CH(CHAR_MAP_characterset_10_9),    // 138 CH_PIT_R3
    CH(CHAR_MAP_characterset_12_9),    // 139 CH_PIT_L4
    CH(CHAR_MAP_characterset_13_9),    // 140 CH_PIT_R4
                                       //
    CH(CHAR_MAP_characterset_5_4),     // 141 CH_BOMB_FLASH
                                       //
    CH(CHAR_MAP_characterset_9_4),     // 142 CH_CRACKED_BRICK_1
    CH(CHAR_MAP_characterset_10_4),    // 143 CH_CRACKED_BRICK_2
    CH(CHAR_MAP_characterset_11_4),    // 144 CH_CRACKED_BRICK_3
    CH(CHAR_MAP_characterset_12_4),    // 145 CH_CRACKED_BRICK_4
    CH(CHAR_MAP_characterset_13_4),    // 146 CH_CRACKED_BRICK_5
    CH(CHAR_MAP_characterset_14_4),    // 147 CH_CRACKED_BRICK_6
    CH(CHAR_MAP_characterset_15_4),    // 148 CH_CRACKED_BRICK_7
                                       //
    CH(CHAR_MAP_characterset_3_11),    // 149 CH_RUBBLE

    CH2(CHAR_MAP_0to9_0_1),    // 150 CH_0
    CH2(CHAR_MAP_0to9_1_1),    // 151 CH_1
    CH2(CHAR_MAP_0to9_2_1),    // 152 CH_2
    CH2(CHAR_MAP_0to9_3_1),    // 153 CH_3
    CH2(CHAR_MAP_0to9_4_1),    // 154 CH_4
    CH2(CHAR_MAP_0to9_5_1),    // 155 CH_5
    CH2(CHAR_MAP_0to9_6_1),    // 156 CH_6
    CH2(CHAR_MAP_0to9_7_1),    // 157 CH_7
    CH2(CHAR_MAP_0to9_8_1),    // 158 CH_8
    CH2(CHAR_MAP_0to9_9_1),    // 159 CH_9

    CH2(CHAR_MAP_0to9_0_3),    // 160 CH_A
    CH2(CHAR_MAP_0to9_1_3),    // 161 CH_B
    CH2(CHAR_MAP_0to9_2_3),    // 162 CH_C
    CH2(CHAR_MAP_0to9_3_3),    // 163 CH_D
    CH2(CHAR_MAP_0to9_4_3),    // 164 CH_E
    CH2(CHAR_MAP_0to9_5_3),    // 160 CH_F
    CH2(CHAR_MAP_0to9_6_3),    // 161 CH_G
    CH2(CHAR_MAP_0to9_7_3),    // 162 CH_H
    CH2(CHAR_MAP_0to9_8_3),    // 163 CH_I
    CH2(CHAR_MAP_0to9_9_3),    // 164 CH_J
    CH2(CHAR_MAP_0to9_0_4),    // 160 CH_K
    CH2(CHAR_MAP_0to9_1_4),    // 161 CH_L
    CH2(CHAR_MAP_0to9_2_4),    // 162 CH_M
    CH2(CHAR_MAP_0to9_3_4),    // 163 CH_N
    CH2(CHAR_MAP_0to9_4_4),    // 164 CH_O
    CH2(CHAR_MAP_0to9_5_4),    // 160 CH_P
    CH2(CHAR_MAP_0to9_6_4),    // 161 CH_Q
    CH2(CHAR_MAP_0to9_7_4),    // 162 CH_R
    CH2(CHAR_MAP_0to9_8_4),    // 163 CH_S
    CH2(CHAR_MAP_0to9_9_4),    // 164 CH_T
    CH2(CHAR_MAP_0to9_0_5),    // 160 CH_U
    CH2(CHAR_MAP_0to9_1_5),    // 161 CH_V
    CH2(CHAR_MAP_0to9_2_5),    // 162 CH_W
    CH2(CHAR_MAP_0to9_3_5),    // 163 CH_X
    CH2(CHAR_MAP_0to9_4_5),    // 164 CH_Y
    CH2(CHAR_MAP_0to9_5_5),    // 164 CH_Z

};

_Static_assert(sizeof(charSet) / sizeof(charSet[0]) == CH_MAX, "charSet table wrong size");

// EOF
