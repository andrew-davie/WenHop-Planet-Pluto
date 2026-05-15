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

mainGameLoop

                	lda #2
                    sta WSYNC
                    sta VBLANK                  ; overscan!

                    ldx tvSystem
                    lda TimerOS,x
                    sta TIM64T

    ; call ARM Overscan

                	ldx #>_RUN_FUNC
                	stx DSPTR
                	ldx #<_RUN_FUNC
                	stx DSPTR

                	ldx #_RUN_ARM_OVERSCAN
                	stx DSWRITE

    ; send controllers to ARM

                	ldx SWCHA
                	stx DSWRITE
                	ldx SWCHB
                	stx DSWRITE
                	ldx INPT4
                	stx DSWRITE
                	ldx INPT5
                	stx DSWRITE			    ; all Atari inputs to ARM

                	ldx call_fn
                	stx CALLFN              ; --> runARM_Overscan()

    ; retrieve ARM 'system' variables

                    lda #DS31DATA
                    sta kernel
                    lda #DS31DATA
                    sta tvSystem
                    lda #DS31DATA
                    sta soundMode
                    lda #DS31DATA
                    sta COLUBK

                    lda #DS31DATA
                    sta AUDV0
                    lda #DS31DATA
                    sta AUDV1
                    lda #DS31DATA
                    sta AUDC0
                    lda #DS31DATA
                    sta AUDC1
                    lda #DS31DATA
                    sta AUDF0
                    lda #DS31DATA
                    sta AUDF1

    ; run kernel-specific 6502 Overscan code (OS_FN_OFFSET) 

                    lda kernel
                    clc
                    adc #OS_FN_OFFSET
                    tax
                    jsr runVectoredCode               ; actually run 6502 kernel-specific VB

.waitOS             lda INTIM	
                    bne .waitOS

    ; vertical blank
    ; any kernel switching will have just happened

                    ldx #%1110
                    txa
.verticalSync       sta WSYNC
                    sta VSYNC                   ; indicate vblank
                    lsr 
                    bne .verticalSync 


                    ldx tvSystem
                    lda TimerVB,x
                    sta TIM64T

    ; call ARM Vertical Blank

                    ldx #>_RUN_FUNC
                    stx DSPTR
                    ldx #<_RUN_FUNC
                    stx DSPTR
                    ldx #_RUN_ARM_VBLANK
                    stx DSWRITE

                    ldx call_fn
                    stx CALLFN                  ; call VerticalBlank in ARM

    ; run kernel-specific 6502 Vertical Blank code (VB_FN_OFFSET)

                    lda kernel
                    clc
                    adc #VB_FN_OFFSET
                    tax
                    jsr runVectoredCode               ; actually run 6502 kernel-specific VB

.waitVB             lda INTIM
                    bne .waitVB

                    sta WSYNC
                    sta VBLANK                  ; screen on

                    ldx kernel
                    jsr runVectoredCode               ; run 6502 kernel

                    jmp mainGameLoop

;-------------------------------------------------------------------------------

TimerOS             .byte 36           ; NTSC
                    .byte 46           ; PAL
                    .byte 36           ; SECAM
                    .byte (36+29)      ; PAL60

TimerVB
                    .byte 43           ; NTSC
                    .byte 50           ; PAL
                    .byte 43           ; SECAM
                    .byte (43+30)      ; PAL60




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


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@ Cross Bank Routine Handler @@@@@@@@@@@@@@@@@@@@@@@@@

VB_FN_OFFSET = ((.END-.START)/3)
OS_FN_OFFSET = (VB_FN_OFFSET * 2)

; When adding entries, add a 'kernel;, a 'VB', and a 'OS' entry
; the offset (above) auto-calculates


; equates for ARM usage

_KERNEL_RAINBOW     = 0
_KERNEL_01          = 1
_KERNEL_COPYRIGHT   = 2


kernelBank_L
.START
                    ; >>> kernel
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernel_01
                    .byte <BANK_kernelCopyright

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

    ; we sneakily self-modify the in-RAM routine to vector to correct kernel/VB
    ; the 'rts' from jumpCodeRAM will actually return to THIS routine's caller

                	lda kernelBank_L,x
                	sta SM_JumpBank_L

	                lda kernelRoutine_L,x
	                sta SM_JumpRoutine_L
	                lda kernelRoutine_H,x
	                sta SM_JumpRoutine_H

	                jmp jumpCodeRAM

