BANK_kernelCopyright = .BANK

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


kernelChampGames
kernelCopyright


#if 0
    ; entry point for champ kernel

                    ldx #_SCANLINES
                    stx scanline


centered6Sprites    sta WSYNC                       ; @0

                    sta.w COLUP1                    ; 3
                    lda #_DS_GRP0a                  ; 2
                    sta GRP0                        ; 3
                    lda #_DS_GRP1a                  ; 2
                    nop                             ; 2
                    sta GRP1                        ; 3
                    lda #_DS_GRP0b                  ; 2
                    nop                             ; 2
                    sta GRP0                        ; 3
                    lda #_DS_GRP1c                  ; 2
                    nop                             ; 2
                    sta temp                        ; 3
                    lda #_DS_GRP0c                  ; 2
                    tax                             ; 2
                    lda #_DS_GRP1b                  ; 2
                    nop                             ; 2
                    ldy temp                        ; 3
                    sta GRP1                        ; 3
                    stx GRP0                        ; 3
                    sty GRP1                        ; 3
                    sta GRP0                        ; 3
    
                    lda #_DS_COLUPF                 ; 2
                    sta COLUPF                      ; 3
                    lda #_DS_COLUP0                 ; 2
                    sta COLUP0                      ; 3

                    dec scanline                    ; 5
                    bne centered6Sprites               ; 3(2)  --> 71 when branching

                    rts

#endif

; EOF

