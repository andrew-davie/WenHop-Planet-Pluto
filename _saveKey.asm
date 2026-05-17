
;███████████████████████████████████████████████████████████████████████████████


skBuffer    = SK_ID            ; define the RAM address you want to store
SK_BYTES    = (SK_END - SK_START)

SK_SLOT = 99                   ; TODO: reserve slot!
SK_ADDRESS = (SK_SLOT * 64)
 
    include "i2c_v2.3.inc"      ; a highly optimized (for space) version   
    i2c_subs                    ; this makes the i2c macros of the include file known to the code 

;-------------------------------------------------------------------------------

WriteSaveKey
                    jsr SetupSaveKey
                    bcc noSKfound2
 
    ; write entire SaveKey buffer

                    ldx #0
.loopWriteSK        lda skBuffer,x
                    jsr i2c_txbyte
                    inx
                    cpx #SK_BYTES
                    bcc .loopWriteSK

                    jsr i2c_stopwrite       ; terminate write and commit to memory

noSKfound2          rts

;-------------------------------------------------------------------------------

ReadSaveKey

    ; pre-cleer flags buffers
    ; will exit with SK_ID = _SK_WENHOP_ID if valid

_WENHOP_SK_ID = 0xAB

                    ldx #SK_BYTES               ; (+1) to include _SK_RESET
                    lda #0
.clearFlagsKey      sta SK_ID,x
                    dex
                    bpl .clearFlagsKey

    ; Note: _SK_ID = 0 at this point
    
    ; non-zero inits go here. The rest are already 0

                    lda #1
                    sta SK_ENABLE_ICC
                    lda #$F2
                    sta SK_ODOMETER
                    sta SK_ODOMETER+1

verify              jsr SetupSaveKey
                    bcc noSKfound

                    jsr i2c_stopwrite
                    jsr i2c_startread

                    ldx #_WENHOP_SK_ID

                    jsr i2c_rxbyte
                    cmp #_WENHOP_SK_ID
                    bne .forceInitSK

        ; HANDLE RESET PRESS TO CLEAR SAVEKEY DATA

                    lda SWCHB                   ; clear
                    lsr                         ; reset -> c
                    bcs .verified

.forceInitSK        inc SK_RESET

                    ldx #_WENHOP_SK_ID
                    stx SK_ID

                    jsr i2c_stopread
                                        
                    jsr WriteSaveKey
                    jsr i2c_stopwrite
                    rts


.verified           stx SK_ID

                    ldx #1
.loopReadSK         jsr i2c_rxbyte
                    sta SK_ID,x
                    inx
                    cpx #SK_BYTES
                    bcc .loopReadSK
 
noSKfound           jsr i2c_stopread
 
    ; SK present IFF SK_ID == _WENHOP_SAVEKEYK_ID
    ; 0 if not present

                    rts

;------------------------------------------------------------------------------
SetupSaveKey

                    jsr i2c_startwrite      ; detect SaveKey
                    bne .exitSK

                    clv
                    lda #>SK_ADDRESS
                    jsr i2c_txbyte
                    lda #<SK_ADDRESS
                    jmp i2c_txbyte

.exitSK             clc
                    rts


;-------------------------------------------------------------------------------

scanAndUpdateSaveKey

    ; Does a diff on one SK byte per call (i.e., once per frame is ideal)
    ; Will write to SK if change detected

                    ldx scanSK
                    inx
                    cpx #SK_BYTES
                    bcc inRange
                    ldx #0
inRange             stx scanSK

                    ldy #>SK_ID
                    sty DSPTR
                    clc
                    txa
                    adc #<_SK_ID
                    sta DSPTR

                    lda #DSCOMM
                    cmp SK_ID,x
                    bne writeSKandExit

                    rts


writeSKandExit      sta SK_ID,x

                    ldx #FASTOFF
                    stx SETMODE


                    jsr i2c_startwrite
                    bne .noSKfound

                    clv
                    lda #>SK_ADDRESS
                    jsr i2c_txbyte

                    clc
                    lda scanSK
                    adc #<SK_ADDRESS
                    jsr i2c_txbyte

                    ldx scanSK
                    lda SK_ID,x
                    jsr i2c_txbyte
                    jsr i2c_stopwrite

.noSKfound  
                    ldx #FASTON
                    stx SETMODE

                    rts                    

; EOF
