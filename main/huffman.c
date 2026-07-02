#include "huffman.h"

#define HUFF_MAX 10 /* maximum code length in bits */
#define HUFF_N 47   /* number of symbols */

static const char h_syms[HUFF_N] = {...};
static const char h_count[HUFF_MAX + 1] = {...};
static const unsigned short h_first[HUFF_MAX + 1] = {...};
static const char h_start[HUFF_MAX + 1] = {...};


int phrase_decode(const char *src, char *dst, int max_len) {
    unsigned int win = 0;
    int wbits = 0, si = 0, di = 0;

    while (di < max_len - 1) {
        while (wbits < HUFF_MAX && wbits + 8 <= 32) {
            win |= (unsigned int)src[si++] << (24 - wbits);
            wbits += 8;
        }
        for (int len = 1; len <= HUFF_MAX && len <= wbits; len++) {
            unsigned int code = win >> (32 - len);
            unsigned short lo = h_first[len];
            char cnt = h_count[len];
            if (cnt && code >= lo && code < (unsigned int)(lo + cnt)) {
                char ch = (char)h_syms[h_start[len] + (code - lo)];
                win <<= len;
                wbits -= len;
                if (ch == '#') {
                    dst[di] = '\0';
                    return di;
                }
                dst[di++] = ch;
                break;
            }
        }
    }
    dst[di] = '\0';
    return di;
}

// EOF
