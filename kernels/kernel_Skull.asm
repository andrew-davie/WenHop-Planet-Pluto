START_SKULL = *

    DEF

    DEFPTR SKULL_COLUBK
    DEFPTR SKULL_PF1_LEFT
    DEFPTR SKULL_PF2_LEFT
    DEFPTR SKULL_PF1_RIGHT
    DEFPTR SKULL_PF2_RIGHT
    DEFPTR SKULL_AUDV0
    DEFPTR SKULL_AUDC0
    DEFPTR SKULL_AUDF0
    DEFPTR SKULL_COLUPF
    DEFPTR SKULL_COLUP0
    DEFPTR SKULL_GRP0A
    DEFPTR SKULL_GRP1A
    DEFPTR SKULL_GRP0B
    DEFPTR SKULL_GRP1B
    DEFPTR SKULL_GRP0C
    DEFPTR SKULL_GRP1C

;-------------------------------------------------------------------------------

posXSkull .byte 80, 88

positionSpritesSkull

                    ldx #1
.loopSkull           lda posXSkull,x

                    sec
                    sta WSYNC
.divideSkull         sbc #15
                    bcs .divideSkull

                    eor #7
                    asl
                    asl
                    asl
                    asl

                    sta.wx HMP0,x
                    sta RESP0,x

                    dex
                    bpl .loopSkull

                    rts



OS_kernelSkull  rts


VB_kernelSkull

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

                    jsr positionSpritesSkull
                    sta WSYNC
                    sta HMOVE

                    ldx #$FC
                    stx PF2                    

                    rts


kernelSkull
                    sta WSYNC
                    jmp 0

    ; runVectoredCode[kernel] comes here


_skullLoop                                           ;@3

                    lda #_DS_SKULL_PF1_LEFT_DATA
                    sta PF1                         ; 5
                    lda #_DS_SKULL_GRP0A_DATA
                    sta GRP0                        ; 5
                    lda #_DS_SKULL_GRP1A_DATA
                    sta GRP1                        ; 5
                    lda #_DS_SKULL_GRP0B_DATA
                    sta GRP0                        ; 5

                    lda #_DS_SKULL_GRP1C_DATA        ; 2
                    tay                             ; 2

                    lda #_DS_SKULL_PF2_LEFT_DATA
                    sta PF2                         ; 5

                    lda #_DS_SKULL_GRP0C_DATA        ; 2
                    tax                             ; 2
                    lda #_DS_SKULL_PF1_RIGHT_DATA
                    sta PF1                         ; 5

                    lda #_DS_SKULL_PF2_RIGHT_DATA    ; 2
                    nop                             ; 2
                    sta PF2                         ; 3

                    lda #_DS_SKULL_GRP1B_DATA        ; 2
                    sta GRP1                        ; 3
                    stx GRP0                        ; 3
                    sty GRP1                        ; 3
                    sta GRP0                        ; 3

                    lda #_DS_SKULL_COLUP0_DATA       ; 2
                    sta COLUP0                      ; 3
                    sta.w COLUP1                    ; 4

                    lda #_DS_SKULL_COLUPF_DATA       ; 2
                    sta COLUPF                      ; 3
;@76=0

                    jmp 0                           ; @3 --> start of line again


; _skullLoop          ;sta WSYNC

;                     lda #_DS_SK_COLUPF_DATA
;                     sta COLUPF

;                     lda #_DS_SK_COLUBK_DATA
;                     sta COLUBK

;                     lda #_DS_SK_GRP0A_DATA
;                     sta GRP0
;                     lda #_DS_SK_GRP1A_DATA
;                     sta.w GRP1
;                     lda #_DS_SK_GRP0B_DATA
;                     sta GRP0
;                     lda #_DS_SK_GRP1C_DATA
;                     ;nop
;                     sta SK_temp
;                     lda #_DS_SK_GRP0C_DATA
;                     tax
;                     lda #_DS_SK_GRP1B_DATA
;                     nop
;                     ldy SK_temp
;                     sta GRP1
;                     stx GRP0
;                     sty GRP1
;                     sta GRP0

;                     lda #_DS_SK_COLUP0_DATA
;                     sta COLUP0
;                     sta COLUP1

;                     jmp 0


_skullExit
                    rts


    echo "KERNEL SKULL [", (*-START_SKULL)d,"] bytes"
; EOF

