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
void driveTeleportSpin(bool fast);

// Freezes a type's shared animation on whatever frame it's currently showing -- processCharAnimations()
// skips auto-advancing any type whose AnimCount == ANIM_HALT, same mechanism AnimTeleport's frames
// use (see its own comment), just applied to a type that's normally auto-advancing instead of one
// that never was.
void haltCharAnimation(int type);

extern const unsigned char AnimateRockBonus[];
extern const unsigned char AnimateStar[];
extern const unsigned char AnimateStarExplode[];
extern const unsigned char AnimateCrackedBrick[];
extern const unsigned char AnimateBomb[];
extern const unsigned char AnimTeleport[];
extern const unsigned char AnimateDoor[];
extern const unsigned char AnimateDoorClose[];
extern const unsigned char AnimFlashOut[];

// EOF
