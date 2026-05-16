
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
∫

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

    include "mainLoop.asm"


; EOF
