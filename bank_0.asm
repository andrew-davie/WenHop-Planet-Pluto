	org CURRENT_BANK
	rorg $f000

.BANK0

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
; .blank_scanlines
; 	sta WSYNC
; 	dex
; 	bne .blank_scanlines			;can use .blank_scanlines in any bank
; 	rts

; .blank_scanlines_aud				;can use .blank_scanlines_aud in any bank
; 	sta WSYNC
; 	lda #AMPLITUDE
; 	sta AUDV0
; 	dex
; 	bne .blank_scanlines_aud
; 	rts



;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@ Code block that gets copied into RAM for bank routine dispatch @@@@@@@@@@@@@@@@@

jumpCode       	cmp BANK1
                jsr jumpCode
                cmp BANK0
jumpCodeEnd
                rts

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Cart Reset @@@@@@@@@@@@@@@@@@@@@@@@@@@@

CartReset

                    CLEAN_START

    ; Copy jump dispatch handler to RAM

	                ldx #(jumpCodeEnd-jumpCode)
initJumpCode        lda jumpCode,x
	                sta .jump_code_RAM,x
	                dex
	                bpl initJumpCode


                    jsr ReadSaveKey             ; Load savekey and send to ARM/C

                    ldx #FASTON
                    stx SETMODE			            ; "Fast Fetch" enable

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
 
    ; Let the ARM do SK inits too

                    ; ldx #>_RUN_FUNC
                    ; stx DSPTR
                    ; ldx #<_RUN_FUNC
                    ; stx DSPTR
                    ; ldx #_FN_LOAD_SAVEKEY
                    ; stx DSWRITE

                    ; ldx #$FF
	                ; stx CALLFN          			; Initialise via ARM function

    ; Setup and call ARM Initialise

                    ldx #>_RUN_FUNC
                    stx DSPTR
                    ldx #<_RUN_FUNC
                    stx DSPTR
                    ldx #_RUN_ARM_INIT              ; routine to run in main()
                    stx DSWRITE
                    ; lda saveKey_detected
                    ; sta DSWRITE			            ; (== _SWCHA)

                    ldx #$FF
                    stx CALLFN          			; Initialise via ARM function

    ; At this point, the ARM function handler runs the Initialise function
    ; Retrive critical configuration variables from ARM

                lda #DS31DATA
                sta kernel
                lda #DS31DATA
                sta tv_system
                lda #DS31DATA
                sta sound_mode
                sta sound_save

                tax
                lda .sound_mode_table,x
                sta SETMODE         			; set proper sound mode [???]
                lda .call_fn_table,x
                sta call_fn			            ; apply proper function caller


	; lda #80				;only for demonstration of positioning method purposes
	; sta test_position		;

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Game Loop @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

mainGameLoop

    ; [???] what is this SETMODE stuff for sound?!!

                lda sound_mode			        ; compare this frame's sound mode
                cmp sound_save			        ; to last frame's
                beq skipChangeModes
                sta sound_save			        ; apply new mode if change
                tax				                ; is detected
                lda .sound_mode_table,x
                sta SETMODE
                lda .call_fn_table,x
                sta call_fn
skipChangeModes tax				                ; branch to proper frame handler
                bne mainGameLoopSampled	        ; standard TIA or sampled sound


mainGameLoopStandard

;@@@@@@@@@@@@@@@@@@@@
	lda #%1110
.vertsync_std
	sta WSYNC
	sta VSYNC
	lsr 
	bne .vertsync_std
	
    ldx tv_system
    lda UpperBlankTimer,x
	sta TIM64T
;@@@@@@@@@@@@@@@@@@@@

	jsr verticalBlank

;@@@@@@@@@@@@@@@@@@@@
.wait_vblank_end_std
	sta WSYNC
	lda INTIM
	bne .wait_vblank_end_std
	sta VBLANK
;@@@@@@@@@@@@@@@@@@@@

	ldx kernel
	jsr .call_bank_routine

;@@@@@@@@@@@@@@@@@@@@
	lda #2
	sta WSYNC
	sta VBLANK

    ldx tv_system
    lda LowerBlankTimer,x
	sta TIM64T
;@@@@@@@@@@@@@@@@@@@@

	jsr .lower_vblank_ARM

	lda audc0
	sta AUDC0
	lda audf0
	sta AUDF0
	lda audv0
	sta AUDV0
	lda audc1
	sta AUDC1
	lda audf1
	sta AUDF1
	lda audv1
	sta AUDV1

;@@@@@@@@@@@@@@@@@@@@
.wait_overscan_std
	sta WSYNC
	lda INTIM	
	bne .wait_overscan_std
;@@@@@@@@@@@@@@@@@@@@

	jmp mainGameLoop

LowerBlankTimer
 .byte LOWER_BLANK_TIMER_PAL60
 .byte LOWER_BLANK_TIMER_NTSC
 .byte LOWER_BLANK_TIMER_SECAM

UpperBlankTimer
 .byte UPPER_BLANK_TIMER_PAL60
 .byte UPPER_BLANK_TIMER_NTSC
 .byte UPPER_BLANK_TIMER_SECAM


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Game Loop - Sampled Sounds @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

