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

static const unsigned char *img[_BOARD_COLS];


static int previewStart;     // = _ARENA_SCANLINES;
static int previewTarget;    // = PREVIEW_Y;


unsigned char wcol[100];

void initGridPreview(int startLine, int endLine) {

    previewStart = startLine;
    previewTarget = endLine;

    for (int i = 0; i < 30; i++)
        setUnlockStatus(rangeRandom(100));

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


    unsigned char rgb[3];

    // draw the inner car "picture"

    scanline += PREVIEW_BORDER + 3;

    roll = roller;

    bool complete = false;    // isGameCompleted();

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

            rgb[0] = convertColour(0xA8);
            rgb[1] = convertColour(0xD8);
            rgb[2] = convertColour(0x48);
#endif

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

                    img[i] = charset[gfx_cars_blip_gif_map[2][s]].data;
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

                sp++;
                *col++ = rgb[roll];
                if (++roll > 2)
                    roll = 0;
            }

            bs--;
        }


        while (++scanline < _SCANLINES) {
            // spit out as sprite data
            *(sp + 0 * _BUFFER_SIZE) = *(sp + 1 * _BUFFER_SIZE) = *(sp + 2 * _BUFFER_SIZE) =

                *(sp + 3 * _BUFFER_SIZE) = *(sp + 4 * _BUFFER_SIZE) = *(sp + 5 * _BUFFER_SIZE) = 0;
            sp++;
        }
}

// EOF
