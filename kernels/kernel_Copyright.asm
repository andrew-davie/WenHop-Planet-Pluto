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
    DEFPTR CP_COLUBK

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

                    ; ldx #>_colubk
                    ; stx DSPTR
                    ; ldx #<_colubk
                    ; stx DSPTR
                    ; lda #DSCOMM 
                    ; sta COLUBK
                    ; sta COLUPF


                    ldx #%00000001
                    stx CTRLPF              ; reflect PF

                    jsr positionSprites
                    sta WSYNC
                    sta HMOVE

                    ldx #$FF
                    stx PF2  

                    ; lda #_DS_CP_COLUP0_DATA
                    ; sta COLUP0
                    ; sta COLUP1


                    rts


kernelCopyright

    ; runVectoredCode[kernel] comes here


_copyrightLoop      sta WSYNC

                    lda #_DS_CP_COLUPF_DATA
                    sta COLUPF

                    lda #_DS_CP_COLUBK_DATA
                    sta COLUBK

                    lda #_DS_CP_GRP0A_DATA
                    sta GRP0
                    lda #_DS_CP_GRP1A_DATA
                    sta.w GRP1
                    lda #_DS_CP_GRP0B_DATA
                    sta GRP0
                    lda #_DS_CP_GRP1C_DATA
                    ;nop
                    sta CP_temp
                    lda #_DS_CP_GRP0C_DATA
                    tax
                    lda #_DS_CP_GRP1B_DATA
                    nop
                    ldy CP_temp
                    sta GRP1
                    stx GRP0
                    sty GRP1
                    sta GRP0

                    lda #_DS_CP_COLUP0_DATA
                    sta COLUP0
                    sta COLUP1

                    jmp 0


_copyrightExit
                    rts


    echo "KERNEL COPYRIGHT [", (*-START_COPYRIGHT)d,"] bytes"
; EOF

