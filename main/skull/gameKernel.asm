

KernelGame


                    ldx #0
                    stx NUSIZ0
                    stx NUSIZ1
                    stx CTRLPF

                        IF __ENABLE_ATARIVOX
;                    stx spoken
    ENDIF


OverScanGame
_EXIT_KERNEL    ;@3


                    lda kernel
                    cmp currentKernel
                    beq noSwitchKernel
                    jmp startAnyKernel
noSwitchKernel

                    sta WSYNC

                    ldx #2
                    stx VBLANK              ; video output off

                    ;ldx tv_type
                    ldx #OS_TIM64T ; TimerOS,x
                    stx TIM64T              ; set timer for OS


    IF __ENABLE_ATARIVOX
         jsr speakJet
    ENDIF


                    ldy #_FN_GAME_OS
                    jsr idleProcess

VerticalSync
                    sta WSYNC

; --- start scanline 1 of Vertical Sync ---

                    ldy #2
                    sty VSYNC                       ; turn on Vertical Sync signal

;                    ldx tv_type
                    ldx #VB_TIM64T ;TimerVB,x
                    stx TIM64T


                    ldx #0
                    ; stx GRP0
                    ; stx GRP1
                    stx PF0
                    stx PF1
                    stx PF2
                    stx COLUBK

                    ldy #<_DS_TO_ARM
                    sty DSPTR
                    sty DSPTR
                    sty DSWRITE                     ; DUMMY
                    ldy SWCHA                       ; read state of both joysticks
                    sty DSWRITE                     ; save in _SWCHA
                    ldy SWCHB                       ; read state of console switches
                    sty DSWRITE                     ; save in _SWCHB
                    ldy INPT4                       ; read state of left joystick firebutton
                    sty DSWRITE                     ; save in _INPT4

;                    ldy INPT5                       ; read state of right joystick firebutton
;                    sty DSWRITE                     ; save in _INPT5
;                    stx WSYNC
;                    stx WSYNC           ; end of VerticalSync scanline 3
;                    stx VSYNC           ; turn off Vertical Sync signal

                    ldx #1

                    sec
                    lda #DSCOMM     ; = _P1_X

                    sta WSYNC           ; VSYNC 1   3 10/0
                    sta WSYNC           ; VSYNC 2

                    sta WSYNC           ; VSYNC 3
DivideLoop2         sbc #15
                    bcs DivideLoop2

                    eor #7
                    asl
                    asl
                    asl
                    asl

                    sta.wx HMP0,X
                    sta RESP0,X

                    dex
                    stx VSYNC

                    lda #_DS_AUDV0
                    sta AUDV0
                    lda #_DS_AUDC0
                    sta AUDC0
                    lda #_DS_AUDF0
                    sta AUDF0

                    lda #DSCOMM     ; = _P0_X

                    sec
                    sta WSYNC
DivideLoop3         sbc #15
                    bcs DivideLoop3

                    eor #7
                    asl
                    asl
                    asl
                    asl
                    sta.wx HMP0,X
                    sta RESP0,X


                    lda #DSCOMM             ; tv_type
                    ;sta tv_type            ; superfluous/unused here


                    lda #_DS_AUDV0
                    sta AUDV1
                    lda #_DS_AUDC0
                    sta AUDC1
                    lda #_DS_AUDF0
                    sta AUDF1

                    lda #_DS_COLUP0                 ; score colours
                    sta COLUP0
                    lda #_DS_COLUP1
                    sta COLUP1

                    lda #DSCOMM
                    sta kernel

                    sta WSYNC
                    sta HMOVE

GameVerticalBlank

    ; IF __ENABLE_ATARIVOX
    ;     jsr speakJet
    ; ENDIF

                    ldy #_FN_GAME_VB
                    jsr idleProcess

                    ldx #0
                    stx VBLANK                      ; video output on
                    stx WSYNC

                    jmp FASTJMP1                    ;@0 kernel starts @3 --> _NORMAL_KERNEL


_NORMAL_KERNEL ; @3

                    SLEEP 5                         ; @8

                    lda #_DS_COLUBK
                    sta WSYNC                       ; 5 @13

                    sta COLUBK                      ; 3

                    lda #_DS_COLUPF
                    sta COLUPF                      ; 5 @21

                    lda #_DS_PF0_LEFT
                    sta PF0                         ; 5

                    lda #_DS_GRP0a
                    sta GRP0                        ; 5

                    lda #_DS_PF1_LEFT
                    sta PF1                         ; 5

                    lda #_DS_GRP1a
                    sta GRP1                        ; 5 @41

                    lda #_DS_PF2_LEFT
                    sta PF2                         ; 5

                    lda #_DS_PF0_RIGHT
                    sta PF0                         ; 5
                    lda #_DS_PF1_RIGHT
                    sta PF1                         ; 5
                    lda #_DS_PF2_RIGHT
                    sta PF2                         ; 5 @61

                    SLEEP 5                         ; @66

                    lda #_DS_COLUP0
                    sta COLUP0                      ; 5
                    lda #_DS_COLUP1
                    sta COLUP1                      ; 5 @76

                    jmp FASTJMP1                    ; 3 @3   --> _NORMAL_KERNEL until last line
                                                    ;        --> _EXIT_KERNEL at end

    ;@3--> start of line again :)


; EOF