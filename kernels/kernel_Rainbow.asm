START_RAINBOW = *

    DEF
    DEFPTR RBW_COLUBK

kernelRainbow
_rainbowLoop        sta WSYNC

                    lda #0
                    sta PF0
                    sta PF1
                    sta PF2
                    sta GRP0
                    sta GRP1

                    lda #_DS_RBW_COLUBK_DATA
                    sta COLUBK

                    jmp 0

_rainbowExit
VB_kernelRainbow
OS_kernelRainbow
                    rts

    echo "KERNEL RAINBOW [", (* - START_RAINBOW)d,"] bytes"

; EOF
