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

    // Same 3 regions as before (top border / black centre / bottom border),
    // just as 3 straight-line loops instead of 1 loop re-testing the same
    // branch on every single iteration -- the region boundaries don't
    // depend on i, so there's nothing to gain from checking them per-pixel.
    {
        int borderEnd = PREVIEW_BORDER < len ? PREVIEW_BORDER : len;
        int innerEnd = PREVIEW_HEIGHT + PREVIEW_BORDER + 1;
        if (innerEnd > len)
            innerEnd = len;

        int i = 0;
        for (; i < borderEnd; i++) {
            pL[i] |= 0x80;
            pM[i] = 255;
            pR[i] |= 0x1F;
        }
        for (; i < innerEnd; i++) {
            pL[i] |= 0x80;
            pM[i] = 0;
            pR[i] = (pR[i] & ~0x1F) | 0x10;
        }
        for (; i < len; i++) {
            pL[i] |= 0x80;
            pM[i] = 255;
            pR[i] |= 0x1F;
        }
    }


    unsigned char rgb[3];

    // draw the inner  "picture"

    scanline += PREVIEW_BORDER + 3;

    // If the picture is already entirely below the visible area (e.g. still
    // mid slide-in -- initGridPreview() starts previewStart at _SCANLINES+50
    // and slides it down 5/frame towards PREVIEW_Y, so this is true for
    // roughly the first dozen frames every time the menu opens), every one
    // of the 90 chLine iterations below would just hit `if (++scanline >=
    // base) continue;` without ever touching sp/col -- scanline is the only
    // thing that actually changes across all of them (roll ends up back at
    // its starting value regardless, since it free-runs mod 3 and 90 is a
    // multiple of 3; bs and lvl are never read after the loop). So skip
    // straight to the equivalent scanline value instead of doing all the
    // (thrown-away) image lookups, bit-packing and colour work to get there.
    // This does NOT change anything about the partially-visible case --
    // that still falls through to the exact original loop below, unchanged.
    if (scanline >= base) {

        scanline += 90;    // 10 y-iterations x 9 chLine-iterations, unconditionally

    } else {

        roll = roller;

        bool complete = false;    // isGameCompleted();

        int bs = 9;


        int lvl = 99;
        for (int y = 90; y >= -10; y -= 10) {

#define B 1
#define G 6
#define R 7

            rgb[0] = convertColour(0xA8);
            rgb[1] = convertColour(0xD8);
            rgb[2] = convertColour(0x48);


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

                    s = wcol[lvl];    // <<--- life!

                    img[i] = charset[gfx_cars_blip_gif_map[2][s]].data;
                }
                lvl--;
            }

            // bs is clamped (not just decremented) below -- once it saturates at 0, every
            // remaining y-iteration still renders (scanline/sp/col must advance every
            // iteration, real on-screen rows depend on it), it just stops being a valid
            // highlight index. See the ternary below: bs == 0 is falsy already gives
            // "no highlight", so saturating at 0 rather than letting it go negative
            // preserves that exact behaviour for the extra (11th) iteration instead of
            // reading gfx_cars_blip_gif_map[1][-1] (the UB this whole thing was chasing).
            const unsigned char *p2 = charset[gfx_cars_blip_gif_map[1][bs]].data;
            const unsigned char *z = charset[gfx_cars_blip_gif_map[1][0]].data;

            // lvl is fully decremented by the i-loop above before this loop ever
            // runs, so it's constant for all 9 iterations below -- hoisted out
            // of the per-iteration check rather than re-testing the same value
            // 9 times.
            bool lvlValid = lvl >= -10;

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


                if (lvlValid)
                    *sp |= (bs ? (p2[offset] << 3) : 0) | z[offset];

                sp++;
                *col++ = rgb[roll];
                if (++roll > 2)
                    roll = 0;
            }

            if (bs > 0)
                bs--;
        }
    }


    if (scanline < _SCANLINES)
        for (int i = 0; i < 6; i++)
            myMemset(sp + i * _BUFFER_SIZE, 0, _SCANLINES - scanline);


    // while (++scanline < _SCANLINES) {
    //     // spit out as sprite data
    //     *(sp + 0 * _BUFFER_SIZE) = //
    //     *(sp + 1 * _BUFFER_SIZE) = //
    //     *(sp + 2 * _BUFFER_SIZE) = //
    //     *(sp + 3 * _BUFFER_SIZE) = //
    //     *(sp + 4 * _BUFFER_SIZE) = //
    //     *(sp + 5 * _BUFFER_SIZE) = 0;
    //     sp++;
    // }
}

// EOF
