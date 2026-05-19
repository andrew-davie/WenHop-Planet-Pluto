BANK_kernelRainbow = .BANK

    DEFPTR RBW_COLUBK, 0

_kernelRainbow
_rainbowLoop        sta WSYNC

                    lda #0
                    sta PF0
                    sta PF1
                    sta PF2
                    sta GRP0
                    sta GRP1

                    lda #_DS_RBW_COLUBK_DATA
                    sta COLUBK

                    jmp FASTJMP1

_rainbowExit
VB_kernelRainbow
OS_kernelRainbow
                    rts

; EOF
