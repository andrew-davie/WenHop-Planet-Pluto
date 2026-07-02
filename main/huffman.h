#pragma once

typedef struct {
    const char *data;
    char stars;
} Phrase;

int phrase_decode(const char *src, char *dst, int max_len);

// EOF
