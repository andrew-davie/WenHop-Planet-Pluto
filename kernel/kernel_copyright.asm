BANK_kernelCopyright = .BANK


    DEFPTR CP_GRP0A, 0
    DEFPTR CP_GRP0B, 1
    DEFPTR CP_GRP0C, 2
    DEFPTR CP_GRP1A, 3
    DEFPTR CP_GRP1B, 4
    DEFPTR CP_GRP1C, 5

    DEFPTR CP_COLUPF, 6
    DEFPTR CP_COLUP0, 7




#if 0

;-------------------------------------------------------------------------------

grabAudio
                    ldx #1
audio               lda #_DS_AUDV0
                    sta AUDV0,x
                    lda #_DS_AUDC0
                    sta AUDC0,x
                    lda #_DS_AUDF0
                    sta AUDF0,X
                    dex
                    bpl audio

                    rts

;-------------------------------------------------------------------------------

positionSprites

                    ldx #1
vbSetInitialChamp   lda #DSCOMM                 ; P1_X, P0_X

                    sec
                    sta WSYNC
DivideLoopC         sbc #15
                    bcs DivideLoopC

                    eor #7
                    asl
                    asl
                    asl
                    asl

                    sta.w HMP0,x
                    sta RESP0,x

                    dex
                    bpl vbSetInitialChamp

                    rts

;-------------------------------------------------------------------------------
#endif

OS_kernelCopyright  rts


VB_kernelCopyright

#if 0

                    ldx #%00110011
                    stx NUSIZ0                      ; three copies close, missile x8
                    stx NUSIZ1                      ; three copies close, missile x8
                    stx VDELP0                      ; vertical delay on
                    stx VDELP1                      ; vertical delay on

                    ldx #0
                    stx ENAM0
                    stx ENAM1

                    ldx #%00000001
                    stx CTRLPF              ; reflect PF

_EXIT_CHAMP_KERNEL

                    ldx #2
                    stx VBLANK              ; video output off (D2 of X is 1 always!)

;                     ldx kernel
;                     cpx currentKernel
;                     beq thisKernelC

;                     jmp startAnyKernel
; thisKernelC

;                    sta WSYNC

                    ldx #OS_TIM64T
                    stx TIM64T              ; set timer for OS

                    jsr grabAudio

                    ldy #_FN_MENU_OS
                    jsr CallArmCode

safeTimerWait2C     lda INTIM
                    bpl safeTimerWait2C


                    ldy #2
                    sta WSYNC
                    sty VSYNC           ; turn on Vertical Sync signal

                    ldx #0
                    stx WSYNC
                    stx WSYNC           ; 3 29/0
                    stx WSYNC           ; end of VerticalSync scanline 3
                    stx VSYNC           ; turn off Vertical Sync signal

                    ldx #VB_TIM64T
                    stx TIM64T

                    ldy #_FN_MENU_VB
                    jsr CallArmCode

                    jsr positionSprites

                    lda #DSCOMM
                    sta kernel
                    lda #DSCOMM
                    sta COLUBK

                    sta WSYNC
                    sta HMOVE

safeTimerWait3C     lda INTIM
                    bpl safeTimerWait3C

                    ldx #0
                    stx VBLANK              ; video output on


    ; Here starts the non-VB kernel part

                    ldx #6 ; #_DS_COLUP0
                    stx COLUP0
                    stx COLUP1
                    ldx #$FC ;#_DS_PF1_RIGHT
                    stx PF2

;                    jmp FASTJMP1
        ; TODO: put vb here
                    rts

#endif

kernelCopyright


_copyrightLoop

                    sta WSYNC                       ; @0

                    sta.w COLUP1                    ; 3
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

                    jmp FASTJMP1


_copyrightExit
                    rts

#endif

; EOF

