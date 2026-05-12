	org CURRENT_BANK
	rorg $f000

BANK6_START

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
; .blank_scanlines_6
; 	sta WSYNC
; 	dex
; 	bne .blank_scanlines_6			;can use .blank_scanlines in any bank
; 	rts

; .blank_scanlines_aud_6				;can use .blank_scanlines_aud in any bank
; 	sta WSYNC
; 	lda #AMPLITUDE
; 	sta AUDV0
; 	dex
; 	bne .blank_scanlines_aud_6
; 	rts
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@







BANK6_CODE_SIZE = * - BANK6_START
	echo "---- BANK6", BANK6_CODE_SIZE, "bytes"
	echo "---- BANK6", ($fff0 - *), "bytes free"









CURRENT_BANK SET CURRENT_BANK + $1000
