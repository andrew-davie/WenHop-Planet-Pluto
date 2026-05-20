START_DETECT_CONSOLE = *

    ; A well-formed black screen for console detection

kernelDetectConsole

                    ldx #_SCANLINES
_detectConsoleLoop  sta WSYNC
                    dex
                    bne _detectConsoleLoop

VB_kernelDetectConsole
OS_kernelDetectConsole

                    rts

    echo "KERNEL DETECT_CONSOLE [", (* - START_DETECT_CONSOLE)d,"] bytes"

; EOF
