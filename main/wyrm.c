#include "defines_dasm.h"

#include "cdfjplus.h"

#include "main.h"
#include "wyrm.h"

#include "attribute.h"
#include "particle.h"
#include "random.h"


const unsigned char wyrmChar[] = {

    0,                    // 0
    CH_WYRM_VERT_BODY,    // 1   U
    CH_WYRM_BODY,         // 2   R
    CH_WYRM_CORNER_RU,    // 3   RU
    CH_WYRM_VERT_BODY,    // 4   D
    CH_WYRM_VERT_BODY,    // 5   UD
    CH_WYRM_CORNER_RD,    // 6   RD
    0,                    // 7
    CH_WYRM_BODY,         // 8   L
    CH_WYRM_CORNER_LU,    // 9   LU
    CH_WYRM_BODY,         // 10  LR
    0,                    // 11
    CH_WYRM_CORNER_LD,    // 12  LD
    0,                    // 13
    0,                    // 14
    0,                    // 15
};

static struct wyrmDetails wyrms[WYRM_POP];

void initWyrms() {
    for (int i = 0; i < WYRM_POP; i++)
        wyrms[i].head = -1;
}

bool newWyrm(int x, int y) {

    for (int i = 0; i < WYRM_POP; i++) {
        if (wyrms[i].head < 0) {
            struct wyrmDetails *thisWyrm = &wyrms[i];

            thisWyrm->head = 0;
            thisWyrm->dir = getRandom32() & 3;
            thisWyrm->x[0] = x;
            thisWyrm->y[0] = y;
            thisWyrm->length = 2;
            thisWyrm->speed = thisWyrm->pace = 2;    // rangeRandom(5) + 3;

            return true;
        }
    }
    return false;
}

