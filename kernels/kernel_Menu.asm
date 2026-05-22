START_MENU = *

    DEFPTR MENU_COLUBK,    0
    DEFPTR MENU_PF1_LEFT,  1
    DEFPTR MENU_PF2_LEFT,  2
    DEFPTR MENU_PF1_RIGHT, 3
    DEFPTR MENU_PF2_RIGHT, 4
    DEFPTR MENU_AUDV0,     5
    DEFPTR MENU_AUDC0,     6
    DEFPTR MENU_AUDF0,     7
    DEFPTR MENU_COLUPF,    8
    DEFPTR MENU_COLUP0,    9
    DEFPTR MENU_GRP0A,     10
    DEFPTR MENU_GRP1A,     11
    DEFPTR MENU_GRP0B,     12
    DEFPTR MENU_GRP1B,     13
    DEFPTR MENU_GRP0C,     14
    DEFPTR MENU_GRP1C,     15

posXMenu .byte 80, 88

positionSpritesMenu

                    ldx #1
.loopMenu           lda posXMenu,x

                    sec
                    sta WSYNC
.divideMenu         sbc #15
                    bcs .divideMenu

                    eor #7
                    asl
                    asl
                    asl
                    asl

                    sta.wx HMP0,x
                    sta RESP0,x

                    dex
                    bpl .loopMenu

                    rts


kernelMenu
                    lda #_DS_MENU_COLUPF_DATA       ; 2
                    sta COLUPF                      ; 3

                    lda #0
                    sta PF0
                    sta PF1
                    sta PF2

                    sta WSYNC
                    jmp 0

_menuLoop                                           ;@3

                    lda #_DS_MENU_PF1_LEFT_DATA
                    sta PF1                         ; 5
                    lda #_DS_MENU_GRP0A_DATA
                    sta GRP0                        ; 5
                    lda #_DS_MENU_GRP1A_DATA
                    sta GRP1                        ; 5
                    lda #_DS_MENU_GRP0B_DATA
                    sta GRP0                        ; 5

                    lda #_DS_MENU_GRP1C_DATA        ; 2
                    tay                             ; 2

                    lda #_DS_MENU_PF2_LEFT_DATA
                    sta PF2                         ; 5

                    lda #_DS_MENU_GRP0C_DATA        ; 2
                    tax                             ; 2
                    lda #_DS_MENU_PF1_RIGHT_DATA
                    sta PF1                         ; 5

                    lda #_DS_MENU_PF2_RIGHT_DATA    ; 2
                    nop                             ; 2
                    sta PF2                         ; 3

                    lda #_DS_MENU_GRP1B_DATA        ; 2
                    sta GRP1                        ; 3
                    stx GRP0                        ; 3
                    sty GRP1                        ; 3
                    sta GRP0                        ; 3

                    lda #_DS_MENU_COLUP0_DATA       ; 2
                    sta COLUP0                      ; 3
                    sta.w COLUP1                    ; 4

                    lda #_DS_MENU_COLUPF_DATA       ; 2
                    sta COLUPF                      ; 3
;@76=0

                    jmp 0                           ; @3 --> start of line again


_menuExit
                    rts




VB_kernelMenu

                    jsr positionSpritesMenu

                    ldx #%00000001
                    stx CTRLPF              ; reflect PF

                    lda #_DS_MENU_COLUPF
                    sta COLUPF

                    sta WSYNC
                    sta HMOVE

                    rts

OS_kernelMenu
                    rts

    echo "KERNEL MENU [", (* - START_MENU)d,"] bytes"

; EOF