;-------------------------------------------------------------------------------


;@@@@@@@@@@@@@@@@@@@@@@@@@ Cross Bank Routine Handler @@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@ Write Save Key Routine @@@@@@@@@@@@@@@@@@@@@@
;@ 1 byte 1310 cycles (17+ scanlines) including call
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
; .write_save_key
; 	jsr .setup_save_key
; 	bcc .noSKfound
; 	ldx #1
; 	stx saveKey_detected
; 	ldx sk_offset
; 	lda sk_RAM,x
; 	jsr .i2c_txbyte
; 	jsr .i2c_stopwrite
; 	rts
; .noSKfound
; 	ldx #0
; 	stx saveKey_detected
; .done_save_key
; 	rts
;@@@@@@@@@@@@@@@@@@@@ Write Save Key Routine @@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@ Read Save Key Routine @@@@@@@@@@@@@@@@@@@@@
;@ 1 byte 1736 cycles (23 scanlines) including call
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
; .read_save_key
; 	jsr .setup_save_key
; 	bcc .noSKfound
; 	ldx #1
; 	stx saveKey_detected
; 	jsr .i2c_stopwrite
; 	jsr .i2c_startread
; 	ldx sk_offset
; 	jsr .i2c_rxbyte
; 	sta sk_RAM,x
; 	jsr .i2c_stopread
; 	rts
;@@@@@@@@@@@@@@@@@@@@@@ Read Save Key Routine @@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@ .SetupSaveKey Routine @@@@@@@@@@@@@@@@@@@@@@
; .setup_save_key
; 	jsr .i2c_startwrite
; 	bne .exitSK
; 	clv
; 	lda sk_addr_h
; 	jsr .i2c_txbyte
; 	lda sk_addr_l
; 	jmp .i2c_txbyte
; .exitSK
;  	clc
;  	rts
;@@@@@@@@@@@@@@@@@@@@@ .SetupSaveKey Routine @@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@ .i2c Routines @@@@@@@@@@@@@@@@@@@@@@@@@@@
; .i2c_startread
; 	ldy #%10100001
; 	.byte $2c
; .i2c_startwrite
; 	ldy #%10100000
; 	lda #24
; 	sta SWCHA
; 	lsr
; 	sta SWACNT
; 	tya
; .i2c_txbyte
; 	eor #$ff
; 	sec
; 	rol
; .i2c_txbyteloop
; 	tay
; 	lda #$0
; 	sta SWCHA
; 	adc #2
; 	asl
; 	asl
; 	sta SWACNT
; 	lda #8
; 	sta SWCHA
; 	tya
; 	asl
; 	bne .i2c_txbyteloop
; 	beq .i2c_rxbit
; .i2c_rxbyte
; 	bvc .i2c_rxskipack
; 	jsr .i2c_txack
; .i2c_rxskipack
; 	bit .i2c_rxbyte
; 	lda #1
; .i2c_rxbyteloop
; 	tay
; .i2c_rxbit
; 	lda #16
; 	sta SWCHA
; 	lsr
; 	sta SWACNT
; 	nop
; 	sta SWCHA
; 	lda SWCHA 
; 	lsr
; 	lsr
; 	lsr
; 	tya
; 	rol
; 	bcc .i2c_rxbyteloop
; 	rts

; .i2c_stopread
; 	bvc .i2c_stopwrite
; 	ldy #$80
; 	jsr .i2c_rxbit
; .i2c_stopwrite
; 	jsr .i2c_txack
; 	lda #0
; 	sta SWACNT
; 	rts

; .i2c_txack
; 	lda #0
; 	sta SWCHA
; 	lda #12
; 	sta SWACNT
; 	asl
; 	sta SWCHA
; 	rts
;@@@@@@@@@@@@@@@@@@@@@@@@ .i2c Routines @@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


	org CURRENT_ORG + $0700			;org usage within each bank remains straightforward - use CURRENT_ + desired offset
	rorg CURRENT_ORG + $0700			;samples need actual position within ROM, so rorg = org for those


_sample_steel
	INCBIN "sd2.bin"