mainGameLoopSampled

;@@@@@@@@@@@@@@@@@@@@
	ldx #2
	ldy #3
.vertsync_samp
	stx WSYNC
	stx VSYNC
	lda #AMPLITUDE
	sta AUDV0
	dey
	bne .vertsync_samp
	sty WSYNC
	sty VSYNC
	lda #AMPLITUDE
	sta AUDV0
	
    ldx tv_system
    lda UpperBlankTimer,x
	sta TIM64T
;@@@@@@@@@@@@@@@@@@@@

	jsr verticalBlank

;@@@@@@@@@@@@@@@@@@@@
.wait_vblank_end_samp
	sta WSYNC
	lda #AMPLITUDE
	sta AUDV0
	lda INTIM
	bne .wait_vblank_end_samp
	sta VBLANK
;@@@@@@@@@@@@@@@@@@@@

	ldx kernel
	jsr .call_bank_routine

;@@@@@@@@@@@@@@@@@@@@
	ldx #2
	stx WSYNC
	stx VBLANK
	lda #AMPLITUDE
	sta AUDV0

    ldx tv_system
    lda LowerBlankTimer,x
	sta TIM64T
;@@@@@@@@@@@@@@@@@@@@

	jsr .lower_vblank_ARM

	lda audc1
	sta AUDC1
	lda audf1
	sta AUDF1
	lda audv1
	sta AUDV1

;@@@@@@@@@@@@@@@@@@@@
.wait_overscan_samp
	sta WSYNC
	lda #AMPLITUDE
	sta AUDV0
	lda INTIM	
	bne .wait_overscan_samp
;@@@@@@@@@@@@@@@@@@@@

	jmp mainGameLoop




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



;@@@@@@@@@@@@@@@@@@@@@@@@ Handle Vertical Blank ARM+6502 @@@@@@@@@@@@@@@@@@@@@@@@

verticalBlank
                ldx #>_RUN_FUNC
                stx DSPTR
                ldx #<_RUN_FUNC
                stx DSPTR
                ldx #_RUN_ARM_VBLANK
                stx DSWRITE
                ldx call_fn
	            stx CALLFN                  ; call VerticalBlank in ARM

    ; Vector to appropriate 6502 Vertical Blank code
    ; Currently this is just position sprites, and sound stuff
    ; in other words, basic inits

	            ldx kernel
                lda kernelVBlank_h,x
                pha
                lda kernelVBlank_l,x
                pha
                rts


kernelVBlank_h  .byte >(k_prep_00-1)
                .byte >(k_prep_01-1)

kernelVBlank_l	.byte <(k_prep_00-1)
	            .byte <(k_prep_01-1)





;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@ Handle Lower VBlank ARM Call @@@@@@@@@@@@@@@@@@@@@@@@
.lower_vblank_ARM

	ldx #0
	stx COLUBK			;clear color registers
    stx COLUP0
	stx COLUP1
	stx COLUPF

	ldx #>_RUN_FUNC
	stx DSPTR
	ldx #<_RUN_FUNC
	stx DSPTR
    
	ldx #_RUN_ARM_LOWER_VBLANK		;let ARM know we are lower VBlank
	stx DSWRITE

	ldx SWCHA
	stx DSWRITE
	ldx SWCHB
	stx DSWRITE
	ldx INPT4
	stx DSWRITE
	ldx INPT5
	stx DSWRITE			;all Atari inputs to ARM

	ldx call_fn
	stx CALLFN

	lda #DS31DATA
	sta kernel			;kernel and sound_mode get refreshed
	lda #DS31DATA
	sta tv_system
	lda #DS31DATA			;from the ARM each frame
	sta sound_mode
	lda #DS31DATA
	sta audv0
	lda #DS31DATA
	sta audc0
	lda #DS31DATA
	sta audf0
	lda #DS31DATA
	sta audv1
	lda #DS31DATA
	sta audc1
	lda #DS31DATA
	sta audf1

	; lda sk_command			;<SaveKey> block of code
	; bmi .write_to_save_key		;<SaveKey> preforms SaveKey operations one byte at a time each frame
	; bne .read_from_save_key		;<SaveKey> to maintain the screen, sound will be slightly distorted

	; lda #DS31DATA			;test for new save key operation
	; beq .skip_save_key_operation
	; sta sk_command
	; tay
	; lda #DS31DATA
	; beq .acknowledge_save_command	;bail on count = 0
	; cmp #65
	; bcs .acknowledge_save_command	;bail on count > 64
	; sta sk_count
	; lda #DS31DATA
	; sta sk_addr_l
	; lda #DS31DATA
	; sta sk_addr_h
	; lda #DS31DATA
	; and #63				;wrap offset into 0-63 sk_RAM buffer range
	; sta sk_offset
	; clc
	; adc sk_count
	; cmp #65
	; bcs .acknowledge_save_command	;bail on access outside sk_RAM
	; tya
	; bpl .read_from_save_key

