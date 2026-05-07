/******************************************************************************
CDFJ+ Project Framework
Gamax Software 2026 - Craig Daniels
******************************************************************************/

#include <stdbool.h>

#include "defines_dasm.h" // defines_dasm.h MUST come before defines_cdfjplus.h

// MUST have whitespace here, otherwise auto-code formatters will change
// the order of the includes, causing a symbol not defined error.

#include "cdfjplus.h" // <- contains references from defines_dasm.h
#include "tia_constants_c.h"

#include "detectConsole.h"
#include "main.h"
#include "savekey.h"

extern unsigned char saveKeyEnableICC;
int usedSolves;
int whichLot;

void LoadSaveKey();

/******************************* Constants/Defines
 * *******************************/

/******************************* Data/Includes *******************************/

// 32 byte DPC+ "waveforms" - each range from 1 to 5 with 3 being "center" of
// waveform
const unsigned char waveforms[] __attribute__((aligned(4))) = {

	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 0- wave silence
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 1- square
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 4, 4, 4, 4,
	3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, // 2- triangle
	5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3,
	3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, // 3- saw
	3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 4, 4, 4, 4,
	3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, // 4- sin
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5- user waveform 1
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6- user waveform 2
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7- user waveform 3
};
#define WAVEFORM_SIZE sizeof(waveforms)
#define WAV_SILENCE 0
#define WAV_SQUARE 1
#define WAV_TRIANGLE 2
#define WAV_SAW 3
#define WAV_SIN 4
#define RAM_SAMPLE                                                             \
	8 // for 64k+ ROMs samples can be held in DD RAM - this assumes
// it remains directly after the waveform RAM

//	example code to include a file holding a digital sample
//	in the same directory as main.c
const unsigned char sample1[] __attribute__((aligned(4))) = {
#include "sd2.inc"
};
#define SAMPLE1_SIZE sizeof(sample1)

/******************************* Functions *******************************/
void runARM_Initialise();
void runARM_VerticalBlank();
void runARM_LowerVBlank();

void HandleControls();
void SilenceWaves();
void SilenceTIA();
// void SaveKeyWrite(unsigned short address, unsigned char offset,
// 				  unsigned char count);
// void SaveKeyRead(unsigned short address, unsigned char offset,
// 				 unsigned char count);

void S00_Upper(void);
void S00_Lower(void);

void S01_Upper(void);
void S01_Lower(void);

void S02_Upper(void);
void S02_Lower(void);

void S03_Upper(void);
void S03_Lower(void);

void S04_Upper(void);
void S04_Lower(void);

void S05_Upper(void);
void S05_Lower(void);

/******************************* External Functions
 * *******************************/

// function defines from ASM_routines.s
// these use ASM with unrolled loops to make them FAST
// use/remove as desired
extern void ClearChannel(void *ptr);
extern void MemCopy32(void *ptr1, void *ptr2, unsigned int count);
extern void Random(unsigned int count);

/******************************* Variables *******************************/
// stay ARM-side
unsigned int rand = 10531789;	// 32 bit LFSR random number
unsigned int frame = 0;			// frame counter
unsigned short game_state = 0;	// internal ARM game state
unsigned short sample_size = 0; // current digital sample size (bytes)
bool saveKeyDetected = false;	// save key present flag
// to Atari-side
unsigned char kernel = 0; // drawing kernel used/passed Atari-side
unsigned char sound_mode = _SND_MODE_TIA; // sound mode used/passed Atari-side
unsigned short save_key_address =
	0x2600; // replace with whatever address desired

// Atari input direct access variables - joysticks and RESET/SELECT are
// automatically wait/repeat handled
bool input_flag[15] = {false, false, false, false, false, false, false, false,
					   false, false, false, false, false, false, false};
#define p1_u input_flag[0]
#define p1_d input_flag[1]
#define p1_l input_flag[2]
#define p1_r input_flag[3]
#define p0_u input_flag[4]
#define p0_d input_flag[5]
#define p0_l input_flag[6]
#define p0_r input_flag[7]
#define p0_b input_flag[8]
#define p1_b input_flag[9]
#define RESET_swch input_flag[10]
#define SELECT_swch input_flag[11]
#define P0_diff input_flag[12]
#define P1_diff input_flag[13]
#define CBW_swch input_flag[14]

