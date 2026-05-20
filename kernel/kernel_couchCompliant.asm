START_COUCH_COMPLIANT = *

    DEFPTR CC_COLUP0, 0


kernelCouchCompliant
_couchLoop          sta WSYNC

                    lda #0
                    sta PF0
                    sta PF1
                    sta PF2
                    sta GRP0
                    sta GRP1

                inx
                   ; lda #_DS_CC_COLUP0__DATA
                    stx COLUBK

                    jmp 0

_couchCompliantExit
VB_kernelCouchCompliant
OS_kernelCouchCompliant
                    rts



    echo "KERNEL COUCH_COMPLIANT [", (* - START_COUCH_COMPLIANT)d,"] bytes"

; EOF
