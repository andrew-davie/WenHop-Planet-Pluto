	org CURRENT_ORG
	rorg $f000

.BANK SET  BANK0

BANK0_START


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@ Code block that gets copied into RAM for bank routine dispatch @@@@@@@@@@@@@@@@@

; The code here is a 'template' copied into the RAM area and the bank and jump
; address are modified before execution. The return will be to the caller of
; the code that jumps to the RAM copy of this code.
; DO NOT RUN *THIS* CODE AS IT WILL CRASH. JUMP TO THE RAM VERSION!

jumpCode       	    cmp BANK1           ; hotspot bank-switch
                    jsr jumpCode
                    cmp BANK0           ; hotspot bank-switch
jumpCodeEnd         rts


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Cart Reset @@@@@@@@@@@@@@@@@@@@@@@@@@@@

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@ Tables for Frame Dispatch @@@@@@@@@@@@

.sound_mode_table		;for auto-adjust to SETMODE mirror
	.byte $00
	.byte $f0
	.byte $00

.call_fn_table			;for auto-adjust to call_fn mirror
	.byte $ff
	.byte $fe
	.byte $fe

;-------------------------------------------------------------------------------

CartReset

                    CLEAN_START

    ; Copy jump dispatch handler to RAM

	                ldx #(jumpCodeEnd-jumpCode)
initJumpCode        lda jumpCode,x
	                sta jumpCodeRAM,x
	                dex
	                bpl initJumpCode


                    jsr ReadSaveKey                 ; Load savekey to ZP SAVEKEY block

                    ldx #FASTON
                    stx SETMODE			            ; "Fast Fetch" enable

    ; Send the SAVEKEY block to ARM-accessible shadow variables
    ; A diff is (later) used to determine any changes that need to be written

                    ldx #>_DS_SK
                    stx DSPTR
                    ldx #<_DS_SK
                    stx DSPTR

                    ldx #0
.sendSK             lda SAVEKEY_IDENT,x
                    sta DSWRITE
                    inx
                    cpx #SK_BYTES + 1
                    bcc .sendSK
 
    ; Call ARM Initialise

                    ldx #>_RUN_FUNC
                    stx DSPTR
                    ldx #<_RUN_FUNC
                    stx DSPTR
                    ldx #_RUN_ARM_INIT              ; routine to run in main()
                    stx DSWRITE

                    ldx #$FF
                    stx CALLFN          		    ; Initialise via ARM function

    ; At this point, the ARM function handler runs the Initialise function
    ; Retrive critical configuration variables from ARM

                    lda #DS31DATA
                    sta kernel
                    lda #DS31DATA
                    sta tvSystem                    ; <--- NTSC here

                    lda #DS31DATA
                    sta soundMode
                    sta soundSave

                    tax
                    lda .sound_mode_table,x
                    sta SETMODE         			    ; set proper sound mode [???]
                    lda .call_fn_table,x
                    sta call_fn			                ; apply proper function caller

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Game Loop @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    include "mainLoop.asm"

;-------------------------------------------------------------------------------




;-------------------------------------------------------------------------------
;@@@@@@@@@@@@@@@@@@@@@@@@@ Cross Bank Routine Handler @@@@@@@@@@@@@@@@@@@@@@@@@

VB_FN_OFFSET = ((.END-.START)/3)
OS_FN_OFFSET = (VB_FN_OFFSET * 2)


_KERNEL_RAINBOW     = 0
_KERNEL_01          = 1
_KERNEL_COPYRIGHT   = 2


kernelBank_L
.START
                    ; >>> kernel
                    .byte <BANK_kernelRainbow       ; 0 KERNEL_RAINBOW
                    .byte <BANK_kernel_01           ; 1 KERNEL_01
                    .byte <BANK_kernelCopyright     ; 2 KERNEL_COPYRIGHT

                    ; >>> VB
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernel_01
                    .byte <BANK_kernelCopyright

                    ; >>> OS
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernel_01
                    .byte <BANK_kernelCopyright
.END

kernelRoutine_L
                    .byte <kernelRainbow
                    .byte <kernel_01
                    .byte <kernelCopyright

                    .byte <VB_kernelRainbow
                    .byte <VB_kernel_01
                    .byte <VB_kernelCopyright

                    .byte <OS_kernelRainbow
                    .byte <OS_kernel_01
                    .byte <OS_kernelCopyright

kernelRoutine_H
                    .byte >kernelRainbow
                    .byte >kernel_01
                    .byte >kernelCopyright

                    .byte >VB_kernelRainbow
                    .byte >VB_kernel_01
                    .byte >VB_kernelCopyright

                    .byte >OS_kernelRainbow
                    .byte >OS_kernel_01
                    .byte >OS_kernelCopyright

;-------------------------------------------------------------------------------

runVectoredCode

    ; self-modify the in-RAM routine to vector to correct kernel/VB
    ; the 'rts' from jumpCodeRAM will actually return to THIS routine's caller

                	lda kernelBank_L,x
                	sta SM_JumpBank_L

	                lda kernelRoutine_L,x
	                sta SM_JumpRoutine_L
	                lda kernelRoutine_H,x
	                sta SM_JumpRoutine_H

	                jmp jumpCodeRAM

;-------------------------------------------------------------------------------

    include "saveKey.asm"
    include "kernel01.asm"


    CHECK_OVERFLOW 0, $FF0

CURRENT_ORG SET CURRENT_ORG + $1000

; EOF
