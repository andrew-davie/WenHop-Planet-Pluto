#pragma once

#define WYRM_POP 16
#define WYRM_MAX 16


struct wyrmDetails {
    signed char x[WYRM_MAX];
    signed char y[WYRM_MAX];
    signed char head;
    unsigned char dir;
    unsigned char length;
};

extern struct wyrmDetails wyrms[WYRM_POP];

void initWyrms();
bool newWyrm(int x, int y);
void processWyrms();

// EOF
