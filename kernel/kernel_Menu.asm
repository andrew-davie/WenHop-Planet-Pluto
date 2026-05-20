START_MENU = *

    DEFPTR MENU_COLUBK, 0

kernelMenu
_menuLoop           sta WSYNC

                    lda #0
                    sta PF0
                    sta PF1
                    sta PF2
                    sta GRP0
                    sta GRP1

                    lda #_DS_MENU_COLUBK_DATA
                    sta COLUBK

                    jmp 0

_menuExit
VB_kernelMenu
OS_kernelMenu
                    rts

    echo "KERNEL MENU [", (* - START_MENU)d,"] bytes"

; EOF
