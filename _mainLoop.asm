; Work on 262 scanlines/frame
; 70MHz --> 60Hz frames --> 1,166,666 cycles/frame
; --> 4452 cycles/scanlines
;
; 198 lines of displahy
; 64 lines VB and OS
; divide evenly; 32 lines each
; --> 142464 cycles
;
;
; VB:  32 lines - 3 vsync = 29 * 76 cycles / 64 --> TIM64T 34
; OS:  32 lines = 30 * 76 / 64 -> TIM64T 35






mainGameLoop

                	lda #2
                    sta WSYNC
                    sta VBLANK                  ; overscan!

                    ldx tvSystem
                    lda TimerOS,x
                    sta TIM64T


                    jsr scanAndUpdateSaveKey            ; update ONE *CHANGED* SaveKey byte


    ; run kernel-specific 6502 Overscan code (OS_FN_OFFSET) 

                    lda kernel
                    clc
                    adc #OS_FN_OFFSET
                    tax
                    jsr runVectoredCode               ; actually run 6502 kernel-specific VB


    ; call ARM Overscan

                	ldx #>_RUN_FUNC
                	stx DSPTR
                	ldx #<_RUN_FUNC
                	stx DSPTR

                	ldx #_RUNARM_OVERSCAN
                	stx DSWRITE

                	lda INTIM
                    ; sec
                    ; sbc #5
                	sta DSWRITE

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

.waitOS             lda INTIM	
                    bne .waitOS

    ; vertical blank
    ; any kernel switching will have just happened

.overtime           ldx #%1110
                    txa
.verticalSync       sta WSYNC
                    sta VSYNC                   ; indicate vblank
                    lsr 
                    bne .verticalSync 


                    ldx tvSystem
                    lda TimerVB,x
                    sta TIM64T

    ; run kernel-specific 6502 Vertical Blank code (VB_FN_OFFSET)

                    lda kernel
                    clc
                    adc #VB_FN_OFFSET
                    tax
                    jsr runVectoredCode               ; actually run 6502 kernel-specific VB

                    ldx #>_RUN_FUNC
                    stx DSPTR
                    ldx #<_RUN_FUNC
                    stx DSPTR
                    ldx #_RUNARM_VBLANK
                    stx DSWRITE

                	lda INTIM
                    ; sec
                    ; sbc #5
                	sta DSWRITE

                    ldx call_fn
                    stx CALLFN                  ; call VerticalBlank in ARM


.waitVB             lda INTIM
                    bne .waitVB

                    sta WSYNC
                    sta VBLANK                  ; screen on

                    ldx kernel
                    jsr runVectoredCode               ; run 6502 kernel

                    jmp mainGameLoop

;-------------------------------------------------------------------------------

KO = (_SCANLINES - 192) * 76 / 64


TimerOS             .byte 38          ; NTSC           262
                    .byte (36+29+4)      ; PAL            312
                    .byte 38           ; SECAM          262
                    .byte 38           ; PAL60          262

TimerVB
                    .byte 31           ; NTSC           262
                    .byte 31      ; PAL            312
                    .byte 31           ; SECAM          262
                    .byte 31           ; PAL60          262


; EOF