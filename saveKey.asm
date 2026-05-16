
;███████████████████████████████████████████████████████████████████████████████


skBuffer    = SAVEKEY_IDENT      ; define the RAM address you want to store
SK_BYTES    = (SK_END - SK_START)
;3 * SAVEKEY_SIZE + 1               ; define how many bytes your want to store
SAVEKEY_ADR = $18c0             ; slot 99?
 
    include "i2c_v2.3.inc"      ; a highly optimized (for space) version   
    i2c_subs                    ; this makes the i2c macros of the include file known to the code 

;-------------------------------------------------------------------------------
WriteSaveKey SUBROUTINE         ; total cycles = 1923 (for 3 bytes)
;-------------------------------------------------------------------------------
; setup SaveKey:
    jsr     SetupSaveKey        ; 6+927
    bcc     .noSKfound          ; 2/3
 
; write high score:
    ldx     #0         ; 2 = 937   
.loopWriteSK
    lda     skBuffer,x          ; 4
    jsr     i2c_txbyte          ;6+296      transmit to EEPROM
    inx                         ; 2
    cpx #SK_BYTES
    bcc     .loopWriteSK        ; 2/3=932

; stop write:
    jsr     i2c_stopwrite       ; 6+42=48   terminate write and commit to memory
.noSKfound
    rts                         ; 6

;-------------------------------------------------------------------------------
ReadSaveKey SUBROUTINE          ; total cycles = 2440 (for 3 bytes)
;-------------------------------------------------------------------------------
; setup SaveKey:


    ; pre-cleer flags buffers
    ; will exit with SAVEKEY_IDENT = _SK_WENHOP_ID if valid

_WENHOP_SK_ID = 0xA7

                    ldx #SK_BYTES               ; (+1) to include _SAVEKEY_RESET
                    lda #0
.clearFlagsKey      sta SAVEKEY_IDENT,x
                    dex
                    bpl .clearFlagsKey

    ; Note: _SAVEKEY_IDENT = 0 at this point
    ; Now unlock the first 10 levels
    
                    lda #%11111111
                    sta SAVEKEY_UNLOCKED
                    lda #%11
                    sta SAVEKEY_UNLOCKED+1

    ; non-zero inits go here. The rest are already 0

                    lda #1
                    sta SAVEKEY_TOTAL_SOLVES
                    sta SAVEKEY_ENABLE_ICC

.verify             jsr SetupSaveKey
                    bcc .noSKfound

                    jsr i2c_stopwrite
                    jsr i2c_startread

                    ldx #_WENHOP_SK_ID

                    jsr i2c_rxbyte
                    cmp #_WENHOP_SK_ID
                    bne .forceInitSK

        ; HANDLE RESET PRESS TO CLEAR SAVEKEY DATA

                    lda SWCHB                   ; clear
                    lsr                         ; reset -> c
                    bcs .verifiedGridLock

                    ; ldx #$26
                    ; stx COLUBK

.forceInitSK        inc SAVEKEY_RESET

                    ldx #_WENHOP_SK_ID
                    stx SAVEKEY_IDENT

                    jsr i2c_stopread
                                        
                    jsr WriteSaveKey
                    jsr i2c_stopwrite
                    rts ;jmp .verify


.verifiedGridLock

                    stx SAVEKEY_IDENT

                    ldx #1
.loopReadSK         jsr i2c_rxbyte
                    sta SAVEKEY_IDENT,x
                    inx
                    cpx #SK_BYTES
                    bcc .loopReadSK
 
.noSKfound          jsr i2c_stopread
 
    ; SK present IFF SAVEKEY_IDENT == _WENHOP_SK_ID
    ; 0 if not present

                    rts

;------------------------------------------------------------------------------
SetupSaveKey SUBROUTINE         ; = 927
;------------------------------------------------------------------------------

                    ; lda #0
                    ; sta scanSK


; detect SaveKey:
    jsr     i2c_startwrite      ;6+312
    bne     .exitSK             ; 2/3
; setup address:
    clv                         ; 2
    lda     #>SAVEKEY_ADR       ; 2         upper byte of address
    jsr     i2c_txbyte          ;6+296
    lda     #<SAVEKEY_ADR       ; 2         lower byte offset
    jmp     i2c_txbyte          ;3+296      returns C==1

.exitSK
    clc
    rts

; 176 bytes in total (less if you inline the subroutines)


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

scanAndUpdateSaveKey

    ; Does a diff on one SK byte per call (i.e., once per frame is ideal)
    ; Will write to SK if change detected

                    ldx scanSK
                    inx
                    cpx #SK_BYTES
                    bcc inRange
                    ldx #0
inRange             stx scanSK

                    ldy #>_SK_ID
                    sty DSPTR
                    clc
                    txa
                    adc #<_SK_ID
                    sta DSPTR

                    lda #DSCOMM
                    cmp SAVEKEY_IDENT,x
                    bne writeSKandExit

                    ; inx
                    ; cpx #SK_BYTES
                    ; bcc scanExit
                    ; ldx #0
;scanExit            stx scanSK
                    rts


writeSKandExit      sta SAVEKEY_IDENT,x

                    ldx #FASTOFF
                    stx SETMODE


                    jsr i2c_startwrite
                    bne .noSKfound

                    clv
                    lda #>SAVEKEY_ADR
                    jsr i2c_txbyte

                    clc
                    lda scanSK
                    adc #<SAVEKEY_ADR
                    jsr i2c_txbyte

                    ldx scanSK
                    lda SAVEKEY_IDENT,x
                    jsr i2c_txbyte
                    jsr i2c_stopwrite

.noSKfound  
                    ldx #FASTON
                    stx SETMODE

                    rts                    


; EOF
