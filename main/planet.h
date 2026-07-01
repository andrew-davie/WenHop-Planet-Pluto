#pragma once

struct GLOBE {
    const char *name;
    const char *physics;
    signed char retrograde;
    const unsigned char *map;
    const unsigned char *const *charSet;
    const unsigned char *palette;
};

extern const struct GLOBE planets[];

// EOF
