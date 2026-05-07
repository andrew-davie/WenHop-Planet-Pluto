	org CURRENT_BANK
	rorg $f000

.BANK5

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
.blank_scanlines_5
	sta WSYNC
	dex
	bne .blank_scanlines_5			;can use .blank_scanlines in any bank
	rts

.blank_scanlines_aud_5				;can use .blank_scanlines_aud in any bank
	sta WSYNC
	lda #AMPLITUDE
	sta AUDV0
	dex
	bne .blank_scanlines_aud_5
	rts
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@







BANK5_CODE_SIZE = * - .BANK5;
	echo "---- BANK5", BANK5_CODE_SIZE, "bytes"
	echo "---- BANK5", ($fff0 - *), "bytes free"










