	org CURRENT_ORG
	rorg $f000

BANK4_START

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
; .blank_scanlines_4
; 	sta WSYNC
; 	dex
; 	bne .blank_scanlines_4			;can use .blank_scanlines in any bank
; 	rts

; .blank_scanlines_aud_4				;can use .blank_scanlines_aud in any bank
; 	sta WSYNC
; 	lda #AMPLITUDE
; 	sta AUDV0
; 	dex
; 	bne .blank_scanlines_aud_4
; 	rts
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@







BANK4_CODE_SIZE = * - BANK4_START
	echo "---- BANK4", BANK4_CODE_SIZE, "bytes"
	echo "---- BANK4", ($fff0 - *), "bytes free"









CURRENT_ORG SET CURRENT_ORG + $1000

