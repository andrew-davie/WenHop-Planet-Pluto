_SK_GAME_ID         = 0x02
SK_SLOT             = 99                   ; TODO: reserve slot with Champ Games
SK_ADDRESS          = (SK_SLOT * 64)

    include "i2c_v2.3.inc"
    i2c_subs

;-------------------------------------------------------------------------------

WriteSaveKey
                    jsr SetupSaveKey
                    bcc noSKfound2
 
    ; write entire SaveKey buffer

                    ldx #0
.loopWriteSK        lda SK_START,x
                    jsr i2c_txbyte
                    inx
                    cpx #SK_BYTES
                    bcc .loopWriteSK

                    jsr i2c_stopwrite       ; terminate write and commit to memory

noSKfound2          rts

;-------------------------------------------------------------------------------

ReadSaveKey

    ; will exit with SK_ID = _SK_GAME_ID if valid

                    ldx #SK_BYTES               ; +1, includes _SK_RESET
                    lda #0
.clearFlagsKey      sta SK_START,x
                    dex
                    bpl .clearFlagsKey

    ; Note: _SK_ID = 0 at this point
    
    ; non-zero inits go here. The rest are already 0

                    lda #1
                    sta SK_ENABLE_ICC

verify              jsr SetupSaveKey
                    bcc noSKfound

                    jsr i2c_stopwrite
                    jsr i2c_startread

                    ldx #_SK_GAME_ID

                    jsr i2c_rxbyte
                    cmp #_SK_GAME_ID
                    bne .forceInitSK

        ; HANDLE RESET PRESS TO CLEAR SAVEKEY DATA

                    lda SWCHB                   ; clear
                    lsr                         ; reset -> c
                    bcs .verified

.forceInitSK        inc SK_RESET

                    ldx #_SK_GAME_ID
                    stx SK_ID

                    jsr i2c_stopread
                                        
                    jsr WriteSaveKey
                    jsr i2c_stopwrite
                    rts


.verified           stx SK_ID

                    ldx #1
.loopReadSK         jsr i2c_rxbyte
                    sta SK_START,x
                    inx
                    cpx #SK_BYTES
                    bcc .loopReadSK
 
noSKfound           jsr i2c_stopread
 
    ; SK present IFF SK_ID == _SK_GAME_ID
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

                    ldy #>_SK_START
                    sty DSPTR
                    clc
                    txa
                    adc #<_SK_START
                    sta DSPTR

                    lda #DSCOMM
                    cmp SK_START,x
                    bne writeSKandExit

                    rts


writeSKandExit      sta SK_START,x

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
                    lda SK_START,x
                    jsr i2c_txbyte
                    jsr i2c_stopwrite

.noSKfound  
                    ldx #FASTON
                    stx SETMODE

                    rts                    

; EOF