_sample_steel_size = * - _sample_steel


	org CURRENT_ORG + $0e00
	rorg $f000+$0e00			;rorg also straightforward - use $f000 + desired offset


;███████████████████████████████████████████████████████████████████████████████


skBuffer    = SAVEKEY_IDENT      ; define the RAM address you want to store
SK_BYTES    = (SK_END - SK_START)
;3 * SAVEKEY_SIZE + 1               ; define how many bytes your want to store
SAVEKEY_ADR = $18c0             ; slot 99?
 
    include "i2c_v2.3.inc"      ; a highly optimized (for space) version   
    i2c_subs                    ; this makes the i2c macros of the include file known to the code 

;-------------------------------------------------------------------------------
WriteSaveKey SUBROUTINE         ; total cycles = 1923 (for 3 bytes)
;-------------------------------------------------------------------------------
; setup SaveKey:
    jsr     SetupSaveKey        ; 6+927
    bcc     .noSKfound          ; 2/3
 
; write high score:
    ldx     #0         ; 2 = 937   
.loopWriteSK
    lda     skBuffer,x          ; 4
    jsr     i2c_txbyte          ;6+296      transmit to EEPROM
    inx                         ; 2
    cpx #SK_BYTES
    bcc     .loopWriteSK        ; 2/3=932

; stop write:
    jsr     i2c_stopwrite       ; 6+42=48   terminate write and commit to memory
.noSKfound
    rts                         ; 6

;-------------------------------------------------------------------------------
ReadSaveKey SUBROUTINE          ; total cycles = 2440 (for 3 bytes)
;-------------------------------------------------------------------------------
; setup SaveKey:


    ; pre-cleer flags buffers
    ; will exit with SAVEKEY_IDENT = _SK_WENHOP_ID if valid

_WENHOP_SK_ID = 0xA7

                    ldx #SK_BYTES               ; (+1) to include _SAVEKEY_RESET
                    lda #0
.clearFlagsKey      sta SAVEKEY_IDENT,x
                    dex
                    bpl .clearFlagsKey

    ; Note: _SAVEKEY_IDENT = 0 at this point
    ; Now unlock the first 10 levels
    
                    lda #%11111111
                    sta SAVEKEY_UNLOCKED
                    lda #%11
                    sta SAVEKEY_UNLOCKED+1

    ; non-zero inits go here. The rest are already 0

                    lda #1
                    sta SAVEKEY_TOTAL_SOLVES
                    sta SAVEKEY_ENABLE_ICC

.verify             jsr SetupSaveKey
                    bcc .noSKfound

                    jsr i2c_stopwrite
                    jsr i2c_startread

                    ldx #_WENHOP_SK_ID

                    jsr i2c_rxbyte
                    cmp #_WENHOP_SK_ID
                    bne .forceInitSK

        ; HANDLE RESET PRESS TO CLEAR SAVEKEY DATA

                    lda SWCHB                   ; clear
                    lsr                         ; reset -> c
                    bcs .verifiedGridLock

                    ; ldx #$26
                    ; stx COLUBK

.forceInitSK        inc SAVEKEY_RESET

                    ldx #_WENHOP_SK_ID
                    stx SAVEKEY_IDENT

                    jsr i2c_stopread
                                        
                    jsr WriteSaveKey
                    jsr i2c_stopwrite
                    rts ;jmp .verify


.verifiedGridLock

                    stx SAVEKEY_IDENT

                    ldx #1
.loopReadSK         jsr i2c_rxbyte
                    sta SAVEKEY_IDENT,x
                    inx
                    cpx #SK_BYTES
                    bcc .loopReadSK
 
.noSKfound          jsr i2c_stopread
 
    ; SK present IFF SAVEKEY_IDENT == _WENHOP_SK_ID
    ; 0 if not present

                    rts

;------------------------------------------------------------------------------
SetupSaveKey SUBROUTINE         ; = 927
;------------------------------------------------------------------------------

                    ; lda #0
                    ; sta scanSK


; detect SaveKey:
    jsr     i2c_startwrite      ;6+312
    bne     .exitSK             ; 2/3
