	org CURRENT_BANK
	rorg $f000

.BANK3

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
.blank_scanlines_3
	sta WSYNC
	dex
	bne .blank_scanlines_3			;can use .blank_scanlines in any bank
	rts

.blank_scanlines_aud_3				;can use .blank_scanlines_aud in any bank
	sta WSYNC
	lda #AMPLITUDE
	sta AUDV0
	dex
	bne .blank_scanlines_aud_3
	rts
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@







BANK3_CODE_SIZE = * - .BANK3;
	echo "---- BANK3", BANK3_CODE_SIZE, "bytes"
	echo "---- BANK3", ($fff0 - *), "bytes free"







