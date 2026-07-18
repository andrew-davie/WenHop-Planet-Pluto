#pragma once

enum CC {

    // ChronoColour modes.
    // Explicit values to prevent idiots rearranging the order.

    CC_NONE = 0,
    CC_PCC = 1,
    CC_ICC = 2,
};

#define PUZZLE_MAX 100
#define SAVEKEY_SIZE ((PUZZLE_MAX + 7) / 8)

void setUnlockStatus(int lot);
bool getUnlockStatus(int lot);
bool getSolveStatus(int lot);
bool getPerfectStatus(int lot);
void setPerfectStatus(int lot);
bool isGameCompleted();
bool unlockLevels();
void setSolveStatus(int lot);


extern enum CC saveKeyEnableICC;

// EOF
