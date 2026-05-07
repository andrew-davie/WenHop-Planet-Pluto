#include "savekey.h"
#include "main.h"

#define PUZZLE_MAX 100
#define GROUPING 10

int unlockedStart;
int unlockedEnd;

unsigned char saveKeyEnableICC;
unsigned char saveKeyUnlocked[SAVEKEY_SIZE];
unsigned char saveKeySolved[SAVEKEY_SIZE];
unsigned char saveKeyPerfect[SAVEKEY_SIZE];

// bool priorGroupSolved[PUZZLE_MAX / GROUPING];
// bool priorGroupPerfect[PUZZLE_MAX / GROUPING];

void setUnlockStatus(int lot) {
	int l = (lot >> 3);
	saveKeyUnlocked[lot >> 3] |= 1 << (lot & 7);

#ifndef RESTRICTED_DEMO
	RAM[_SK_UNL + l] = saveKeyUnlocked[l];
#endif
}

bool getUnlockStatus(int lot) {
	return saveKeyUnlocked[lot >> 3] & (1 << (lot & 7));
}

bool getSolveStatus(int lot) {
	return saveKeySolved[lot >> 3] & (1 << (lot & 7));
}

bool getPerfectStatus(int lot) {
	return saveKeyPerfect[lot >> 3] & (1 << (lot & 7));
}

void setPerfectStatus(int lot) {
	int l = (lot >> 3);
	saveKeyPerfect[l] |= (1 << (lot & 7));
#ifndef RESTRICTED_DEMO
	RAM[_SK_PER + l] = saveKeyPerfect[l];
#endif
}

bool isGameCompleted() {
	int completed = 0;
	for (int i = 0; i < PUZZLE_MAX /* && completed*/; i++)
		if (getSolveStatus(i))
			completed++;
#ifdef RESTRICTED_DEMO
	return completed == 10;
#else
	return completed == PUZZLE_MAX;
#endif
}

bool unlockLevels() {

	bool unlocked = false;
	int totalSolves = 1;

	for (int lot = 0; lot < PUZZLE_MAX; lot += GROUPING) {

		bool solved = true;
		bool perfect = true;

		for (int i = lot; i < lot + GROUPING && solved; i++) {
			solved &= getSolveStatus(i);
			perfect &= getPerfectStatus(i);
		}

		if (solved) {

			totalSolves++;

			if (perfect)
				totalSolves++;

#ifndef RESTRICTED_DEMO
			RAM[_SK_TOTAL_SOLVES] = totalSolves;
#endif
			int size = (perfect && lot < 90) ? 2 * GROUPING : GROUPING;

			unlockedStart = -1;

			for (int i = lot + GROUPING;
				 i < lot + GROUPING + size && i < PUZZLE_MAX; i++) {

				if (!getUnlockStatus(i)) {
					if (unlockedStart < 0)
						unlockedStart = i + 1;
					unlocked = true;
				}
				setUnlockStatus(i);
				unlockedEnd = i + 1;
			}
		}
	}

	return unlocked;
}

// bool isGroupSolved(int lot) {

// 	// is the current GROUPING block completely solved?
// 	bool solved = false;

// 	for (int i = 0; i < PUZZLE_MAX; i += GROUPING)
// 		for (int j = 0; j < GROUPING; j++)
// 			if (i + j == lot) {
// 				solved = true;
// 				for (int k = 0; k < GROUPING; k++)
// 					if (!getSolveStatus(i + k)) {
// 						solved = false;
// 						goto done;
// 					}
// 				goto done;
// 			}

// done:
// 	return solved;
// }

// bool isGroupPerfect(int lot) {

// 	// is the current GROUPING block completely solved?
// 	bool solved = false;

// 	for (int i = 0; i < PUZZLE_MAX; i += GROUPING)
// 		for (int j = 0; j < GROUPING; j++)
// 			if (i + j == lot) {
// 				solved = true;
// 				for (int k = 0; k < GROUPING; k++)
// 					if (!getPerfectStatus(i + k)) {
// 						solved = false;
// 						goto done;
// 					}
// 				goto done;
// 			}

// done:
// 	return solved;
// }

// bool isPriorSolved(int lot) {

// 	int grp = 0;
// 	while (lot >= GROUPING) {
// 		lot -= GROUPING;
// 		grp++;
// 	}

// 	return priorGroupSolved[grp];
// }

// bool isPriorPerfect(int lot) {

// 	int grp = 0;
// 	while (lot >= GROUPING) {
// 		lot -= GROUPING;
// 		grp++;
// 	}

// 	return priorGroupPerfect[grp];
// }

void setSolveStatus(int lot) {
	int l = lot >> 3;
	saveKeySolved[l] |= 1 << (lot & 7);
#ifndef RESTRICTED_DEMO
	RAM[_SK_SLV + l] = saveKeySolved[l];
#endif
}

// EOF