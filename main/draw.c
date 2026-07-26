#include <stdbool.h>

#include "defines_dasm.h"

#include "animations.h"
#include "attribute.h"
#include "bitPatterns.h"
#include "cdfjplus.h"    // <- contains references from defines_dasm.h
#include "characterset.h"
#include "colour.h"
#include "draw.h"
#include "main.h"
#include "mellon.h"
#include "playerAnimation.h"
#include "reverseBits.h"
#include "scroll.h"
#include "sound.h"

#include "../gfx/alphanumeric.h"
#include "../gfx/fontcompact.h"
#include "../gfx/fontlarge.h"


static int dramaticPause;


void draw6Bitmap(unsigned int grpOffset, unsigned int colup0Offset,    //
                 const unsigned char bitmap6[][6],                     //
                 int height, int y, int colour) {

    // Draw a 6-sprite wide bitmap

    unsigned char *p = RAM + grpOffset + y;
    unsigned char *q = RAM + colup0Offset + y - 1;
    const unsigned char *bm = &bitmap6[0][0];

    colour = convertColour(colour);

    if (y < 0) {
        bm -= y * 6;
        height += y;
        p -= y;
        q -= y;
    }

    if (y + height >= _SCANLINES)
        height = _SCANLINES - y;


    for (int line = 0; line < height; line++) {
        q[line] = colour;
        for (int c = 0; c < 6; c++)
            p[c * _BUFFER_SIZE + line] = *bm++;
    }
}


static const char *ps = 0;
static unsigned char *buf;
static unsigned char *colrx;
static int cx;
static int cy;
static int font;
static int stringColour;
static int speedDelay;
static int current;
static int underline;
static int escape;

enum justify {
    JUSTIFY_NONE,
    JUSTIFY_LEFT,
    JUSTIFY_RIGHT,
    JUSTIFY_CENTER,
};


static enum justify justify;

const struct FONT {
    enum fontsize size;
    int lineHeight;
    const unsigned char *const *asciiTable;
    const unsigned char *charWidths;
} fonts[] = {
    {FONT_STANDARD, ALPHANUMERIC_FONT_HEIGHT, alphanumeric_asciiTable, alphanumeric_charWidths},
    {FONT_COMPACT, FONTCOMPACT_FONT_HEIGHT, fontcompact_asciiTable, fontcompact_charWidths},
    {FONT_LARGE, FONTLARGE_FONT_HEIGHT, fontlarge_asciiTable, fontlarge_charWidths},
};


void drawString(int fontNumber, int c, int delay, int buffer, int colbuf, const char *string, int y) {

    // x: 0..47 pixel pos in GRP array
    // y: 1.._SCANLINES-1 line  (0 doesn't work because of colour)
    // string zero-terminated

    font = fontNumber;
    stringColour = c;
    underline = false;
    escape = false;

    dramaticPause = 0;

    ps = string;
    buf = RAM + buffer;          // + y;
    colrx = RAM + colbuf - 1;    // + y - 1;

    speedDelay = delay;
    current = 0;

    justify = JUSTIFY_NONE;

    cx = 0;
    cy = y;
}

// clang-format off

#define J_ON        0b10000000  /* visible */
#define J_PARAGRAPH 0b00001000  /* new paragraph if followed by EOL */
#define J_UNDER     0b00000100  /* underline */
#define J_LEFT      0b00000010  /* left-justify */
#define J_RIGHT     0b00000001  /* right-justify */

