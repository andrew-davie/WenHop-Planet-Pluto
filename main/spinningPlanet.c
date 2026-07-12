
#include <math.h>
#include <stdint.h>
#include <string.h>

#define SPHERE_W 32
#define SPHERE_H 66
#define ONE7 128
#define ONE7SQ 16384
#define AMAX 128
#define AMASK 127
#define AQUART 32
#define AHALF 64
#define TEX_TW 5
#define TEX_TH 8
#define TEX_TX 40
#define TEX_TY 22
#define TEX_W 200
#define TEX_H 176
#define ATAN_FINE 1024

static int8_t asin_table[ONE7 + 1];
static int8_t atan_table[ATAN_FINE + 1];
static uint16_t recip_table[ONE7 + 1];
static int8_t norm7x[SPHERE_W];
static int8_t norm7y[SPHERE_H];

void sphere_init(void) {
    for (int i = 0; i <= ONE7; i++)
        asin_table[i] = (int8_t)lround(asin(i / (double)ONE7) / (2.0 * M_PI) * (double)AMAX);
    for (int i = 0; i <= ATAN_FINE; i++)
        atan_table[i] = (int8_t)lround(atan(i / (double)ATAN_FINE) / (2.0 * M_PI) * (double)AMAX);
    recip_table[0] = 16384u;
    for (int i = 1; i <= ONE7; i++)
        recip_table[i] = (uint16_t)(16384u / (unsigned)i);
    for (int i = 0; i < SPHERE_W; i++)
        norm7x[i] = (int8_t)lround((i - (SPHERE_W - 1) * 0.5) / (SPHERE_W * 0.5) * (double)ONE7);
    for (int i = 0; i < SPHERE_H; i++)
        norm7y[i] = (int8_t)lround((i - (SPHERE_H - 1) * 0.5) / (SPHERE_H * 0.5) * (double)ONE7);
}

static inline int32_t isqrt7(int32_t n) {
    if (n <= 0)
        return 0;
#if defined(__arm__) || defined(__aarch64__)
    int32_t x = 1 << ((31 - __builtin_clz((uint32_t)n)) >> 1);
#else
    int32_t x = 1;
    while (x * x <= n)
        x <<= 1;
    x >>= 1;
#endif
    x = (x + n / x) >> 1;
    x = (x + n / x) >> 1;
    while (x * x > n)
        x--;
    return x;
}

static inline int32_t fp_asin(int32_t x) {
    int32_t neg = x < 0;
    int32_t ax = neg ? -x : x;
    if (ax > ONE7)
        ax = ONE7;
    int32_t r = asin_table[ax];
    return neg ? -r : r;
}

static inline int32_t fp_atan2(int32_t y, int32_t x) {
    if (x == 0 && y == 0)
        return 0;
    int32_t ax = x < 0 ? -x : x;
    int32_t ay = y < 0 ? -y : y;
    int32_t angle;
    if (ax >= ay) {
        int32_t axc = ax > ONE7 ? ONE7 : ax;
        int32_t t = axc ? (ay * (int32_t)recip_table[axc]) >> 4 : 0;
        if (t > ATAN_FINE)
            t = ATAN_FINE;
        angle = atan_table[t];
    } else {
        int32_t ayc = ay > ONE7 ? ONE7 : ay;
        int32_t t = ayc ? (ax * (int32_t)recip_table[ayc]) >> 4 : 0;
        if (t > ATAN_FINE)
            t = ATAN_FINE;
        angle = AQUART - atan_table[t];
    }
    if (x < 0)
        angle = AHALF - angle;
    if (y < 0)
        angle = (-angle) & AMASK;
    return angle & AMASK;
}

/* Sample one channel tile map, return 0 or 1 */
static inline int32_t tile_bit(const uint8_t *tiles, int32_t tx, int32_t ty) {
    if (tx >= TEX_W)
        tx = TEX_W - 1;
    if (ty >= TEX_H)
        ty = TEX_H - 1;
    int32_t tile_col = tx / TEX_TW;
    int32_t tile_row = ty / TEX_TH;
    int32_t tile_px = tx - tile_col * TEX_TW;
    int32_t tile_py = ty - tile_row * TEX_TH;
    const uint8_t *tile = tiles + (tile_row * TEX_TX + tile_col) * TEX_TH;
    return (tile[tile_py] >> (7 - tile_px)) & 1;
}

