#include "life.h"

#include "random.h"

// 10x10 world, wrapping left/right only (no wrap top/bottom).
// A cell is 0 (dead) or 1-7 (alive, holding one of the 8 available colours).
extern unsigned char wcol[LIFE_CELLS];

static unsigned char nextwcol[LIFE_CELLS];

// Left to its own devices Life on a small grid usually dies out or freezes.
// Every RESEED_INTERVAL generations, drop in an R-pentomino: a 5-cell
// "Methuselah" pattern that famously churns for ~1100 generations before
// settling, which keeps the board livelier than a handful of random cells.
#define RESEED_INTERVAL 10

static const int R_PENTOMINO[5][2] = {
    {0, 1}, {0, 2}, {1, 0}, {1, 1}, {2, 1},
};

static int wrapX(int x) {

    if (x < 0)
        return x + LIFE_SIZE;
    if (x >= LIFE_SIZE)
        return x - LIFE_SIZE;
    return x;
}

static void sprinkleRPentomino() {

    int originX = rangeRandom(LIFE_SIZE);
    int originY = rangeRandom(LIFE_SIZE - 2);    // keep all 3 rows on the board (no vertical wrap)
    unsigned char colour = 1 + rangeRandom(7);

    for (int i = 0; i < 5; i++) {
        int x = wrapX(originX + R_PENTOMINO[i][1]);
        int y = originY + R_PENTOMINO[i][0];
        wcol[y * LIFE_SIZE + x] = colour;
    }
}

// A newborn cell takes the colour shared by most of the 3 live neighbours
// that caused its birth (ties favour the lowest colour index).
static unsigned char birthColour(unsigned char votes[8]) {

    unsigned char best = 1;
    for (unsigned char colour = 2; colour < 8; colour++)
        if (votes[colour] > votes[best])
            best = colour;
    return best;
}


void initLife() {

    for (int i = 0; i < LIFE_CELLS; i++)
        wcol[i] = rangeRandom(3) ? 0 : 1 + rangeRandom(7);    // ~1/3 alive, random colour
}


// Same neighbour-counting rules as before, restructured for the hot path:
//  - wrapX(x+dx) only ever actually wraps at the two edge columns, but was
//    being called (with its two branches) for every one of up to 8
//    neighbours of every cell. x's two horizontal neighbour columns only
//    depend on x, not on dy, so they're resolved once per cell instead.
//  - ny*LIFE_SIZE was being recomputed for each of the 3 dx checks within a
//    given dy, even though it's the same value all 3 times -- hoisted per
//    dy instead (same for y*LIFE_SIZE, constant for the whole row).
//  - votes[8] only matters for a *dead* cell's birth-colour vote (an alive
//    cell only needs the neighbour count for the 2/3-neighbour survival
//    check) -- alive cells now skip the votes array (zeroing it and the
//    per-neighbour increments) entirely rather than maintaining it unread.
static void computeRow(int y) {

    int yBase = y * LIFE_SIZE;

    for (int x = 0; x < LIFE_SIZE; x++) {

        int xm1 = x ? x - 1 : LIFE_SIZE - 1;
        int xp1 = (x == LIFE_SIZE - 1) ? 0 : x + 1;

        unsigned char cell = wcol[yBase + x];
        int aliveCount = 0;

        if (cell) {

            for (int dy = -1; dy <= 1; dy++) {

                int ny = y + dy;
                if (ny < 0 || ny >= LIFE_SIZE)
                    continue;    // no wrap top/bottom

                int rowBase = ny * LIFE_SIZE;

                if (wcol[rowBase + xm1])
                    aliveCount++;
                if (dy != 0 && wcol[rowBase + x])
                    aliveCount++;
                if (wcol[rowBase + xp1])
                    aliveCount++;
            }

            nextwcol[yBase + x] = (aliveCount == 2 || aliveCount == 3) ? cell : 0;

        } else {

            unsigned char votes[8] = {0};

            for (int dy = -1; dy <= 1; dy++) {

                int ny = y + dy;
                if (ny < 0 || ny >= LIFE_SIZE)
                    continue;    // no wrap top/bottom

                int rowBase = ny * LIFE_SIZE;
                unsigned char neighbour;

                neighbour = wcol[rowBase + xm1];
                if (neighbour) {
                    aliveCount++;
                    votes[neighbour]++;
                }

                if (dy != 0) {
                    neighbour = wcol[rowBase + x];
                    if (neighbour) {
                        aliveCount++;
                        votes[neighbour]++;
                    }
                }

                neighbour = wcol[rowBase + xp1];
                if (neighbour) {
                    aliveCount++;
                    votes[neighbour]++;
                }
            }

            nextwcol[yBase + x] = (aliveCount == 3) ? birthColour(votes) : 0;
        }
    }
}

// Computes up to 'rows' rows of the next generation, resuming from wherever
// the previous call left off. All rows read the untouched wcol (the current
// generation) and write into nextwcol, so a generation can be safely spread
// across many calls. Once the last row is done, nextwcol is committed back
// into wcol and (every RESEED_INTERVAL generations) a fresh R-pentomino is
// dropped in.
void life(int rows) {

    static int nextRow = 0;

    if (rows < 1)
        rows = 1;

    int rowsRemaining = LIFE_SIZE - nextRow;
    if (rows > rowsRemaining)
        rows = rowsRemaining;

    for (int r = 0; r < rows; r++)
        computeRow(nextRow + r);

    nextRow += rows;

    if (nextRow >= LIFE_SIZE) {

        nextRow = 0;

        for (int i = 0; i < LIFE_CELLS; i++)
            wcol[i] = nextwcol[i];

        static int generation = 0;
        if (++generation >= RESEED_INTERVAL) {
            generation = 0;
            sprinkleRPentomino();
        }
    }
}

// EOF