void processWyrms() {


    for (int i = 0; i < WYRM_POP; i++) {

        struct wyrmDetails *wyrm = &wyrms[i];

        if (--wyrm->speed)
            continue;


        wyrm->speed = wyrm->pace;

        int headPos = wyrm->head;
        if (headPos < 0)
            continue;

        bool belowSurface = (wyrm->y[0] * CHAR_TRIX_Y > lavaSurfaceTrixel);

        int x = wyrm->x[headPos];
        int y = wyrm->y[headPos];

        int candidateX = x + xdir[wyrm->dir];
        int candidateY = y + ydir[wyrm->dir];

        unsigned char *newHead = RAM + _BOARD + candidateY * _BOARD_COLS + candidateX;

        int mask = ATT_BLANK | ATT_GRAB;
        enum ObjectType whatsThere = CharToType[GET(*newHead)];
        bool moveable = Attribute[whatsThere] & mask;

        if (!moveable) {

            wyrm->dir = (wyrm->dir + 1) & 3;

            candidateX = x + xdir[wyrm->dir];
            candidateY = y + ydir[wyrm->dir];

            newHead = RAM + _BOARD + candidateY * _BOARD_COLS + candidateX;
            whatsThere = CharToType[GET(*newHead)];
            if (Attribute[whatsThere] & mask) {
                moveable = true;
            }
        }

        bool candidateBelowSurface = (candidateY * CHAR_TRIX_Y > lavaSurfaceTrixel);
        if (!candidateBelowSurface && candidateBelowSurface != belowSurface)
            moveable = false;

        if (!moveable) {

            if (wyrm->head > 2 /* && (gameFrame & 1)*/) {

                unsigned char *segment = RAM + _BOARD + wyrm->y[0] * _BOARD_COLS + wyrm->x[0];
                *segment = CH_DUST_ROCK_0;

                for (int w = 0; w < WYRM_MAX - 1; w++) {
                    wyrm->x[w] = wyrm->x[w + 1];
                    wyrm->y[w] = wyrm->y[w + 1];
                }
                wyrm->head--;
                wyrm->length--;
            }

            // if (!rangeRandom(5)) {
            //  reverse wyrm

            unsigned char tempX2[WYRM_MAX], tempY2[WYRM_MAX];
            for (int w = 0; w <= wyrm->head; w++) {
                tempX2[w] = wyrm->x[w];
                tempY2[w] = wyrm->y[w];
            }

            for (int w = 0; w <= wyrm->head; w++) {
                wyrm->x[w] = tempX2[wyrm->head - w];
                wyrm->y[w] = tempY2[wyrm->head - w];
            }

            wyrm->dir = (wyrm->dir + 2) & 3;
            //}
        }

        else {

            unsigned char *segment;
            if (headPos > 0) {
                int dir = dirFromCoords(candidateX, candidateY, wyrm->x[headPos], wyrm->y[headPos]) |

                          dirFromCoords(wyrm->x[headPos - 1], wyrm->y[headPos - 1], wyrm->x[headPos], wyrm->y[headPos]);

                segment = RAM + _BOARD + wyrm->y[headPos] * _BOARD_COLS + wyrm->x[headPos];
                *segment = wyrmChar[dir];
            }

            if (headPos >= wyrm->length) {

                if (true /*tmp*/ || !(Attribute[whatsThere] & (ATT_WATERFLOW | ATT_GRAB)) ||
                    wyrm->length > WYRM_MAX - 2) {
                    unsigned char *tailPos = RAM + _BOARD + wyrm->y[0] * _BOARD_COLS + wyrm->x[0];

                    *tailPos = CH_BLANK;    // CH_DUST_ROCK_0;

                    for (int i = 0; i < WYRM_MAX - 1; i++) {
                        wyrm->x[i] = wyrm->x[i + 1];
                        wyrm->y[i] = wyrm->y[i + 1];
                    }

                    headPos--;
                }

                else {
                    wyrm->length++;
                }

                if (wyrm->y[0] * CHAR_TRIX_Y > lavaSurfaceTrixel) {
                    if (!rangeRandom(500)) {
                        newWyrm(wyrm->x[0], wyrm->y[0]);
                    }
                }
            }

            wyrm->x[++headPos] = candidateX;
            wyrm->y[headPos] = candidateY;

            wyrm->head = headPos;

            segment = RAM + _BOARD + candidateY * _BOARD_COLS + candidateX;

            if (TYPEOF(*segment) == TYPE_DOGE)
                nDots(4, candidateX, candidateY, PT_TWO, 50, 3, 0, 100, 7);

            *segment = wyrm->dir + CH_WYRM_HEAD_U;
        }

        if (headPos) {

            unsigned char *tailPos = RAM + _BOARD + wyrm->y[0] * _BOARD_COLS + wyrm->x[0];

            unsigned char tail = CH_WYRM_TAIL_R;
            if (wyrm->y[1] > wyrm->y[0])
                tail = CH_WYRM_TAIL_U;

            else if (wyrm->y[1] < wyrm->y[0])
                tail = CH_WYRM_TAIL_D;

            else if (wyrm->x[1] > wyrm->x[0])
                tail = CH_WYRM_TAIL_L;
            // else
            //     tail = CH_WYRM_TAIL_R;

            *tailPos = tail;

            unsigned char headChar = CH_WYRM_HEAD_U;
            if (wyrm->x[headPos] < wyrm->x[headPos - 1])
                headChar = CH_WYRM_HEAD_L;
            else if (wyrm->x[headPos] > wyrm->x[headPos - 1])
                headChar = CH_WYRM_HEAD_R;
            else if (wyrm->y[headPos] > wyrm->y[headPos - 1])
                headChar = CH_WYRM_HEAD_D;

            unsigned char *head = RAM + _BOARD + wyrm->y[headPos] * _BOARD_COLS + wyrm->x[headPos];
            *head = headChar;
        }

        nDots(2, wyrm->x[0], wyrm->y[0], PT_SPIRAL, 30, 2, 5, 100, 7);
    }
}
