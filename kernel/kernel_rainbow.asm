BANK_kernelRainbow = .BANK

    DEFPTR RBW_COLUBK, 0


VB_kernelRainbow
OS_kernelRainbow    rts


kernelRainbow




              ldx #_SCANLINES






_rainbowLoop
.loop               

                    sta WSYNC

                    ; ldx #$42
                    ; stx COLUBK

                    lda #_DS_RBW_COLUBK_DATA
                    sta COLUBK


                    dex

;                    jmp FASTJMP1
;
                    bne .loop

_rainbowExit

                    rts

; EOF
