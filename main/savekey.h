#pragma once

#include "cdfjplus.h"
#include "defines_dasm.h"

#include <stdbool.h>

#define SAVEKEY_SIZE ((100 + 7) / 8)

extern unsigned char saveKeyUnlocked[SAVEKEY_SIZE];
extern unsigned char saveKeySolved[SAVEKEY_SIZE];
extern unsigned char saveKeyPerfect[SAVEKEY_SIZE];
extern unsigned char saveKeyEnableICC;

void setUnlockStatus(int lot);
bool getUnlockStatus(int lot);
bool getSolveStatus(int lot);
bool getPerfectStatus(int lot);
void setPerfectStatus(int lot);
bool isGameCompleted();
bool unlockLevels();
bool isGroupSolved(int lot);
bool isGroupPerfect(int lot);
// bool isPriorSolved(int lot);
// bool isPriorPerfect(int lot);
void setSolveStatus(int lot);

// extern bool priorGroupSolved[];
// extern bool priorGroupPerfect[];

// EOF