const unsigned char justifyChar['~' - ' ' + 1] = {

//   🟨 = visible
//  ⎧       🟨 = new paragraph if followed by | 
//  ⎮      ⎧  🟨 = underline
//  ⎮      ⎮ ⎧ 🟨 = left-justify
//  ⎮      ⎮ ⎮⎧  🟨 = right-justify
//  ⎮      ⎮ ⎮⎮ ⎮
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   ' '         32
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '!'         33
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '"'         34
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '#'         35
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '$'         36       replacement '.' without a delay (for numbers0
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '%'         37
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '&'         38
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   '''         39 
    🟨🟦🟦🟦🟦🟦🟦🟨 ,    //   '('         40
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   ')'         41
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '*'         42
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '+'         43
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   ','         44       + short pause
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '-'         45
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '.'         46
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '/'         47
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '0'         48
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '1'         49
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '2'         50
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '3'         51
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '4'         52
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '5'         53
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '6'         54
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '7'         55
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '8'         56
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '9'         57
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ':'         58
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ';'         59
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '<'         60       left justify string
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '='         61       center justify string
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '>'         62       right justify string
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '?'         63
    🟨🟦🟦🟦🟦🟨🟨🟦 ,    //   '@'         64
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'A'         65
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'B'         66
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'C'         67
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'D'         68
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'E'         69
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'F'         70
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'G'         71
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'H'         72
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'I'         73
    🟨🟦🟦🟦🟦🟨🟨🟦 ,    //   'J'         74
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'K'         75
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'L'         76
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'M'         77
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'N'         78
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'O'         79
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'P'         80
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Q'         81
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'R'         82
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'S'         83
    🟨🟦🟦🟦🟦🟨🟨🟨 ,    //   'T'         84
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'U'         85
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'V'         86
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'W'         87
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'X'         88
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Y'         89
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Z'         90
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '['         91
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   'backslash  92
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ']'         93
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '^'         94
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '_'         95      toggle underline
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '`'         96
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'a'         97
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'b'         98
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'c'         99
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'd'         100
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'e'         101
    🟨🟦🟦🟦🟦🟨🟨🟨 ,    //   'f'         102
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'g'         103
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'h'         104
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'i'         105
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'j'         106
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'k'         107
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'l'         108
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'm'         109
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'n'         110
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'o'         111
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'p'         112
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'q'         113
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'r'         114
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   's'         115
    🟨🟦🟦🟦🟦🟨🟨🟨 ,    //   't'         116
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'u'         117
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'v'         118
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'w'         119
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'x'         120
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'y'         121
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'z'         122
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '{'         123
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '|'         124      new line
    🟦🟦🟦🟦🟨🟦🟦🟦 ,    //   '}'         125      new paragraph
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '~'         126      pause
};

// clang-format on


#define LTR(s) (s - ' ')
#define EOL '|'
#define EOP '}'

enum {
    CTL_EOL = '|',
    CTL_PAR = '}',
    CTL_LEFT = '<',
    CTL_RIGHT = '>',
    CTL_CENTER = '=',
    CTL_UNDERLINE = '_',
    CTL_PAUSE = '~',
};


int getLineWidth(const char *str) {

    bool esc = false;

    int width = 0;
    while (*str && *str != EOL && *str != EOP) {

        int ch = *str;
        int ci = LTR(ch);

        str++;

        if (ch == '\\') {
            esc = true;
            continue;
        }

        if (esc || (justifyChar[ci] & J_ON)) {

            if (ch != '#' && ch != '\"')
                width += fonts[font].charWidths[ci] + 1;

            if (justifyChar[ci] & J_LEFT)
                width--;
            if (justifyChar[ci] & J_RIGHT)
                width--;

            esc = false;
        }
    }

    return width ? width - 1 : 0;
}


void setJustifyX(const char *str) {

    switch (justify) {

    case JUSTIFY_CENTER:
        cx = (48 - getLineWidth(str)) >> 1;
        break;

    case JUSTIFY_RIGHT:
        cx = 48 - getLineWidth(str);
        break;

    default:
        cx = 0;
        break;
    }
}


void doLetter(int ci, int chx, int chy, char colour) {

    int column = chx >> 3;
    int bit = 7 - (chx & 7);

    const unsigned char *charShape = fonts[font].asciiTable[ci];
    if (charShape) {

        for (int i = 0; i < fonts[font].lineHeight; i++) {

            int vertPos = chy + i;
            if (vertPos >= 0 && vertPos < _SCANLINES) {

                int shape = charShape[i] << (8 - fonts[font].charWidths[ci]);

                int bs = bit + 1;
                int band = column;

                if (shape)
                    colrx[vertPos] = colour;

                while (shape && (band < 6)) {

                    buf[vertPos + band * _BUFFER_SIZE] |= (shape << bs) >> 8;

                    shape = (shape << bs) & 0xFF;
                    bs = 8;
                    band++;
                }
            }
        }
    }
}


bool drawNextChar() {

    if (dramaticPause)
        dramaticPause--;

    while (!dramaticPause && ps && *ps && ++current > speedDelay) {

        int ch = *ps;
        int ci = LTR(ch);

        ps++;


        if (!escape) {

            switch (ch) {

            case '\\':
                escape = true;
                current = 0;
                continue;

            case CTL_PAR:

                cy += fonts[font].lineHeight >> 1;
                dramaticPause = 20;
                __attribute__((fallthrough));

            case CTL_EOL: {

                setJustifyX(ps);
                cy += fonts[font].lineHeight;
                continue;
            }

            case CTL_RIGHT:
                justify = JUSTIFY_RIGHT;
                setJustifyX(ps);
                continue;

            case CTL_LEFT:
                justify = JUSTIFY_LEFT;
                setJustifyX(ps);
                continue;

            case CTL_CENTER:
                justify = JUSTIFY_CENTER;
                setJustifyX(ps);
                continue;

            case CTL_UNDERLINE:
                underline = !underline;
                continue;

            case CTL_PAUSE:
                dramaticPause = 50;
                continue;
            }
        }

        if ((escape || (justifyChar[ci] & J_ON)) && fonts[font].charWidths[ci]) {

            current = 0;

            if (ch == '.' && *ps != '#' && *ps != '.')    // pause sentence except at end
                dramaticPause = 50;

            else if (ch == ',')
                dramaticPause = 10;

            if (ch != ' ' && ch != '+')                           // review empty star, space
                ADDAUDIO(ch == '*' ? SFX_DOGE : SFX_KEYPRESS);    // review-star = "*"

            if (ch == '\"')
                cx -= fonts[font].charWidths[LTR('\"')];

            if (justifyChar[ci] & J_LEFT)
                cx--;

            doLetter(ci, cx, cy, convertColour(stringColour));
            if ((justifyChar[ci] & J_UNDER) && underline) {

                int uwide = fonts[font].charWidths[ci] - fonts[font].charWidths[LTR('_')] + 1;
                for (int i = 0; i <= uwide; i++)
                    doLetter(LTR('_'), cx + i, cy + 1, convertColour(0x44));
            }

            cx += fonts[font].charWidths[ci] + 1;

            if (justifyChar[ci] & J_RIGHT)
                cx--;

            escape = false;
        }
    }

    return ps && *ps;
}


void blitShape(int ch, int shift, int y, int height, int offset) {

    // Single-pass 64-bit version -- same technique as drawScreen()/drawScreenMirror():
    // one wide fetch+shift instead of two glued 20px halves.
    //
    // The old version drew the left half (PF0/PF1/PF2 @ offset) and the right half
    // (PF0/PF1/PF2 @ offset+3*_BUFFER_SIZE, "_BUF_GAME_PF0_RIGHT") with two separate
    // 32-bit calls, each shifting the 8-bit-wide glyph column into its own 20-bit-wide
    // half (shift range -7..19 relative to that half). Because the two halves sit at a
    // fixed +20px / +3*_BUFFER_SIZE relationship to each other, both computations are
    // really the *same* "glyph << bitsh" value read through two different 24-bit windows:
    //
    //   bitsh_left  = 19 - shift          (old left-call formula)
    //   bitsh_right = 19 - (shift - 20)   (old right-call formula, shift-20 being the
    //                                      old caller's "shift" arg for the right half)
    //              = bitsh_left + 20
    //
    // So computing a single wider shift bitsh = 39 - shift (using the real, unsplit
    // screen column) and reading the LEFT window 20 bits higher than the RIGHT window
    // reproduces both old calls exactly from one 64-bit value -- verified by exhaustive
    // brute-force comparison against the old two-call implementation (bit-for-bit
    // identical over millions of random shift/y/roller/glyph/buffer combinations,
    // including the 32-bit truncation clipping the old code exhibited at extreme
    // negative shift, which naturally falls out of window position rather than needing
    // to be special-cased).
    //
    //   window (bit offset, 8 bits each, PF0/PF2 reverseBits'd same as before):
    //     PF0_LEFT  @ 43   PF1_LEFT  @ 35   PF2_LEFT  @ 27
    //     PF0_RIGHT @ 23   PF1_RIGHT @ 15   PF2_RIGHT @  7
    //
    // blitShape() already clipped on y internally (the "y < _SCANLINES" loop guard and
    // "if (y >= 0)" below, both present before this change). x-axis clipping used to be
    // the caller's job instead: it chose whether to call the left half, the right half,
    // both, or neither, via the "-8 < shift < 20" / "12 < shift < 40" checks before each
    // call. Now that one call handles both halves, those same two conditions have to live
    // in here too -- writeLeft/writeRight below are exactly those old caller-side checks,
    // just moved in so a single pass can decide per-half whether to write.

    bool writeLeft = shift > -8 && shift < 20;
    bool writeRight = shift > 12 && shift < 40;

    if (!writeLeft && !writeRight)
        return;

    unsigned char *p1 = RAM + offset + y;
    unsigned char *p2 = p1 + _BUFFER_SIZE;
    unsigned char *p3 = p2 + _BUFFER_SIZE;
    unsigned char *p4 = p1 + 3 * _BUFFER_SIZE;
    unsigned char *p5 = p4 + _BUFFER_SIZE;
    unsigned char *p6 = p5 + _BUFFER_SIZE;

    int modifier = y + 1;

    while (modifier > 2)
        modifier -= 3;

    while (modifier < 0)
        modifier += 3;

    int bitsh = 39 - shift;

    const char nextRoller[] = {1, 2, 0, 1, 2, 0, 1, 2, 0};

    for (int trix = 0; trix < height && y < _SCANLINES; trix += 3) {

        if (y >= 0) {

            const unsigned char *fp = charSet[ch];

            unsigned long long mask = fp[trix] | fp[trix + 1] | fp[trix + 2];
            mask <<= bitsh;
            mask = ~mask;

            unsigned char mPF0L = reverseBits[(unsigned char)(mask >> 43)];
            unsigned char mPF1L = (unsigned char)(mask >> 35);
            unsigned char mPF2L = reverseBits[(unsigned char)(mask >> 27)];
            unsigned char mPF0R = reverseBits[(unsigned char)(mask >> 23)];
            unsigned char mPF1R = (unsigned char)(mask >> 15);
            unsigned char mPF2R = reverseBits[(unsigned char)(mask >> 7)];

#define INNER(r)                                                                                                       \
    {                                                                                                                  \
        unsigned long long shiftCh = fp[trix + nextRoller[roller + r + modifier]];                                    \
        shiftCh <<= bitsh;                                                                                             \
                                                                                                                       \
        if (writeLeft) {                                                                                               \
            *(p1 + r) = (*(p1 + r) & mPF0L) | reverseBits[(unsigned char)(shiftCh >> 43)];                             \
            *(p2 + r) = (*(p2 + r) & mPF1L) | (unsigned char)(shiftCh >> 35);                                          \
            *(p3 + r) = (*(p3 + r) & mPF2L) | reverseBits[(unsigned char)(shiftCh >> 27)];                             \
        }                                                                                                               \
        if (writeRight) {                                                                                              \
            *(p4 + r) = (*(p4 + r) & mPF0R) | reverseBits[(unsigned char)(shiftCh >> 23)];                             \
            *(p5 + r) = (*(p5 + r) & mPF1R) | (unsigned char)(shiftCh >> 15);                                          \
            *(p6 + r) = (*(p6 + r) & mPF2R) | reverseBits[(unsigned char)(shiftCh >> 7)];                              \
        }                                                                                                               \
    }

            INNER(0)
            INNER(1)
            INNER(2)
        }

        p1 += 3;
        p2 += 3;
        p3 += 3;
        p4 += 3;
        p5 += 3;
        p6 += 3;

        y += 3;
    }
}


void drawBitmapChar(int ch, int x, int y) {

    if (y + CHAR_Y >= 0 && y < _SCANLINES && x > -8 && x < 40)
        blitShape(ch, x, y, CHAR_Y, _BUF_GAME_PF0_LEFT);
}


void drawAttachedChar(int ch) {

    ch = GET(ch);
    int type = CharToType[ch];

    if (Animate[type])
        ch = *Animate[type];


    int ay = 0;
    int autoY = autoMoveY;
    while (autoY > 0) {
        autoY -= 3;
        ay++;
    }

    int offsetX = 0;
    int offsetY = attachment == CH_DOGE_00 ? -12 : -24;

    if (attachmentOffset) {

        offsetX = attachmentOffset->x;
        offsetY = attachmentOffset->y;

        if ((attachmentOffset + 1)->x || (attachmentOffset + 1)->y)
            attachmentOffset++;

        else
            attachmentOffset = 0;
    }


    int y = playerY * CHAR_Y - (scrollY >> 16) * 3 + autoMoveY /*- 20 + 1*/ - (shakeY >> 16) + offsetY;


    int trixX = ((playerX - 1) * CHAR_TRIX_X) + -(scrollX >> 16) + (faceDirection * (frameAdjustX + (autoMoveX >> 2))) +
                2 - (shakeX >> 16) - offsetX;

    if (y + CHAR_Y >= 0 && y < _SCANLINES && trixX > -8 && trixX < 40)
        blitShape(ch, trixX, y, CHAR_Y, _BUF_GAME_PF0_LEFT);
}


// EOF
