#pragma once

#define ANIM_HALT 0
#define ANIM_LOOP 255
#define ANIM_JUMP 254

extern const unsigned char *const AnimateBase[];
extern const unsigned char *Animate[];
extern char AnimCount[];

extern void initCharAnimations();
extern void startCharAnimation(int type, const unsigned char *idx);
extern void processCharAnimations();

// EOF
