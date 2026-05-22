; use this as a template for new game-state/kernels

; add the new state in enum in gameState.h
;      GS_EXAMPLE,    // #
; prototype the functions
;      void initGameState_Example();
;      void VB_Example();
;      void OS_Example();

; make copies of stateExample.c, and kernel_example.asm
; remembering to use new kernel name instead of 'example' (per below)

; in (new) stateExample.c:
; add the following to to initKernel_example()
;      SetJumpVectors(_BUF_EX_JUMP, _kernelExample, _exampleExit, _SCANLINES)
;      setPointer(DSJMP1PTR, _BUF_EX_JUMP);
; add to VB_Example()
;      setPointer(DSJMP1PTR, _BUF_EX_JUMP);

; in main.c
; add the following to end of table (*const verticalBlank[GS_MAX])()
;      VB_Example,       // #
; add the following to end of table  (*const overscan[GS_MAX])()
;      OS_Example,       // #
; add the following to the end of table (*const initialiseKernel[_KERNEL_MAX])
;   initKernel_Example              // #
; add the following to the end of table whichKernel[GS_MAX]
;   _KERNEL_EXAMPLE,            // #
; add the following to the end of table void (*const initialiseGameState[GS_MAX])()
;   initGameState_Example, //#


; in kernels.h add
;      extern void initKernel_Example();

; in (new) kernel_example.asm
; define any data streams using DEFPTR, renaming as required
;      DEFPTR NEW_COLUBK, 0
; use  _DS_NEW_COLUBK to set in ARM, and _DS_NEW_COLUBK_DATA to fetch in ASM

; add in one of the banks (e.g bank_0.asm)
;   BANK_kernelExample = BANK#
;      include "kernels/kernel_example.asm"

; in runVectoredCode.asm
; add a new KERNEL defintion above KERNEL_MAX
; add the new vector entries in kernelBank_L, kernelRoutine_L and kernelRoutine_H

; add a new buffer declaration in _displayData.asm
;
;    SEG.U GS_EXAMPLE
;    ORG _BUFFERS
;
;    DEFBUF 2, EX_JUMP
;    DEFBUF 1, EX_COLUBK
;
;    if * > END_BUFFERS
;END_BUFFERS SET *
;    endif

; add 'stateExample.c' to the SRCS list in the Makefile


START_EXAMPLE = *

    DEF                         ; init defptr block
    DEFPTR EX_COLUBK, 0

kernelExample
_exampleLoop        sta WSYNC

                    lda #0
                    sta PF0
                    sta PF1
                    sta PF2
                    sta GRP0
                    sta GRP1

                    lda #_DS_EX_COLUBK_DATA
                    sta COLUBK

                    jmp 0

_exampleExit
VB_Example
OS_Example
                    rts

    echo "KERNEL EXAMPLE [", (* - START_EXAMPLE)d,"] bytes"

; EOF