// for each control user can assign a number of frames in between
// the first press acknowledge and the second, default 14
// use 0 to bypass the wait timer
unsigned char input_wait[12] = {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14};
#define p1_u_wait input_wait[0]
#define p1_d_wait input_wait[1]
#define p1_l_wait input_wait[2]
#define p1_r_wait input_wait[3]
#define p0_u_wait input_wait[4]
#define p0_d_wait input_wait[5]
#define p0_l_wait input_wait[6]
#define p0_r_wait input_wait[7]
#define p0_b_wait input_wait[8]
#define p1_b_wait input_wait[9]
#define RESET_swch_wait input_wait[10]
#define SELECT_swch_wait input_wait[11]

// for each control user can assign a number of frames in between
// the second acknowledge and any after, default 7
// use 0 to bypass the repeat timer
unsigned char input_repeat[12] = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
#define p1_u_repeat input_repeat[0]
#define p1_d_repeat input_repeat[1]
#define p1_l_repeat input_repeat[2]
#define p1_r_repeat input_repeat[3]
#define p0_u_repeat input_repeat[4]
#define p0_d_repeat input_repeat[5]
#define p0_l_repeat input_repeat[6]
#define p0_r_repeat input_repeat[7]
#define p0_b_repeat input_repeat[8]
#define p1_b_repeat input_repeat[9]
#define RESET_swch_repeat input_repeat[10]
#define SELECT_swch_repeat input_repeat[11]

