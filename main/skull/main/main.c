// clang-format off

#include <limits.h>
#include <stdbool.h>

#include "defines_cdfj.h"
#include "defines_from_dasm_for_c.h"

#include "defines.h"

#include "main.h"
#include "menu.h"

#include "menu.h"
#include "random.h"
#include "sound.h"

// clang-format on

bool rageQuit;
unsigned char inpt4;
unsigned char swcha;
int time;

unsigned char mm_tv_type; // 0 = NTSC, 1 = PAL, 2 = PAL-60, 3 = SECAM... start @
                          // NTSC always

int gameFrame;
int gameSpeed;
unsigned int triggerPressCounter;

unsigned int currentPalette;

#if ENABLE_SHAKE
int shakeX, shakeY;
int shakeTime;
#endif

// COMPILE-TIME REVERSE BITS IN BYTE
#define RVS(a)                                                                 \
  (((((a) >> 0) & 1) << 7) | ((((a) >> 1) & 1) << 6) |                         \
   ((((a) >> 2) & 1) << 5) | ((((a) >> 3) & 1) << 4) |                         \
   ((((a) >> 4) & 1) << 3) | ((((a) >> 5) & 1) << 2) |                         \
   ((((a) >> 6) & 1) << 1) | ((((a) >> 7) & 1) << 0))

#define P0(a) RVS(a)
#define P1(a) P0(a), P0(a + 1)
#define P2(a) P1(a), P1(a + 2)
#define P3(a) P2(a), P2(a + 4)
#define P4(a) P3(a), P3(a + 8)
#define P5(a) P4(a), P4(a + 16)
#define P6(a) P5(a), P5(a + 32)
#define P7(a) P6(a), P6(a + 64)
#define P8(a) P7(a), P7(a + 128)

// Want to call RVS(n) for 0-255 values. The weird #defines above allows a
// single-call It's effectively a recursive power-of-two call of the base RVS
// macro

const unsigned char BitRev[] = {
    P8(0),
};

// Function Prototypes

void initNextLife();
void SystemReset();
void setupBoard();
void processCharAnimations();

static void (*const runFunc[])() = {

    SystemReset, // _FN_SYSTEM_RESET

    MenuOverscan,      // _FN_MENU_OS
    MenuVerticalBlank, // _FN_MENU_VB
    SchedulerMenu,     // _FN_MENU_IDLE
};

int main() { // <-- 6507/ARM interfaced here!

  (*runFunc[RUN_FUNC])();
  return 0;
}

void SystemReset() {

  initRandom();
  initAudio();
  startMusic();
  initMenuDatastreams();

  for (int i = 0; i <= 34; i++)
    QINC[i] = 0x100; // data stream increments -> 1.0

  initKernel(KERNEL_COPYRIGHT);

  rageQuit = false;
  ARENA_COLOUR = 1;
}

void setJumpVectors(int midKernel, int exitKernel) {

  unsigned short int *p = &RAM_SINT[_BUF_JUMP1 / 2];
  for (int i = 0; i < _ARENA_SCANLINES; i++)
    *p++ = midKernel;
  *p = exitKernel;
}

const unsigned char sinoid[] = {0, 1, 1, 3, 5, 6, 6, 7, 7, 6, 6, 5, 3, 1, 1, 0};

// EOF