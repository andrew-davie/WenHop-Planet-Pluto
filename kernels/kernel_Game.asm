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
    

kernelGame
_gameLoop

_NORMAL_KERNEL

    ; This is the entire display
    ;@3

                    lda #_DS_GAME_COLUP0_DATA
                    sta COLUP0                      ; 5
;                    SLEEP 5                         ; @8

                    lda #_DS_GAME_COLUBK_DATA
                    sta WSYNC                       ; 5 @13

                    sta COLUBK                      ; 3

                    lda #_DS_GAME_COLUPF_DATA
                    sta COLUPF                      ; 5 @21

                    lda #_DS_GAME_PF0_LEFT_DATA
                    sta PF0                         ; 5

                    lda #_DS_GAME_GRP0A_DATA
                    sta GRP0                        ; 5

                    lda #_DS_GAME_PF1_LEFT_DATA
                    sta PF1                         ; 5

                    lda #_DS_GAME_GRP1A_DATA
                    sta GRP1                        ; 5 @41

                    lda #_DS_GAME_PF2_LEFT_DATA
                    sta PF2                         ; 5

                    lda #_DS_GAME_PF0_RIGHT_DATA
                    sta PF0                         ; 5
                    lda #_DS_GAME_PF1_RIGHT_DATA
                    sta PF1                         ; 5
                    lda #_DS_GAME_PF2_RIGHT_DATA
                    sta PF2                         ; 5 @61 

                    SLEEP 10                         ; @66

                    lda #_DS_GAME_COLUP1_DATA
                    sta COLUP1                      ; 5 @76


                    jmp 0


_gameExit           rts

VB_kernelGame

                    lda #0
                    sta CTRLPF
                    sta NUSIZ0
                    sta NUSIZ1



                    rts


OS_kernelGame
                    rts

    echo "KERNEL GAME [", (* - START_GAME)d,"] bytes"

; EOF
