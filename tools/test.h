#pragma once

#define CARS_CHAR_WIDTH 5
#define CARS_CHAR_HEIGHT 10

typedef struct {
    unsigned char data[CARS_CHAR_HEIGHT * 3]; // RGB
} character;

extern const character test[0];

//EOF
