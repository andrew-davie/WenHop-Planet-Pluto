
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ DisplayData RAM - Begins at $0000 and extends by DISPLAY_SIZE bytes
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	SEG.U DISPLAYDATA
	org $0000                       ; 6502 <-> ARM


    ;---------------------------------------------
    ; SaveKey
    
_SK_START

_SK_ID              ds 1
_SK_ENABLE_ICC      ds 1
_SK_ODOMETER        ds 2           ; how many games played!

_SK_RESET           ds 1            ; NOT saved; just a toggle

    ;---------------------------------------------

_RUN_FUNC           ds 1
_SWCHA	            ds 1
_SWCHB	            ds 1
_INPT4              ds 1
_INPT5              ds 1

    ;---------------------------------------------
    ; ordering assumed in transfer of vars from ARM...

_kernel             ds 1        ; see GAME_KERNEL definitions
_tvSystem           ds 1        ; see TV_TYPE_ definitions
_soundMode          ds 1
_colubk             ds 1

    ;---------------------------------------------

_BUF_AUDV               ds 2
_BUF_AUDC               ds 2
_BUF_AUDF               ds 2

;------------------------------------------------------------------------------
; OVERLAID VARIABLES

; BUFFERS MUST BE LAST
; Reason: they share memory and anything after the shortest will be stomped

    MAC DEFBUF ; {size}, {name}
_BUF_{2}             ds {1} * _SCANLINES
    ENDM

END_BUFFERS SET 0


    ALIGN 4         ; allowing 32-bit word access to clearing buffers in ARM

_BUFFERS = *

;-------------------------------------------------------------------------------

    SEG.U GS_RAINBOW
    ORG _BUFFERS

    DEFBUF 2, RB_JUMP
    DEFBUF 1, RB_COLUBK

    if * > END_BUFFERS
END_BUFFERS SET *
    endif


;-------------------------------------------------------------------------------

    SEG.U GS_COPYRIGHT
    ORG _BUFFERS

    DEFBUF 2, COPYRIGHT_JUMP
    DEFBUF 6, COPYRIGHT_GRP
    DEFBUF 2, COPYRIGHT_PF
    DEFBUF 1, COPYRIGHT_COLUPF
    DEFBUF 1, COPYRIGHT_COLUP0

    if * > END_BUFFERS
END_BUFFERS SET *
    endif


;-------------------------------------------------------------------------------

    SEG.U GS_MENU
    ORG _BUFFERS

    DEFBUF 2, MENU_JUMP

    DEFBUF 1, MENU_COLUBK
    DEFBUF 1, MENU_COLUPF
    DEFBUF 1, MENU_COLUP0

    ; grouping important due to clear in menu

    ; Order of these 4 important...
    ; TODO: combine to 4 long

    DEFBUF 4, MENU_PF

    ; TODO:  combine to 6-long

    DEFBUF 1, MENU_GRP0A
    DEFBUF 1, MENU_GRP1A
    DEFBUF 1, MENU_GRP0B
    DEFBUF 1, MENU_GRP1B
    DEFBUF 1, MENU_GRP0C
    DEFBUF 1, MENU_GRP1C

    ; end of grouping (10)

    if * > END_BUFFERS
END_BUFFERS SET *
    endif


;------------------------------------------------------------------------------

    org END_BUFFERS
    ; more vars here if required (but strange - put them before buffers!)

;------------------------------------------------------------------------------

	IF (END_BUFFERS <= _DISPLAY_SIZE)
    	echo "----- DISPLAY RAM [", (_DISPLAY_SIZE)d, "] -->",(END_BUFFERS)d, "bytes used,",(_DISPLAY_SIZE - END_BUFFERS)d , "bytes free"
	ELSE
	    echo "FATAL ERROR - Display Data exceeds",(_DISPLAY_SIZE)d ,"bytes"
	    err
	ENDIF  

; EOF
