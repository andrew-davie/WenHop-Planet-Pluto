#pragma once

#define CARS_CHAR_WIDTH 5
#define CARS_CHAR_HEIGHT 11

typedef struct {
    unsigned char data[CARS_CHAR_HEIGHT * 3]; // RGB
} character;

extern const character charset[42];

extern const unsigned char gfx_cars_blank_gif_map[1][1];
extern const unsigned char gfx_cars_numbers_gif_map[16][2];
extern const unsigned char gfx_cars_blip_gif_map[2][10];

//EOF
