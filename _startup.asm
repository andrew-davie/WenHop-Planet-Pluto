CartReset

                    CLEAN_START

                    ldx #FASTON
                    stx SETMODE

    ; Copy jump dispatch handler to RAM

	                ldx #(JUMP_CODE_END - JUMP_CODE_START) - 1
initJumpCode        lda jumpCode,x
	                sta jumpCodeRAM,x
	                dex
	                bpl initJumpCode


    ; general system reset on ARM
    ; before SaveKey load, so memory clears work!

                    ldx #>_RUN_FUNC
                    stx DSPTR
                    ldx #<_RUN_FUNC
                    stx DSPTR
                    ldx #_RUNARM_SYSTEM_RESET
                    stx DSWRITE

                    ldx #$FF
                    jsr callARM          		    ; Initialise via ARM function

                    jsr ReadSaveKey                 ; Load savekey to ZP SAVEKEY block (*fast mode OFF)

    ; Send the SAVEKEY block to ARM-accessible shadow variables and then to ARM
    ; A diff is (later) used to determine any changes that need to be written

                    ldx #>_SK_START
                    stx DSPTR
                    ldx #<_SK_START
                    stx DSPTR

                    ldx #0
.sendSK             lda SK_ID,x
                    sta DSWRITE
                    inx
                    cpx #SK_BYTES + 1
                    bcc .sendSK


                    ldx #>_RUN_FUNC
                    stx DSPTR
                    ldx #<_RUN_FUNC
                    stx DSPTR
                    ldx #_RUNARM_LOAD_SAVEKEY
                    stx DSWRITE

                    ldx #$FF
                    jsr callARM   		    ; Initialise via ARM function

.notRealSKData

    ; At this point, the ARM function handler _RUNARM_SYSTEM_RESET has set some vars
    ; Retrive critical configuration variables from ARM


                    lda #DS31DATA
                    sta kernel
                    lda #DS31DATA
                    sta tvSystem                    ; <--- == NTSC

    ; TODO: sound stuff yet to be fully convered

                    lda #DS31DATA
                    sta soundMode
                    sta soundSave

                    tax
                    lda .sound_mode_table,x
                    sta SETMODE         			    ; set proper sound mode [???]
                    lda .call_fn_table,x
                    sta call_fn			                ; apply proper function caller

    include "_mainLoop.asm"


;-------------------------------------------------------------------------------
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
; Code block that gets copied into RAM for bank routine dispatch

; The code here is a 'template' copied into the RAM area and the bank and jump
; address are modified before execution. The return will be to the caller of
; the code that jumps to the RAM copy of this code.
; DO NOT RUN *THIS* CODE AS IT WILL CRASH. JUMP TO THE RAM VERSION!


JUMP_CODE_START
jumpCode       	    cmp BANK1           ; hotspot bank-switch
                    jsr 0               ; selfmod
                    cmp BANK0           ; hotspot bank-switch
                    rts
JUMP_CODE_END


;-------------------------------------------------------------------------------
; EOF
