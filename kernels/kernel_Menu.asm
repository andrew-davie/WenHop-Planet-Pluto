START_MENU = *

    DEF

    DEFPTR MENU_COLUBK
    DEFPTR MENU_PF1_LEFT
    DEFPTR MENU_PF2_LEFT
    DEFPTR MENU_PF1_RIGHT
    DEFPTR MENU_PF2_RIGHT
    DEFPTR MENU_AUDV0
    DEFPTR MENU_AUDC0
    DEFPTR MENU_AUDF0
    DEFPTR MENU_COLUPF
    DEFPTR MENU_COLUP0
    DEFPTR MENU_GRP0A
    DEFPTR MENU_GRP1A
    DEFPTR MENU_GRP0B
    DEFPTR MENU_GRP1B
    DEFPTR MENU_GRP0C
    DEFPTR MENU_GRP1C

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

    ; runVectoredCode[kernel] comes here

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


_menuExit           sta WSYNC
                    rts




VB_kernelMenu

                    jsr positionSpritesMenu

                    ldx #%00000001
                    stx CTRLPF              ; reflect PF

                    lda #_DS_MENU_COLUPF
                    sta COLUPF

                    sta WSYNC
                    sta HMOVE

                    lda #_DS_MENU_COLUPF_DATA       ; 2
                    sta COLUPF                      ; 3

                    lda #0
                    sta PF0
                    sta PF1
                    sta PF2

                    ldx #%00110011
                    stx NUSIZ0
                    stx NUSIZ1
                    stx VDELP0
                    stx VDELP1



                    rts

OS_kernelMenu
                    rts

    echo "KERNEL MENU [", (* - START_MENU)d,"] bytes"

; EOF