void sphere_render(const uint8_t *tiles_r, const uint8_t *tiles_g, const uint8_t *tiles_b, uint8_t *out,
                   int32_t rotation, int32_t zoom_pct) {
    for (int py = 0; py < SPHERE_H; py++) {
        int32_t ny7 = norm7y[py];
        int32_t ny2 = ny7 * ny7;

        for (int px = 0; px < SPHERE_W; px++) {
            int32_t nx7 = norm7x[px];
            int32_t r2 = nx7 * nx7 + ny2;

            if (r2 >= ONE7SQ) {
                *out++ = 0;
                continue;
            }

            int32_t znx7 = (nx7 * zoom_pct * 655) >> 16;
            int32_t zny7 = (ny7 * zoom_pct * 655) >> 16;
            int32_t zr2 = znx7 * znx7 + zny7 * zny7;
            int32_t nz7;
            if (zr2 >= ONE7SQ) {
                int32_t zlen = isqrt7(zr2);
                znx7 = (znx7 * 127) / zlen;
                zny7 = (zny7 * 127) / zlen;
                nz7 = 1;
            } else {
                nz7 = isqrt7(ONE7SQ - zr2);
            }

            int32_t lat_a = fp_asin(zny7);
            int32_t lon_a = fp_atan2(znx7, nz7);
            if (lon_a > AHALF)
                lon_a -= AMAX;
            lon_a = (lon_a + rotation + AMAX) & AMASK;

            int32_t tx = (lon_a * 25) >> 4;
            int32_t ty = ((lat_a + AQUART) * 11) >> 2;
            if (ty >= TEX_H)
                ty = TEX_H - 1;

            /* combine 3 channels into colour index 0..7 */
            int32_t col = tile_bit(tiles_r, tx, ty) | tile_bit(tiles_g, tx, ty) << 1 | tile_bit(tiles_b, tx, ty) << 2;
            *out++ = (uint8_t)col;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Demo: gcc -DSPHERE_DEMO -O2 -o sphere_demo sphere2600.c -lm
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifdef SPHERE_DEMO
#include <stdio.h>
#include <stdlib.h>

/* simple deterministic noise for tile generation */
static uint32_t tile_hash(int x, int y, int seed) {
    uint32_t h = (uint32_t)(x * 1619 + y * 31337 + x * y * 6271 + seed * 104729);
    h ^= h >> 16;
    h *= 0x45d9f3bu;
    h ^= h >> 16;
    return h;
}

/* fill one tile layer: each tile solid or empty based on smoothed noise */
static void make_layer(uint8_t *tiles, int seed) {
    uint8_t solid = 0;
    for (int b = 0; b < TEX_TW; b++)
        solid |= (uint8_t)(1 << (7 - b));

    for (int tr = 0; tr < TEX_TY; tr++) {
        for (int tc = 0; tc < TEX_TX; tc++) {
            /* smooth over 5x5 neighbourhood */
            uint32_t sum = 0, weight = 0;
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int nx = ((tc + dx) % TEX_TX + TEX_TX) % TEX_TX;
                    int ny = ((tr + dy) % TEX_TY + TEX_TY) % TEX_TY;
                    /* weight: approximate gaussian via distance */
                    uint32_t w = 16 >> (abs(dx) + abs(dy));
                    if (w == 0)
                        w = 1;
                    sum += (tile_hash(nx, ny, seed) >> 16) * w;
                    weight += 0xffffu * w;
                }
            }
            int land = (sum * 2 > weight) ? 1 : 0;
            uint8_t *t = tiles + (tr * TEX_TX + tc) * TEX_TH;
            uint8_t v = land ? solid : 0;
            for (int r = 0; r < TEX_TH; r++)
                t[r] = v;
        }
    }
}

static const char *colour_names[] = {"blk", "red", "grn", "yel", "blu", "mag", "cyn", "wht"};
/* simple ANSI colours for terminal preview */
static const char *ansi[] = {"\033[40m  ", "\033[41m  ", "\033[42m  ", "\033[43m  ",
                             "\033[44m  ", "\033[45m  ", "\033[46m  ", "\033[47m  "};

static void print_frame(const uint8_t *out, int use_ansi) {
    for (int py = 0; py < SPHERE_H; py++) {
        for (int px = 0; px < SPHERE_W; px++) {
            int v = out[py * SPHERE_W + px];
            if (use_ansi) {
                fputs(ansi[v], stdout);
            } else {
                if (!v)
                    fputs("  ", stdout);
                else
                    printf("%2s", colour_names[v]);
            }
        }
        if (use_ansi)
            fputs("\033[0m", stdout);
        putchar('\n');
    }
}

int main(int argc, char **argv) {
    int use_ansi = (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'c');

    sphere_init();

    size_t layer_bytes = (size_t)(TEX_TX * TEX_TY * TEX_TH);
    uint8_t *tr = malloc(layer_bytes);
    uint8_t *tg = malloc(layer_bytes);
    uint8_t *tb = malloc(layer_bytes);
    if (!tr || !tg || !tb)
        return 1;

    make_layer(tr, 1);
    make_layer(tg, 2);
    make_layer(tb, 3);

    uint8_t out[SPHERE_W * SPHERE_H];

    int rots[] = {0, 16, 32, 48};
    for (int i = 0; i < 4; i++) {
        printf("\n--- rot=%d (%d deg) zoom=100 ---\n", rots[i], rots[i] * 360 / AMAX);
        sphere_render(tr, tg, tb, out, rots[i], 100);
        print_frame(out, use_ansi);
    }

    free(tr);
    free(tg);
    free(tb);
    return 0;
}
#endif