; 	ldx #0
; .loop_DD_to_sk_RAM			;load sk_RAM buffer with data from ARM
; 	lda #DS31DATA
; 	sta sk_RAM,x
; 	inx
; 	cpx #64
; 	bne .loop_DD_to_sk_RAM
; 	beq .skip_save_key_operation

; .read_from_save_key
; 	jsr .read_save_key
; 	ldx #>_save_data
; 	stx DSPTR
; 	lda #<_save_data		;reads can update DD ARM after each byte
; 	clc
; 	adc sk_offset
; 	sta DSPTR
; 	ldx sk_offset
; 	lda sk_RAM,x
; 	sta DSWRITE
; 	jmp .done_savekey_operation

; .write_to_save_key
; 	jsr .write_save_key

; .done_savekey_operation
; 	inc sk_offset
; 	inc sk_addr_l
; 	dec sk_count
; 	bne .skip_save_key_operation
; .acknowledge_save_command
; 	ldx #0
; 	stx sk_command
; 	ldx #>_save_command		;X = 0 here, using this for DSWRITE
; 	stx DSPTR			;<SaveKey>
; 	ldy #<_save_command		;<SaveKey>
; 	sty DSPTR			;<SaveKey>
; 	stx DSWRITE			;<SaveKey> block of code

; .skip_save_key_operation
	rts
;@@@@@@@@@@@@@@@@@@@@@@@@ Handle Lower VBlank ARM Call @@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@ Cross Bank Routine Handler @@@@@@@@@@@@@@@@@@@@@@@@@
.call_bank_routine
	lda .jump_table_target_bank,x
	sta .jump_code_RAM_t_bank
.call_bank_routine_sans_bank
	lda .jump_table_target_routine_l,x
	sta .jump_code_RAM_t_r_l
	lda .jump_table_target_routine_h,x
	sta .jump_code_RAM_t_r_h
	jmp .jump_code_RAM
;@@@@@@@@@@@@@@@@@@@@

.jump_table_target_bank			;kernel routines can be located anywhere on any bank
	.byte #<BANK1
	.byte #<BANK1

.jump_table_target_routine_l		;each routine gets an entry in the table set
	.byte #<kernel_00
	.byte #<kernel_01

.jump_table_target_routine_h
ROUTINE_KERNEL_00 = * - .jump_table_target_routine_h	;use this method to create names for manual routine calling
	.byte #>kernel_00
	.byte #>kernel_01

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


	org CURRENT_BANK+$0700			;org usage within each bank remains straightforward - use CURRENT_BANK + desired offset
	rorg CURRENT_BANK+$0700			;samples need actual position within ROM, so rorg = org for those


_sample_steel
	INCBIN "sd2.bin"
_sample_steel_size = * - _sample_steel


	org CURRENT_BANK+$0e00
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

    ; pre-cleer GridLock flags buffers
    ; will exit with SAVEKEY_IDENT = _SK_GRIDLOCK_ID if valid

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


k_prep_00
k_prep_xx	;[example] kernels can share prep code when setup is identical but display handling differs


                    jsr scanAndUpdateSaveKey

	;each kernel also dispatches a "prep" routine to allow for object pre-positioning

                    sta WSYNC			        ;       direct table position method
                    nop				            ; 2		load index with desired position 0-159

                    ldx #72				        ;2,4	can be immediate value or from a datastream
                    lda .calc_rpfp_table,x		;4,8	load data from .calc_rpfp_table
                    sta HMP0			        ;3,11	apply to HMxx register
                    and #$0f			        ;2,13	mask low nybble
                    tax				            ;2,15	to index register
.positionP0         dex				            ;2,17	dex register for loop
                    bpl .positionP0		        ;2,19	on negative fall through
                    sta RESP0			        ;3,22  	on smallest loop - RESxx register must fall on cycle 22

                    sta WSYNC

                    ldx test_position		    ;3		when in need to use a variable
                    lda .calc_rpfp_table,x		;4,7
                    sta HMP1			        ;3,10
                    and #$0f			        ;2,12
                    tax				            ;2,14
.positionP1         dex				            ;2,16
                    bpl .positionP1		        ;2,18
                    sta.w RESP1			        ;4,22	use sta.w to use that extra cycle

                    sta WSYNC
                    sta HMOVE

                    sta WSYNC
                    sta HMCLR

                    rts


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@ Kernel 01 Prep @@@@@@@@@@@@@@@@@@@@@@@@@
k_prep_01

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


	; org CURRENT_BANK+$0f00
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







BANK0_CODE_SIZE = * - .BANK0
	echo "---- BANK0", BANK0_CODE_SIZE, "bytes"
	echo "---- BANK0", ($fff0 - *), "bytes free"


;@@@@@@@@@@@@@@@ Bank 0 Footer - needed for CDFJ+ to function @@@@@@@@@@@@@@@
	; ROM Pointers
	org $17F0		;This section is only needed in BANK 0
	rorg $FFF0
	DC.B 0, 0, 0, 0		;CDFJ Hotspots
	DC.L C_STACK		;$F4	C Stack
	DC.L C_CODE+1		;$F8	C Code (+1 for THUMB Mode)
	DC.W CartReset		;$FC	Reset
	DC.W CartReset		;$FE	BRK


