BANK_kernelRainbow = .BANK


VB_kernelRainbow
OS_kernelRainbow    rts


kernelRainbow


              ldx #_SCANLINES
.loop               

                    sta WSYNC

                    txa ;lda #DS0DATA
                    sta COLUBK

                    dex
                    bne .loop
                    rts

; EOF
