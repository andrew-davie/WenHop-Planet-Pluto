START_GLOBE = *

    DEF

    DEFPTR GLOBE_COLUBK
    DEFPTR GLOBE_COLUPF
    DEFPTR GLOBE_COLUP0

    DEFPTR GLOBE_PF0_LEFT
    DEFPTR GLOBE_PF1_LEFT
    DEFPTR GLOBE_PF2_LEFT
    DEFPTR GLOBE_PF0_RIGHT
    DEFPTR GLOBE_PF1_RIGHT
    DEFPTR GLOBE_PF2_RIGHT

    DEFPTR GLOBE_GRP0A
    DEFPTR GLOBE_GRP1A
    DEFPTR GLOBE_GRP0B
    DEFPTR GLOBE_GRP1B
    DEFPTR GLOBE_GRP0C
    DEFPTR GLOBE_GRP1C

    DEFPTR GLOBE_AUDV0
    DEFPTR GLOBE_AUDC0
    DEFPTR GLOBE_AUDF0

;-------------------------------------------------------------------------------

posXGlobe .byte 80, 88

positionSpritesGlobe
 
                    ldx #1
.loopGlobe          lda posXGlobe,x

                    sec
                    sta WSYNC
.divideGlobe        sbc #15
                    bcs .divideGlobe

                    eor #7
                    asl
                    asl
                    asl
                    asl

                    sta.wx HMP0,x
                    sta RESP0,x

                    dex
                    bpl .loopGlobe

                    rts



OS_kernelGlobe      rts


VB_kernelGlobe

                    ldx #%00110011
                    stx NUSIZ0
                    stx NUSIZ1
                    stx VDELP0
                    stx VDELP1

                    ldx #0
                    stx ENAM0
                    stx ENAM1
                    stx COLUBK
                    stx COLUPF

                    ldx #%00000001
                    stx CTRLPF              ; reflect PF

                    jsr positionSpritesGlobe
                    sta WSYNC
                    sta HMOVE

                    rts


kernelGlobe

                    sta WSYNC
                    jmp 0

    ; runVectoredCode[kernel] comes here

_globeLoop                                           ;@3

                    lda #_DS_GLOBE_PF1_LEFT_DATA
                    sta PF1                         ; 5
                    lda #_DS_GLOBE_GRP0A_DATA
                    sta GRP0                        ; 5
                    lda #_DS_GLOBE_GRP1A_DATA
                    sta GRP1                        ; 5
                    lda #_DS_GLOBE_GRP0B_DATA
                    sta GRP0                        ; 5

                    lda #_DS_GLOBE_GRP1C_DATA       ; 2
                    tay                             ; 2

                    lda #_DS_GLOBE_PF2_LEFT_DATA
                    sta PF2                         ; 5

                    lda #_DS_GLOBE_GRP0C_DATA       ; 2
                    tax                             ; 2
                    lda #_DS_GLOBE_PF1_RIGHT_DATA
                    sta PF1                         ; 5

                    lda #_DS_GLOBE_PF2_RIGHT_DATA   ; 2
                    nop                             ; 2
                    sta PF2                         ; 3

                    lda #_DS_GLOBE_GRP1B_DATA       ; 2
                    sta GRP1                        ; 3
                    stx GRP0                        ; 3
                    sty GRP1                        ; 3
                    sta GRP0                        ; 3

                    lda #_DS_GLOBE_COLUP0_DATA      ; 2
                    sta COLUP0                      ; 3
                    sta.w COLUP1                    ; 4

                    lda #_DS_GLOBE_COLUPF_DATA      ; 2
                    sta COLUPF                      ; 3
; @76 == 0

                    jmp 0                           ; @3 --> start of line again

_globeExit          sta WSYNC
                    rts


    echo "KERNEL GLOBE [", (*-START_GLOBE)d,"] bytes"
; EOF

