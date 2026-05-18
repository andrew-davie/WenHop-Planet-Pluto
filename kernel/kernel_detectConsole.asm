BANK_kernelDetectConsole = .BANK

    ; A well-formed black screen for console detection

VB_kernelDetectConsole
OS_kernelDetectConsole

                    rts

kernelDetectConsole


                    ldx #_SCANLINES
_detectConsoleLoop  sta WSYNC

                    lda #$42
                    sta COLUBK

                    dex
                    bne _detectConsoleLoop
                    rts

; EOF
