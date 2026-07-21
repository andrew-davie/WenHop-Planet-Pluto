START_SKULL = *

    DEF

    DEFPTR SKULL_COLUBK
    DEFPTR SKULL_COLUPF
    DEFPTR SKULL_COLUP0
    DEFPTR SKULL_PF1_LEFT
    DEFPTR SKULL_PF2_LEFT
    DEFPTR SKULL_PF1_RIGHT
    DEFPTR SKULL_PF2_RIGHT
    DEFPTR SKULL_GRP0A
    DEFPTR SKULL_GRP1A
    DEFPTR SKULL_GRP0B
    DEFPTR SKULL_GRP1B
    DEFPTR SKULL_GRP0C
    DEFPTR SKULL_GRP1C
    DEFPTR SKULL_AUDV0
    DEFPTR SKULL_AUDC0
    DEFPTR SKULL_AUDF0

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
                    ; stx COLUBK
                    ; stx COLUP0
                    ; stx COLUP1
                    stx COLUPF

                    lda #_DS_SKULL_COLUBK_DATA
                    sta COLUBK                      ; todo: fix -- we only have 1 BK not a screenfull

                    ; ldx #$20
                    ; stx COLUBK
                    ; ldx #>_colubk
                    ; stx DSPTR
                    ; ldx #<_colubk
                    ; stx DSPTR
                    ; lda #DSCOMM 
                    ; sta COLUBK
                    ; sta COLUPF


                    ldx #%01
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

_skullLoop                                          ;@3

                    lda #_DS_SKULL_PF1_LEFT_DATA
                    sta PF1                         ; 5
                    lda #_DS_SKULL_GRP0A_DATA
                    sta GRP0                        ; 5
                    lda #_DS_SKULL_GRP1A_DATA
                    sta GRP1                        ; 5
                    lda #_DS_SKULL_GRP0B_DATA
                    sta GRP0                        ; 5

                    lda #_DS_SKULL_GRP1C_DATA       ; 2
                    tay                             ; 2

                    lda #_DS_SKULL_PF2_LEFT_DATA
                    sta PF2                         ; 5

                    lda #_DS_SKULL_GRP0C_DATA       ; 2
                    tax                             ; 2
                    lda #_DS_SKULL_PF1_RIGHT_DATA
                    sta PF1                         ; 5

                    lda #_DS_SKULL_PF2_RIGHT_DATA   ; 2
                    nop                             ; 2
                    sta PF2                         ; 3

                    lda #_DS_SKULL_GRP1B_DATA       ; 2
                    sta GRP1                        ; 3
                    stx GRP0                        ; 3
                    sty GRP1                        ; 3
                    sta GRP0                        ; 3

                    lda #_DS_SKULL_COLUP0_DATA      ; 2
                    sta COLUP0                      ; 3
                    sta.w COLUP1                    ; 4

                    lda #_DS_SKULL_COLUPF_DATA      ; 2
                    sta COLUPF                      ; 3
; @76 == 0

                    jmp 0                           ; @3 --> start of line again

_skullExit          sta WSYNC
                    rts


    echo "KERNEL SKULL [", (*-START_SKULL)d,"] bytes"

; EOF
