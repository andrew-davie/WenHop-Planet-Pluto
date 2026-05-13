	org CURRENT_ORG
	rorg $f000

BANK3_START

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
; .blank_scanlines_3
; 	sta WSYNC
; 	dex
; 	bne .blank_scanlines_3			;can use .blank_scanlines in any bank
; 	rts

; .blank_scanlines_aud_3				;can use .blank_scanlines_aud in any bank
; 	sta WSYNC
; 	lda #AMPLITUDE
; 	sta AUDV0
; 	dex
; 	bne .blank_scanlines_aud_3
; 	rts
;@@@@@@@@@@@@@@@@@@@@@ These routines put at beginning of each bank so all have access @@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@







    CHECK_OVERFLOW 3, $1000

CURRENT_ORG SET CURRENT_ORG + $1000

