BANK_kernelDetectConsole = .BANK

    ; A well-formed black screen for console detection


kernelDetectConsole


                    ldx #_SCANLINES
_detectConsoleLoop  sta WSYNC
                    dex
                    bne _detectConsoleLoop

VB_kernelDetectConsole
OS_kernelDetectConsole

                    rts

; EOF