// internal control handling variables - no need for direct user access
unsigned short input_counter[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned short input_target[12];

/******************************* Entry Point *******************************/

// Function Prototypes

void GameOverscan();
void GameVerticalBlank();
void initNextLife();
void SystemReset();
void setupBoard();
void processCharAnimations();
void runARM_Null() {}

/*void (*const runFunc[])() = {
	runARM_Null,		// _RUN_ARM_NULL
	runARM_Initialise,	// _RUN_ARM_INIT
	runARM_VerticalBlank, // _RUN_ARM_UPPER_VBLANK
	runARM_LowerVBlank, // _RUN_ARM_LOWER_VBLANK
};

int main() { // <-- 6507/ARM interfaced here!

	if (_RUN_FUNC < sizeof(runFunc) / sizeof(runFunc[0]))
		(*runFunc[_RUN_FUNC])();
	return 0;
}
*/

int main() {

	switch (RAM[_RUN_FUNC]) {
	case _RUN_ARM_INIT:
		runARM_Initialise();
		break;
	case _RUN_ARM_VBLANK:
		runARM_VerticalBlank();
		break;
	case _RUN_ARM_LOWER_VBLANK:
		runARM_LowerVBlank();
		break;
	default:
		break;
	}

	return 0;
}

void runARM_Initialise() {

	saveKeyDetected = RAM[_SWCHA];

	myMemsetInt(
		(void *)_DD_BASE, 0,
		_DISPLAY_SIZE32); // updates 4 bytes at a time for the entire DD area

	for (int i = 0; i < 34; i++)
		setIncrement(i, 1, 0); // increments to 1

	MemCopy32((void *)_WAV_BASE, (void *)waveforms,
			  WAVEFORM_SIZE / 4); // waveforms to DD memory

	//   example code to copy digital sound sample from ARM ROM array
	//   into the _digital_sample DD RAM location to then be played using
	//   setWaveform (0, RAM_SAMPLE);
	MemCopy32((void *)_DD_BASE + _digital_sample, (void *)sample1,
			  SAMPLE1_SIZE / 4);

	//  set up demo jump table 1 for kernel_01
	for (int i = 0; i <= 190; i++) {
		RAM_2B[(_jump_table_1 / 2) + i] = _kernel_01_loop;
	}
	RAM_2B[(_jump_table_1 / 2) + 191] = _kernel_01_done;

	RAM[_kernel] = kernel;
	RAM[_tv_system] = tv_system;
	RAM[_sound_mode] = sound_mode;
	setPointer(DS31PTR, _kernel); // pass initial state to Atari

	//	RAM[_RUN_FUNC] = _RUN_NULL;

	SilenceWaves(); // init DPC waveforms

	LoadSaveKey();
}

static void (*const upperFn[])() = {
	S00_Upper, //
	S01_Upper, //
	S02_Upper, //
	S03_Upper, //
	S04_Upper, //
	S05_Upper, //
};

// upper VBlank dispatcher - includes sample end handler
void runARM_VerticalBlank() {

	detectConsoleType();

	(*upperFn[game_state])();

	if (sound_mode == _SND_MODE_SAMPLE) {
		unsigned int size = (getWavePtr(0) >> 13);
		if (size > (unsigned int)(sample_size - 64)) {
			setNote(0, 0);
		}
	}
}

static void (*const lowerFn[])() = {
	S00_Lower, //
	S01_Lower, //
	S02_Lower, //
	S03_Lower, //
	S04_Lower, //
	S05_Lower, //
};

// lower VBlank dispatcher - includes control handler and communication to Atari
void runARM_LowerVBlank() {
	HandleControls();

	(*lowerFn[game_state])();

	Random(1);
	frame += 1;

	RAM[_kernel] = kernel;
	RAM[_tv_system] = tv_system;
	RAM[_sound_mode] = sound_mode;
	setPointer(DS31PTR, _kernel);
}

// Controller Handler - converts raw input to debounced pulsed wait and repeat
// timings
void HandleControls() {
	unsigned short SWCH_input = (unsigned short)RAM[_SWCHA];
	if ((RAM[_INPT4] & 0b10000000) != 0)
		SWCH_input |= 0x0100;
	if ((RAM[_INPT5] & 0b10000000) != 0)
		SWCH_input |= 0x0200;
	if ((RAM[_SWCHB] & 0b00000001) != 0)
		SWCH_input |= 0x0400;
	if ((RAM[_SWCHB] & 0b00000010) != 0)
		SWCH_input |= 0x0800;

	CBW_swch = ((RAM[_SWCHB] & 0b00001000) != 0);
	P0_diff = ((RAM[_SWCHB] & 0b01000000) != 0);
	P1_diff = ((RAM[_SWCHB] & 0b10000000) != 0);

	for (int i = 0; i <= 11; i++) {
		input_flag[i] = false;
		if ((SWCH_input & 1) == 0) {
			input_counter[i]++;
			if (input_counter[i] == 1) {
				input_flag[i] = true;
				input_target[i] = input_wait[i] + 1;
			}
			if (input_counter[i] == input_target[i]) {
				input_flag[i] = true;
				input_target[i] = input_target[i] + input_repeat[i] + 1;
			}
		} else {
			input_counter[i] = 0;
		}
		SWCH_input = SWCH_input / 2;
	}
}

// Used to set waveforms to all silent / no note
void SilenceWaves() {
	setWaveform(0, WAV_SILENCE);
	setWaveform(1, WAV_SILENCE);
	setWaveform(2, WAV_SILENCE);
	setNote(0, 0);
	setNote(1, 0);
	setNote(2, 0);
}

// Used to set TIA sound to all silent / no note
void SilenceTIA() {
	RAM[_AUDV0] = 0;
	RAM[_AUDC0] = 0;
	RAM[_AUDF0] = 0;
	RAM[_AUDV1] = 0;
	RAM[_AUDC1] = 0;
	RAM[_AUDF1] = 0;
}

void LoadSaveKey() {

	saveKeyEnableICC = RAM[_SK_ENABLE_ICC];
	usedSolves = RAM[_SK_USED_SOLVES];
	whichLot = RAM[_SK_LAST];

	for (int i = 0; i < SAVEKEY_SIZE; i++) {
		saveKeyPerfect[i] = RAM[_SK_PER + i];
		saveKeyUnlocked[i] = RAM[_SK_UNL + i];
		saveKeySolved[i] = RAM[_SK_SLV + i];
	}

	// increment counter

	int count = RAM[_SK_COUNT] + 256 * RAM[_SK_COUNT + 1] + 1;
	RAM[_SK_COUNT] = count;
	RAM[_SK_COUNT + 1] = count >> 8;
}

// Write to SaveKey
// void SaveKeyWrite(unsigned short address, unsigned char offset,
// 				  unsigned char count) {
// 	RAM_2B[_save_addr_l / 2] = address;
// 	RAM[_save_count] = count;
// 	RAM[_save_offset] = offset;
// 	if (saveKeyDetected)
// 		RAM[_save_command] = _SAVE_KEY_WRITE;
// }

// // Read from SaveKey
// void SaveKeyRead(unsigned short address, unsigned char offset,
// 				 unsigned char count) {
// 	RAM_2B[_save_addr_l / 2] = address;
// 	RAM[_save_count] = count;
// 	RAM[_save_offset] = offset;
// 	if (saveKeyDetected)
// 		RAM[_save_command] = _SAVE_KEY_READ;
// }

/**************************** State 00 ****************************/
// test game state 00 - TIA sound mode and SaveKey
void S00_Upper(void) {
	RAM[_buffer0] += 1;
	for (int i = 1; i <= 191; i++) {
		RAM[_buffer0 + i] = RAM[_buffer0 + i - 1] + 1;
	}
	setPointer(DS0PTR, _buffer0);
}

void S00_Lower(void) {
	if (p0_r) // right to change to digital sample sound mode
	{
		kernel = 1;
		game_state = 1;
		sound_mode = _SND_MODE_SAMPLE;
	}

	// if (p0_u) // up to write a single byte to EEPROM offset 6
	// {
	// 	RAM[_save_data + 6] = 5;
	// 	SaveKeyWrite(save_key_address + 6, 6, 1);
	// }

	// if (p0_d) // down to write a pair of bytes to EEPROM offset 1
	// {
	// 	RAM[_save_data + 1] = 7;
	// 	RAM[_save_data + 2] = 26;
	// 	SaveKeyWrite(save_key_address + 1, 1, 2);
	// }

	// if (p0_b) // p0 button to read entire 64 byte block
	// {
	// 	SaveKeyRead(save_key_address, 0, 64);
	// }
}

/**************************** State 01 ****************************/
// test game state 01 - digital sample sound mode
void S01_Upper(void) {
	RAM[_buffer0] -= 1;
	for (int i = 1; i <= 191; i++) {
		RAM[_buffer0 + i] = RAM[_buffer0 + i - 1] - 1;
	}
	setPointer(DS0PTR, _buffer0);
	setPointer(DSJMP1PTR, _jump_table_1);
}

void S01_Lower(void) {
	if (p0_l) { // left to change to TIA sound mode
		game_state = 0;
		kernel = 0;
		sound_mode = _SND_MODE_TIA;
		SilenceWaves();
	}

	if (p0_r) { // right to change to DPC sound mode
		kernel = 1;
		game_state = 2;
		sound_mode = _SND_MODE_DPC;
		SilenceWaves();
	}

	if (p0_u) { // up for sample play from ROM
		resetWave(0);
		setNote(0, 1600);
		setSamplePtr(_sample_steel);
		sample_size = _sample_steel_size;
	}

	if (p0_d) { // down for sample play from buffer
		resetWave(0);
		setNote(0, 800);
		setWaveform(0, RAM_SAMPLE);
		sample_size = SAMPLE1_SIZE;
	}
}

/**************************** State 02 ****************************/
// test game state 02 - DPC+ sound mode
void S02_Upper(void) {
	RAM[_buffer0 + 191] -= 1;
	for (int i = 190; i >= 0; i--) {
		RAM[_buffer0 + i] = RAM[_buffer0 + i + 1] - 1;
	}
	setPointer(DS0PTR, _buffer0);
	setPointer(DSJMP1PTR, _jump_table_1);
}

void S02_Lower(void) {

	if (p0_l) // left to change to digital sample sound mode
	{
		kernel = 1;
		game_state = 1;
		sound_mode = _SND_MODE_SAMPLE;
		SilenceWaves();
	}
	if (p0_u) // up to play wave channel 0
	{
		setWaveform(0, WAV_TRIANGLE);
		setNote(0, getPitch(58));
	}
	if (p0_r) // right to play wave channel 1
	{
		setWaveform(1, WAV_SQUARE);
		setNote(1, getPitch(60));
	}
	if (p0_d) // down to play wave channel 2
	{
		setWaveform(2, WAV_SAW);
		setNote(2, getPitch(62));
	}
	if (p0_b) // p0 button to silence all wave channels
	{
		SilenceWaves();
	}
}

/**************************** State 03 ****************************/
void S03_Upper(void) {}

void S03_Lower(void) {}

/**************************** State 04 ****************************/
void S04_Upper(void) {}

void S04_Lower(void) {}

/**************************** State 05 ****************************/
void S05_Upper(void) {}

void S05_Lower(void) {}