; setup address:
    clv                         ; 2
    lda     #>SAVEKEY_ADR       ; 2         upper byte of address
    jsr     i2c_txbyte          ;6+296
    lda     #<SAVEKEY_ADR       ; 2         lower byte offset
    jmp     i2c_txbyte          ;3+296      returns C==1

.exitSK
    clc
    rts

; 176 bytes in total (less if you inline the subroutines)


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

scanAndUpdateSaveKey

    ; Does a diff on one SK byte per call (i.e., once per frame is ideal)
    ; Will write to SK if change detected

                    ldx scanSK
                    inx
                    cpx #SK_BYTES
                    bcc inRange
                    ldx #0
inRange             stx scanSK

                    ldy #>_SK_ID
                    sty DSPTR
                    clc
                    txa
                    adc #<_SK_ID
                    sta DSPTR

                    lda #DSCOMM
                    cmp SAVEKEY_IDENT,x
                    bne writeSKandExit

                    ; inx
                    ; cpx #SK_BYTES
                    ; bcc scanExit
                    ; ldx #0
;scanExit            stx scanSK
                    rts


writeSKandExit      sta SAVEKEY_IDENT,x

                    ldx #FASTOFF
                    stx SETMODE


                    jsr i2c_startwrite
                    bne .noSKfound

                    clv
                    lda #>SAVEKEY_ADR
                    jsr i2c_txbyte

                    clc
                    lda scanSK
                    adc #<SAVEKEY_ADR
                    jsr i2c_txbyte

                    ldx scanSK
                    lda SAVEKEY_IDENT,x
                    jsr i2c_txbyte
                    jsr i2c_stopwrite

.noSKfound  
                    ldx #FASTON
                    stx SETMODE

                    rts                    


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@ Kernel 00 Prep @@@@@@@@@@@@@@@@@@@@@@@@@
; Let's make this the gamekernel

; OS_kernel_00        rts

; VB_kernel_00


;                     jsr scanAndUpdateSaveKey

; 	;each kernel also dispatches a "prep" routine to allow for object pre-positioning

;                     sta WSYNC			        ;       direct table position method
;                     nop				            ; 2		load index with desired position 0-159

;                     ldx #72				        ;2,4	can be immediate value or from a datastream
;                     lda .calc_rpfp_table,x		;4,8	load data from .calc_rpfp_table
;                     sta HMP0			        ;3,11	apply to HMxx register
;                     and #$0f			        ;2,13	mask low nybble
;                     tax				            ;2,15	to index register
; .positionP0         dex				            ;2,17	dex register for loop
;                     bpl .positionP0		        ;2,19	on negative fall through
;                     sta RESP0			        ;3,22  	on smallest loop - RESxx register must fall on cycle 22

;                     sta WSYNC

;                     ldx test_position		    ;3		when in need to use a variable
;                     lda .calc_rpfp_table,x		;4,7
;                     sta HMP1			        ;3,10
;                     and #$0f			        ;2,12
;                     tax				            ;2,14
; .positionP1         dex				            ;2,16
;                     bpl .positionP1		        ;2,18
;                     sta.w RESP1			        ;4,22	use sta.w to use that extra cycle

;                     sta WSYNC
;                     sta HMOVE

;                     sta WSYNC
;                     sta HMCLR

;                     rts


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@ Kernel 01 Prep @@@@@@@@@@@@@@@@@@@@@@@@@

OS_kernel_01        rts


VB_kernel_01

	ldx test_position		;3		position in index and
	lda .calc_rpfp_table,x		;4		table read must come before the WSYNC

	sta WSYNC
	sta HMP1			;3
	and #$0f			;2,5
	tax				;2,7
	lda #AMPLITUDE			;2,9		positioning method while using
	sta AUDV0			;3,12		wave sound
	sta $2d				;3,15
.loop_position_p1_2			;
	dex				;2,17
	bpl .loop_position_p1_2		;2,19
	sta RESP1			;3,22

	sta WSYNC
	sta HMOVE
	lda #AMPLITUDE
	sta AUDV0

	sta WSYNC
	sta HMCLR
	lda #AMPLITUDE
	sta AUDV0

	rts



;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


	; org CURRENT_ORG+$0f00
	; rorg $f000+$0f00


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@ Positioning Table @@@@@@@@@@@@@@@@@@@@@@@@

