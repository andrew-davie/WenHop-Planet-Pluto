#pragma once
#include <stdbool.h>
#include <stdint.h>

#define CHAR_HEIGHT 33
typedef struct {
	unsigned char data[CHAR_HEIGHT]; // RGB
} character;

extern const character charset[];
// extern const unsigned char gfx_cars_mockup_gif_map[6][8];
//  extern const unsigned char *chassis[];
void findCarUsingXY(int x, int y);

typedef struct {
	//	const unsigned char *data;
	int width;
	int height;
	bool flip;
} carDef;

typedef struct {
	int carType;
	unsigned char height;
	unsigned char width;
} carRef;

typedef struct {
	const unsigned short *units;
	int carCount; // number of units in this puzzle
	              // int carx;       // start car position
	              // int difficulty; // number of moves to complete
} puzzle_group;

extern const uint16_t puzzleUnits[];
extern const puzzle_group puzzle;

extern const carDef carData[];
extern uint16_t carDataMap[];
// extern int puzzleID;
extern int optimalMoves;
extern bool open;
extern bool openPassage;
extern int idleTime;
// extern bool priorGroupSolved[];
// extern bool priorGroupPerfect[];
extern bool showIntermission;
extern bool showBest;

#define CXSHIFT 6
extern int ccx;

void initCars(int whichLot);
void moveCars();
int clipLot(int lot);
void vectorJoystick();
// void initCat();

// EOF
