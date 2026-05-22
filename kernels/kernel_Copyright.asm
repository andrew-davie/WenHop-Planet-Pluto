START_COPYRIGHT = *

    DEF

    DEFPTR CP_GRP0A
    DEFPTR CP_GRP1A
    DEFPTR CP_GRP0B
    DEFPTR CP_GRP1B
    DEFPTR CP_GRP0C
    DEFPTR CP_GRP1C
    DEFPTR CP_PF
    DEFPTR CP_COLUPF
    DEFPTR CP_COLUP0

;-------------------------------------------------------------------------------

posX .byte 56,64

positionSprites     SUBROUTINE

                    ldx #1
.loop               lda posX,x

                    sec
                    sta WSYNC
.divide             sbc #15
                    bcs .divide

                    eor #7
                    asl
                    asl
                    asl
                    asl

                    sta.w HMP0,x
                    sta RESP0,x

                    dex
                    bpl .loop

                    rts

;-------------------------------------------------------------------------------


OS_kernelCopyright  rts


VB_kernelCopyright
                    ldx #%00110011
                    stx NUSIZ0
                    stx NUSIZ1
                    stx VDELP0
                    stx VDELP1

                    ldx #0
                    stx ENAM0
                    stx ENAM1
                    stx COLUBK
                    ; stx COLUP0
                    ; stx COLUP1
                    stx COLUPF

                    ldx #%00000001
                    stx CTRLPF              ; reflect PF

                    jsr positionSprites
                    sta WSYNC
                    sta HMOVE

                    ldx #$FC
                    stx PF2                    

                    rts


kernelCopyright
_copyrightLoop      sta WSYNC                       ; @0
                    sta.w COLUP1                    ; 4

                    lda #_DS_CP_GRP0A_DATA          ; 2
                    sta GRP0                        ; 3
                    lda #_DS_CP_GRP1A_DATA          ; 2
                    nop                             ; 2
                    sta GRP1                        ; 3
                    lda #_DS_CP_GRP0B_DATA          ; 2
                    nop                             ; 2
                    sta GRP0                        ; 3
                    lda #_DS_CP_GRP1C_DATA          ; 2
                    nop                             ; 2
                    sta CP_temp                     ; 3
                    lda #_DS_CP_GRP0C_DATA          ; 2
                    tax                             ; 2
                    lda #_DS_CP_GRP1B_DATA          ; 2
                    nop                             ; 2
                    ldy CP_temp                     ; 3
                    sta GRP1                        ; 3
                    stx GRP0                        ; 3
                    sty GRP1                        ; 3
                    sta GRP0                        ; 3

                    lda #_DS_CP_COLUPF_DATA         ; 2
                    sta COLUPF                      ; 3
                    lda #_DS_CP_COLUP0_DATA         ; 2
                    sta COLUP0                      ; 3

                    jmp 0                           ; 3


_copyrightExit
                    rts


    echo "KERNEL COPYRIGHT [", (*-START_COPYRIGHT)d,"] bytes"
; EOF

