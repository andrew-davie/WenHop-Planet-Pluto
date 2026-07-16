#include "defines_dasm.h"

#include "cdfjplus.h"

#include "colour.h"
#include "cset.h"
#include "drawGridPreview.h"
#include "life.h"
#include "main.h"
#include "random.h"
#include "savekey.h"


#define PREVIEW_BOTTOM_OFFSET 0
#define PREVIEW_SLIDE_SPEED 5

static const unsigned char *img[_1ROW];


int previewStart;     // = _ARENA_SCANLINES;
int previewTarget;    // = PREVIEW_Y;


unsigned char wcol[100];
unsigned char neighbours[100];

void initGridPreview(int startLine, int endLine) {

    previewStart = startLine;
    previewTarget = endLine;

    for (int i = 0; i < 30; i++)
        setUnlockStatus(rangeRandom(100));


    // for (int i = 0; i < 100; i++)
    //     wcol[i] = 4;

    // for (int i = 0; i < 30; i++) {
    //     int base = rangeRandom(91);
    //     for (int j = 0; j < rangeRandom(10); j++)
    //         wcol[base + j] = rangeRandom(7) + 1;
    // }

    initLife();
}


void drawGridPreview() {

    const int base = _SCANLINES - PREVIEW_BOTTOM_OFFSET;

    for (int i = 0; i < PREVIEW_SLIDE_SPEED; i++)
        previewStart += (previewStart < previewTarget) - (previewStart > previewTarget);

#define PREVIEW_BORDER 6
#define PREVIEW_HEIGHT ((11 * 9) + 3)

    unsigned char *pL = RAM + _BUF_MENU_PF + _BUFFER_SIZE + previewStart;
    unsigned char *pM = pL + _BUFFER_SIZE;
    unsigned char *pR = pM + _BUFFER_SIZE;
    unsigned char *sp = RAM + _BUF_MENU_GRP + previewStart + 3 + PREVIEW_BORDER;
    unsigned char *col = RAM + _BUF_MENU_COLUP0 + previewStart + 3 + PREVIEW_BORDER - 1;

    // draw the whole "polaroid" (white border, black center)

    int scanline = previewStart;

    int len = PREVIEW_HEIGHT + PREVIEW_BORDER * 2 + 2;
    if (base - scanline < len)
        len = base - scanline;

    int roll;

    for (int i = 0; i < len; i++) {

        pL[i] |= 0x80;

        if (i < PREVIEW_BORDER || i > PREVIEW_HEIGHT + PREVIEW_BORDER) {

            pM[i] = 255;
            pR[i] |= 0x1F;

        } else {

            pM[i] = 0;
            pR[i] = (pR[i] & ~0x1F) | 0x10;
        }
    }


#if 0
    // the SOLVES flashing icon

    if (true) { /*status == STATUS_PREVIEW &&*/ /*getUnlockStatus(cave)*/ /*&& longPressReset <= 20*/
        //        RAM[_SK_TOTAL_SOLVES] > usedSolves) {

        static int flashing = 0;
        static int bleepDelay = 0;

        // if (bleepDelay)
        //     --bleepDelay;

        // else {

        //     bleepDelay = 22;

        //     if (!flashing)
        //         flashing = RAM[_SK_TOTAL_SOLVES] - usedSolves;
        //     else {

        //         if (!--flashing)
        //             bleepDelay = 50;
        //     }
        // }

        roll = roller;
        int colr = flashing ? ((bleepDelay & 16) ? 0 : 6) : 0;

        int base = PREVIEW_BORDER + 75;
        for (int j = 0; j < 7; j++) {
            if (base + j + previewStart < _SCANLINES - PREVIEW_BOTTOM_OFFSET) {
                pM[base + j] &= ~0x80;
                if ((colr & (1 << roll)))
                    pM[base + j] |= 0x80;
            }

            if (++roll > 2)
                roll = 0;
        }
    }

#endif

    unsigned char rgb[3];

    // draw the inner car "picture"

    scanline += PREVIEW_BORDER + 3;

    bool unlocked = getUnlockStatus(cave);
    bool solved = true;    // getSolveStatus(cave);
    bool perfect = getPerfectStatus(cave);


    // if (forceStatusDisplay)
    // 	--forceStatusDisplay;

    roll = roller;

    bool complete = false;    // isGameCompleted();

    if (false) {    // status == STATUS_PREVIEW) {

        //     if (solved)
        //         rgb[0] = rgb[1] = rgb[2] = 0xC4;    // SOLVED = greens

        //     else if (!unlocked)
        //         rgb[0] = rgb[1] = rgb[2] = 0x44;    // LOCKED = reds

        //     else

        //         for (int c = 0; c < 3; c++)
        //             rgb[c] = carColours[carColour][c];

        // for (int i = 0; i < 3; i++)
        //     rgb[i] = convertColour(rgb[i]);

        // // CARS DISPLAY

        // for (int y = 0; y < __BOARD_DEPTH; y++) {
        //     unsigned char *p = ADDRESS_OF(y) + 1;
        //     grab(p, 6);

        //     for (int chLine = 0; chLine < CHAR_HEIGHT; chLine += 3) {

        //         if (++scanline >= base)
        //             continue;

        //         int offset = chLine + roll;
        //         int gr = ((img[0][offset] >> 3 << 4) | (img[0][offset] << 5 >> 5)) << 12 |     //
        //                  (((img[1][offset] >> 3 << 4) | (img[1][offset] << 5 >> 5)) << 6) |    //
        //                  ((img[2][offset] >> 3 << 4) | (img[2][offset] << 5 >> 5));

        //         int gr2 = ((img[3][offset] >> 3 << 4) | (img[3][offset] << 5 >> 5)) << 18 |      //
        //                   (((img[4][offset] >> 3 << 4) | (img[4][offset] << 5 >> 5)) << 12) |    //
        //                   (((img[5][offset] >> 3 << 4) | (img[5][offset] << 5 >> 5)) << 6);

        //         // spit out as sprite data
        //         *(sp + 0 * _SCANLINES) = gr >> 16;
        //         *(sp + 1 * _SCANLINES) = gr >> 8;
        //         *(sp + 2 * _SCANLINES) = gr;

        //         *(sp + 3 * _SCANLINES) = gr2 >> 16;
        //         *(sp + 4 * _SCANLINES) = gr2 >> 8;
        //         *(sp + 5 * _SCANLINES) = gr2;

        //         sp++;
        //         *col++ = rgb[roll];

        //         if (++roll > 2)
        //             roll = 0;
        //     }
    }

    else {
        // #endif

        // STATUS_ALLVIEW

        // for (int i = 0; i < 3; i++)
        //     rgb[i] = convertColour(rgb[i]);


        // for (int i = 0; i < 9; i++)
        // 	*(sp + i) = 255;

        // sp += 3;
        // col += 3;
        // scanline += 3;

        int bs = 9;


        int lvl = 99;
        for (int y = 90; y >= -10; y -= 10) {

#ifdef RESTRICTED_DEMO

#define B 1
#define G 2
#define R 4

            rgb[0] = 0x84;
            rgb[1] = 0xC8;
            rgb[2] = 0x44;

#else

#define B 1
#define G 6
#define R 7

            // if (complete) {

            //     rgb[0] = 0x2A;
            //     rgb[1] = 0xC4;
            //     rgb[2] = 0xC4;
            // }

            // else {
            rgb[0] = convertColour(0xA8);
            rgb[1] = convertColour(0xD8);
            rgb[2] = convertColour(0x48);
            // }
#endif

            // #ifndef RESTRICTED_DEMO
            //             if (!getUnlockStatus(y))
            //                 rgb[0] = rgb[1] = rgb[2] = convertColour(0x42);
            // #endif

            static int pc = 0;
            pc++;

            for (int i = 9; i >= 0; i--) {

                if (lvl < 0)
                    img[i] = charset[gfx_cars_blip_gif_map[1][i]].data;

                else {
                    int s = R;

                    bool fls = frame & 48;

                    if (getUnlockStatus(lvl)) {

                        if (getPerfectStatus(lvl))
                            s = G;

                        else if (getSolveStatus(lvl)) {

                            if (complete)
                                s = 7;    // fls ? 7 : G;
                            else
                                s = fls ? G : B;
                        }

                        else
                            s = B;
                    }

                    s = wcol[lvl];

                    // int blink = 0;
                    if (y + i == cave && (frame & 8))
                        s ^= complete ? 1 : 7;

                    img[i] = charset[gfx_cars_blip_gif_map[0][s]].data;
                }
                lvl--;
            }


            unsigned char *p2 = charset[gfx_cars_blip_gif_map[1][bs]].data;
            unsigned char *z = charset[gfx_cars_blip_gif_map[1][0]].data;

            for (int chLine = 0; chLine < 31 - 6; chLine += 3) {
                if (++scanline >= base)
                    continue;

                int offset = chLine + roll;

                int gr = img[4][offset] | (img[3][offset] << 4) | (img[2][offset] << 8) | (img[1][offset] << 12) |
                         (img[0][offset] << 16);

                int gr2 = (img[9][offset] | (img[8][offset] << 4) | (img[7][offset] << 8) | (img[6][offset] << 12) |
                           (img[5][offset] << 16));

                gr2 |= gr << 20;
                gr >>= 4;

                // spit out as sprite data
                *(sp + 0 * _BUFFER_SIZE) = gr >> 16;
                *(sp + 1 * _BUFFER_SIZE) = gr >> 8;
                *(sp + 2 * _BUFFER_SIZE) = gr;

                *(sp + 3 * _BUFFER_SIZE) = gr2 >> 16;
                *(sp + 4 * _BUFFER_SIZE) = gr2 >> 8;
                *(sp + 5 * _BUFFER_SIZE) = gr2;


                if (lvl >= -10)
                    *sp |= (bs ? (p2[offset] << 3) : 0) | z[offset];

                // p += 3;

                sp++;
                *col++ = rgb[roll];
                if (++roll > 2)
                    roll = 0;
            }

            bs--;


            // }
        }
        //    }


        while (++scanline < _SCANLINES) {
            // spit out as sprite data
            *(sp + 0 * _BUFFER_SIZE) = *(sp + 1 * _BUFFER_SIZE) = *(sp + 2 * _BUFFER_SIZE) =

                *(sp + 3 * _BUFFER_SIZE) = *(sp + 4 * _BUFFER_SIZE) = *(sp + 5 * _BUFFER_SIZE) = 0;
            sp++;
        }

        // Badges

#define BADGE_POSITION 38

        // if (flashLocked)
        //     flashLocked--;

        // if (flashLocked & 12)
        //     draw6Bitmap(gfx_grid_padlock_gif, gfx_grid_padlock_gif_HEIGHT, scanline - 82 + BADGE_POSITION,
        //                 convertColour(0x14));

        // if (status == STATUS_PREVIEW) {
        //     if (solved)
        //         draw6Bitmap(gfx_grid_tick_gif, gfx_grid_tick_gif_HEIGHT, scanline - 82 + BADGE_POSITION,
        //                     convertColour(0x14));
        //     if (perfect)
        //         draw6Bitmap(gfx_grid_perfect_gif, gfx_grid_perfect_gif_HEIGHT, scanline - 82 + BADGE_POSITION +
        //         12,
        //                     convertColour(0x14));
        // }
    }
}

// EOF
