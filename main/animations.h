#pragma once

#include <stdbool.h>

#define ANIM_HALT 0
#define ANIM_LOOP 255
#define ANIM_JUMP 254
#define ANIM_ELECTRIC 253

extern const unsigned char *const AnimateBase[];
extern const unsigned char PickupCharacter[];
extern const unsigned char *Animate[];

void initCharAnimations();
void startCharAnimation(int type, const unsigned char *idx);
void processCharAnimations();
void toggleGears(bool active);

extern const unsigned char AnimateRockBonus[];
extern const unsigned char AnimateStar[];
extern const unsigned char AnimateStarExplode[];
extern const unsigned char AnimateCrackedBrick[];

// EOF
