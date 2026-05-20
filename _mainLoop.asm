
mainGameLoop

                	lda #2
                    sta WSYNC
                    sta VBLANK                  ; overscan!

                    ldx tvSystem
                    lda TimerOS,x
                    sta TIM64T

    ; call ARM Overscan

                	ldx #>_RUN_FUNC
                	stx DSPTR
                	ldx #<_RUN_FUNC
                	stx DSPTR

                	ldx #_RUNARM_OVERSCAN
                	stx DSWRITE

    ; send controllers to ARM

                	ldx SWCHA
                	stx DSWRITE
                	ldx SWCHB
                	stx DSWRITE
                	ldx INPT4
                	stx DSWRITE
                	ldx INPT5
                	stx DSWRITE			    ; all Atari inputs to ARM

                	ldx call_fn
                	stx CALLFN              ; --> runARM_Overscan()

    ; retrieve ARM 'system' variables

                    lda #DS31DATA
                    sta kernel
                    lda #DS31DATA
                    sta tvSystem
                    lda #DS31DATA
                    sta soundMode
                    lda #DS31DATA
                    sta COLUBK

                    lda #DS31DATA
                    sta AUDV0
                    lda #DS31DATA
                    sta AUDV1
                    lda #DS31DATA
                    sta AUDC0
                    lda #DS31DATA
                    sta AUDC1
                    lda #DS31DATA
                    sta AUDF0
                    lda #DS31DATA
                    sta AUDF1


                    jsr scanAndUpdateSaveKey            ; update ONE *CHANGED* SaveKey byte


    ; run kernel-specific 6502 Overscan code (OS_FN_OFFSET) 

                    lda kernel
                    clc
                    adc #OS_FN_OFFSET
                    tax
                    jsr runVectoredCode               ; actually run 6502 kernel-specific VB

.waitOS             lda INTIM	
                    bne .waitOS

    ; vertical blank
    ; any kernel switching will have just happened

                    ldx #%1110
                    txa
.verticalSync       sta WSYNC
                    sta VSYNC                   ; indicate vblank
                    lsr 
                    bne .verticalSync 


                    ldx tvSystem
                    lda TimerVB,x
                    sta TIM64T

    ; call ARM Vertical Blank

                    ldx #>_RUN_FUNC
                    stx DSPTR
                    ldx #<_RUN_FUNC
                    stx DSPTR
                    ldx #_RUNARM_VBLANK
                    stx DSWRITE

                    ldx call_fn
                    stx CALLFN                  ; call VerticalBlank in ARM

    ; run kernel-specific 6502 Vertical Blank code (VB_FN_OFFSET)

                    lda kernel
                    clc
                    adc #VB_FN_OFFSET
                    tax
                    jsr runVectoredCode               ; actually run 6502 kernel-specific VB

.waitVB             lda INTIM
                    bne .waitVB

                    sta WSYNC
                    sta VBLANK                  ; screen on

                    ldx kernel
                    jsr runVectoredCode               ; run 6502 kernel

                    jmp mainGameLoop

;-------------------------------------------------------------------------------

KO = (_SCANLINES - 192) * 76 / 64


TimerOS             .byte 36-KO           ; NTSC           262
                    .byte (36+29)      ; PAL            312
                    .byte 36-KO           ; SECAM          262
                    .byte 36-KO           ; PAL60          262

TimerVB
                    .byte 43           ; NTSC           262
                    .byte (43+30)      ; PAL            312
                    .byte 43           ; SECAM          262
                    .byte 43           ; PAL60          262


; EOF