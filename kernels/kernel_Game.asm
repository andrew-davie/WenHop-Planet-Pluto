START_GAME = *

    DEF

    DEFPTR GAME_COLUBK
    DEFPTR GAME_PF0_LEFT
    DEFPTR GAME_PF0_RIGHT
    DEFPTR GAME_PF1_LEFT
    DEFPTR GAME_PF1_RIGHT
    DEFPTR GAME_PF2_LEFT
    DEFPTR GAME_PF2_RIGHT
    DEFPTR GAME_COLUP0
    DEFPTR GAME_COLUP1
    DEFPTR GAME_COLUPF
    DEFPTR GAME_GRP0A
    DEFPTR GAME_GRP1A
    

;-------------------------------------------------------------------------------

kernelGame

    ; runVectoredCode[kernel] comes here

                    lda #_DS_GAME_COLUP1_DATA
                    sta COLUP1                      ; 5 @76

                    lda #_DS_GAME_COLUP0_DATA
                    sta COLUP0                      ; 5

                    lda #_DS_GAME_COLUBK_DATA

                    sta WSYNC
                    jmp 0


_gameLoop ; @3

                    sta COLUBK                      ; 3

                    lda #_DS_GAME_COLUPF_DATA
                    sta COLUPF                      ; 5

                    lda #_DS_GAME_PF0_LEFT_DATA
                    sta PF0                         ; 5

                    lda #_DS_GAME_GRP0A_DATA
                    sta GRP0                        ; 5

                    lda #_DS_GAME_PF1_LEFT_DATA
                    sta PF1                         ; 5

                    lda #_DS_GAME_GRP1A_DATA
                    sta GRP1                        ; 5

                    lda #_DS_GAME_PF2_LEFT_DATA
                    sta PF2                         ; 5

                    lda #_DS_GAME_PF0_RIGHT_DATA
                    sta PF0                         ; 5
                    lda #_DS_GAME_PF1_RIGHT_DATA
                    sta PF1                         ; 5
                    lda #_DS_GAME_PF2_RIGHT_DATA
                    sta PF2                         ; 5

                    SLEEP 13

                    lda #_DS_GAME_COLUP1_DATA
                    sta COLUP1                      ; 5 @76


                    lda #_DS_GAME_COLUP0_DATA
                    sta COLUP0                      ; 5

                    lda #_DS_GAME_COLUBK_DATA

                    jmp 0                           ; 3

_gameExit
                    lda #0
                    sta COLUPF
                    ; sta PF0
                    ; sta PF1
                    ; sta PF2
                    sta WSYNC
                    rts

;-------------------------------------------------------------------------------


VB_kernelGame

                    ldx #>_P0_X
                    stx DSPTR
                    ldx #<_P0_X
                    stx DSPTR

                    ldx #0
posSpritesGame      lda #DSCOMM

                    sec
                    sta WSYNC
div15Loop           sbc #15
                    bcs div15Loop

                    eor #7
                    asl
                    asl
                    asl
                    asl

                    sta.wx HMP0,x
                    sta RESP0,x

                    inx
                    cpx #2
                    bne posSpritesGame

                    sta WSYNC
                    sta HMOVE

                    ldx #0
                    stx CTRLPF

;                    lda #%101
                    stx NUSIZ0
                    stx NUSIZ1

                    stx COLUBK
                    stx COLUPF
                    stx COLUP0
                    stx COLUP1
                    stx GRP0
                    stx GRP1
                    stx PF0
                    stx PF1
                    stx PF2
                    rts


;-------------------------------------------------------------------------------

OS_kernelGame
                    rts

;-------------------------------------------------------------------------------

    echo "KERNEL GAME [", (* - START_GAME)d,"] bytes"

; EOF
