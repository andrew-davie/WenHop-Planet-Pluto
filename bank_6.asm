	org CURRENT_ORG
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






    CHECK_OVERFLOW 6, $1000

CURRENT_ORG SET CURRENT_ORG + $1000