.calc_rpfp_table		;RoughPosition/FinePosition table for position method [22]
	; .byte %00110000
	; .byte %00100000
	; .byte %00010000
	; .byte %00000000
	; .byte %11110000
	; .byte %11100000
	; .byte %11010000
	; .byte %11000000
	; .byte %10110000
	; .byte %10100000
	; .byte %10010000

	; .byte %01110001
	; .byte %01100001
	; .byte %01010001
	; .byte %01000001
	; .byte %00110001
	; .byte %00100001
	; .byte %00010001
	; .byte %00000001
	; .byte %11110001
	; .byte %11100001
	; .byte %11010001
	; .byte %11000001
	; .byte %10110001
	; .byte %10100001
	; .byte %10010001

	; .byte %01110010
	; .byte %01100010
	; .byte %01010010
	; .byte %01000010
	; .byte %00110010
	; .byte %00100010
	; .byte %00010010
	; .byte %00000010
	; .byte %11110010
	; .byte %11100010
	; .byte %11010010
	; .byte %11000010
	; .byte %10110010
	; .byte %10100010
	; .byte %10010010

	; .byte %01110011
	; .byte %01100011
	; .byte %01010011
	; .byte %01000011
	; .byte %00110011
	; .byte %00100011
	; .byte %00010011
	; .byte %00000011
	; .byte %11110011
	; .byte %11100011
	; .byte %11010011
	; .byte %11000011
	; .byte %10110011
	; .byte %10100011
	; .byte %10010011

	; .byte %01110100
	; .byte %01100100
	; .byte %01010100
	; .byte %01000100
	; .byte %00110100
	; .byte %00100100
	; .byte %00010100
	; .byte %00000100
	; .byte %11110100
	; .byte %11100100
	; .byte %11010100
	; .byte %11000100
	; .byte %10110100
	; .byte %10100100
	; .byte %10010100

	; .byte %01110101
	; .byte %01100101
	; .byte %01010101
	; .byte %01000101
	; .byte %00110101
	; .byte %00100101
	; .byte %00010101
	; .byte %00000101
	; .byte %11110101
	; .byte %11100101
	; .byte %11010101
	; .byte %11000101
	; .byte %10110101
	; .byte %10100101
	; .byte %10010101

	; .byte %01110110
	; .byte %01100110
	; .byte %01010110
	; .byte %01000110
	; .byte %00110110
	; .byte %00100110
	; .byte %00010110
	; .byte %00000110
	; .byte %11110110
	; .byte %11100110
	; .byte %11010110
	; .byte %11000110
	; .byte %10110110
	; .byte %10100110
	; .byte %10010110

	; .byte %01110111
	; .byte %01100111
	; .byte %01010111
	; .byte %01000111
	; .byte %00110111
	; .byte %00100111
	; .byte %00010111
	; .byte %00000111
	; .byte %11110111
	; .byte %11100111
	; .byte %11010111
	; .byte %11000111
	; .byte %10110111
	; .byte %10100111
	; .byte %10010111

	; .byte %01111000
	; .byte %01101000
	; .byte %01011000
	; .byte %01001000
	; .byte %00111000
	; .byte %00101000
	; .byte %00011000
	; .byte %00001000
	; .byte %11111000
	; .byte %11101000
	; .byte %11011000
	; .byte %11001000
	; .byte %10111000
	; .byte %10101000
	; .byte %10011000

	; .byte %01111001
	; .byte %01101001
	; .byte %01011001
	; .byte %01001001
	; .byte %00111001
	; .byte %00101001
	; .byte %00011001
	; .byte %00001001
	; .byte %11111001
	; .byte %11101001
	; .byte %11011001
	; .byte %11001001
	; .byte %10111001
	; .byte %10101001
	; .byte %10011001

	; .byte %01111010
	; .byte %01101010
	; .byte %01011010
	; .byte %01001010
	; .byte %00111010
	; .byte %00101010
	; .byte %00011010
	; .byte %00001010
	; .byte %11111010
	; .byte %11101010
	; .byte %11011010
	; .byte %11001010
	; .byte %10111010
	; .byte %10101010
	; .byte %10011010



    CHECK_OVERFLOW 0, $FF0

CURRENT_ORG SET CURRENT_ORG + $1000
