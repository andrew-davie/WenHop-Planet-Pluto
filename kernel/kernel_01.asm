BANK_kernel_01 = .BANK



OS_kernel_01        rts


VB_kernel_01

;                     ldx test_position		;3		position in index and
;                     lda .calc_rpfp_table,x		;4		table read must come before the WSYNC

;                     sta WSYNC
;                     sta HMP1			;3
;                     and #$0f			;2,5
;                     tax				;2,7
;                     lda #AMPLITUDE			;2,9		positioning method while using
;                     sta AUDV0			;3,12		wave sound
;                     sta $2d				;3,15
; .loop_position_p1_2			;
;                     dex				;2,17
;                     bpl .loop_position_p1_2		;2,19
;                     sta RESP1			;3,22

;                     sta WSYNC
;                     sta HMOVE
                    lda #AMPLITUDE
                    sta AUDV0

                    sta WSYNC
                    sta HMCLR
                    lda #AMPLITUDE
                    sta AUDV0

                    rts

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Kernel 01 Routine @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

kernel_01

_kernel01Loop
				;kernel_01 demontrates the FastJump streams

                    sta WSYNC

                    lda #AMPLITUDE
                    sta AUDV0			;as well as handling wave samples
                    tay

                    lda #DS0DATA
                    sta COLUBK

                    lda amp_table1,y
                    sta GRP0
                    lda amp_table2,y
                    sta GRP1

                    jmp FASTJMP1

_kernel01Exit
 	rts



amp_table2
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
amp_table1
	.byte #%00000000
	.byte #%10000000
	.byte #%11000000
	.byte #%11100000
	.byte #%11110000
	.byte #%11111000
	.byte #%11111100
	.byte #%11111110
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111



; EOF
