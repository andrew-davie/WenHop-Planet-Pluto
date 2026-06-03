#pragma once

#define WYRM_POP 8
#define WYRM_MAX 8


struct wyrmDetails {
    signed char x[WYRM_MAX];
    signed char y[WYRM_MAX];
    signed char head;
    unsigned char dir;
    unsigned char length;
    unsigned char speed;
    unsigned char pace;
};

extern struct wyrmDetails wyrms[WYRM_POP];

void initWyrms();
bool newWyrm(int x, int y);
void processWyrms();

// EOF
