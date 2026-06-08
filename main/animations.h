#pragma once

#include <stdbool.h>

#define ANIM_HALT 0
#define ANIM_LOOP 255
#define ANIM_JUMP 254

extern const unsigned char *const AnimateBase[];
extern const unsigned char *Animate[];
extern char AnimCount[];

void initCharAnimations();
void startCharAnimation(int type, const unsigned char *idx);
void processCharAnimations();
void toggleGears(bool active);

// EOF
