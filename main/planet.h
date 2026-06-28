#pragma once

struct GLOBE {
    const char *name;
    const unsigned char colour;
    const char *physics;
    signed char retrograde;
    const unsigned char *map;
    const unsigned char *const *charSet;
    const unsigned char *palette;
};

#define MAX_PLANET 11
extern const struct GLOBE planets[MAX_PLANET];

// EOF
