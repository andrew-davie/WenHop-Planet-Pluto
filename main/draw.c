#include <stdbool.h>

#include "defines_dasm.h"

#include "bitPatterns.h"
#include "cdfjplus.h"    // <- contains references from defines_dasm.h
#include "colour.h"
#include "draw.h"
#include "sound.h"

#include "../gfx/alphanumeric.h"
#include "../gfx/fontcompact.h"
#include "../gfx/fontlarge.h"


int dramaticPause;


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
        // if (y++ >= 0) {
        q[line] = colour;
        for (int c = 0; c < 6; c++)
            p[c * _BUFFER_SIZE + line] = *bm++;
        // }

        // else {
        //     bm += 6;
        // }
    }
}


static const char *ps = 0;
static unsigned char *buf;
static unsigned char *colrx;
static int cx;
static int cy;
static int font;
static int colour;
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
    const unsigned char **asciiTable;
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
    colour = convertColour(c);
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
//  ⎮      ⎧ 🟨 = underline
//  ⎮      ⎮⎧  🟨 = left-justify
//  ⎮      ⎮⎮ ⎧  🟨 = right-justify
//  ⎮      ⎮⎮ ⎮ ⎮
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ' '         32
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '!'         33
    🟨🟦🟦🟦🟨🟦🟦🟦 ,    //   '"'         34
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '#'         35
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '$'         36       replacement '.' without a delay (for numbers0
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '%'         37
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '&'         38
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '''         39 
    🟨🟦🟦🟦🟦🟦🟦🟨 ,    //   '('         40
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   ')'         41
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '*'         42
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   '+'         43
    🟨🟦🟦🟦🟦🟦🟨🟦 ,    //   ','         44       + short pause
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '-'         45
    🟨🟦🟦🟦🟨🟦🟦🟦 ,    //   '.'         46       + long pause if '|' follows
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
    🟨🟦🟦🟦🟨🟦🟨🟦 ,    //   '?'         63
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
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'J'         74
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
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '🟨'         88
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Y'         89
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'Z'         90
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '['         91
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   'backslash  92
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   ']'         93
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '^'         94
    🟦🟦🟦🟦🟦🟦🟦🟦 ,    //   '🟦'         95      toggle underline
    🟨🟦🟦🟦🟦🟦🟦🟦 ,    //   '`'         96
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'a'         97
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'b'         98
    🟨🟦🟦🟦🟦🟨🟦🟨 ,    //   'c'         99
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'd'         🟨🟦🟦
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
    🟨🟦🟦🟦🟦🟨🟨🟦 ,    //   't'         116
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'u'         117
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'v'         118
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   'w'         119
    🟨🟦🟦🟦🟦🟨🟦🟦 ,    //   '🟨'         120
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


void doLetter(int ci, int cx, int cy, char colour) {

    int column = cx >> 3;
    int bit = 7 - (cx & 7);

    const unsigned char *charShape = fonts[font].asciiTable[ci];
    if (charShape) {

        for (int i = 0; i < fonts[font].lineHeight; i++) {

            int vertPos = cy + i;
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

            doLetter(ci, cx, cy, colour);
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


// EOF
