#pragma once

#define CARS_CHAR_WIDTH 5
#define CARS_CHAR_HEIGHT 11

typedef struct {
    unsigned char data[CARS_CHAR_HEIGHT * 3]; // RGB
} character;

extern const character charset[49];

extern const unsigned char gfx_cars_blank_gif_map[1][1];
extern const unsigned char gfx_cars_numbers_gif_map[16][2];
extern const unsigned char gfx_cars_blip_gif_map[3][10];

#define CHAR_MAP_blank_0_0 0
#define CHAR_MAP_numbers_0_0 1
#define CHAR_MAP_numbers_0_1 2
#define CHAR_MAP_numbers_0_2 3
#define CHAR_MAP_numbers_1_2 4
#define CHAR_MAP_numbers_0_3 5
#define CHAR_MAP_numbers_0_4 6
#define CHAR_MAP_numbers_0_5 7
#define CHAR_MAP_numbers_0_6 8
#define CHAR_MAP_numbers_1_6 9
#define CHAR_MAP_numbers_0_7 10
#define CHAR_MAP_numbers_1_7 11
#define CHAR_MAP_numbers_0_8 12
#define CHAR_MAP_numbers_0_9 13
#define CHAR_MAP_numbers_0_10 14
#define CHAR_MAP_numbers_1_10 15
#define CHAR_MAP_numbers_0_11 16
#define CHAR_MAP_numbers_1_11 17
#define CHAR_MAP_numbers_0_12 18
#define CHAR_MAP_numbers_1_12 19
#define CHAR_MAP_numbers_0_13 20
#define CHAR_MAP_numbers_1_13 21
#define CHAR_MAP_numbers_0_14 22
#define CHAR_MAP_numbers_1_14 23
#define CHAR_MAP_numbers_1_15 24
#define CHAR_MAP_blip_0_0 0
#define CHAR_MAP_blip_1_0 25
#define CHAR_MAP_blip_2_0 26
#define CHAR_MAP_blip_3_0 27
#define CHAR_MAP_blip_4_0 28
#define CHAR_MAP_blip_5_0 29
#define CHAR_MAP_blip_6_0 30
#define CHAR_MAP_blip_7_0 31
#define CHAR_MAP_blip_0_1 32
#define CHAR_MAP_blip_1_1 33
#define CHAR_MAP_blip_2_1 34
#define CHAR_MAP_blip_3_1 35
#define CHAR_MAP_blip_4_1 36
#define CHAR_MAP_blip_5_1 37
#define CHAR_MAP_blip_6_1 38
#define CHAR_MAP_blip_7_1 39
#define CHAR_MAP_blip_8_1 40
#define CHAR_MAP_blip_9_1 41
#define CHAR_MAP_blip_1_2 42
#define CHAR_MAP_blip_2_2 43
#define CHAR_MAP_blip_3_2 44
#define CHAR_MAP_blip_4_2 45
#define CHAR_MAP_blip_5_2 46
#define CHAR_MAP_blip_6_2 47
#define CHAR_MAP_blip_7_2 48

//EOF
