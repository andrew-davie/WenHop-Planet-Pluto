
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


_SK_PER             ds SAVEKEY_SIZE
_SK_UNL             ds SAVEKEY_SIZE
_SK_SLV             ds SAVEKEY_SIZE

_SK_LAST            ds 1
_SK_USED_SOLVES     ds 1
_SK_SEEN_TROPHY     ds 1
_SK_TOTAL_SOLVES    ds 1

_SK_RESET           ds 1            ; NOT saved; just a toggle

    ;---------------------------------------------

_RUN_FUNC           ds 1
_INTIM              ds 1            ; spare time --> ARM

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

_BUF_AUDV           ds 2
_BUF_AUDC           ds 2
_BUF_AUDF           ds 2


_BOARD_COLS = 40
_BOARD_ROWS = 22
_1ROW = _BOARD_COLS

    .align 4
_BOARD              ds _BOARD_COLS * _BOARD_ROWS + 4    ; extra for grab+1 in drawscreen "bug"




;------------------------------------------------------------------------------
; OVERLAID VARIABLES

; BUFFERS MUST BE LAST
; Reason: they share memory and anything after the shortest will be stomped
; Buffers are guaranteed 4-byte aligned/size, so quick clears can be performed

    MAC DEFBUF ; {size}, {name}
_BUF_{2}            ds {1} * _BUFFER_SIZE
    ENDM

END_BUFFERS SET 0


    ALIGN 4         ; allowing 32-bit word access to clearing buffers in ARM

_BUFFERS = *

;-------------------------------------------------------------------------------

    SEG.U GS_COPYRIGHT
    ORG _BUFFERS

    DEFBUF 2, COPYRIGHT_JUMP

    DEFBUF 1, COPYRIGHT_COLUPF
    DEFBUF 1, COPYRIGHT_COLUP0
    DEFBUF 1, COPYRIGHT_COLUBK

    DEFBUF 4, COPYRIGHT_PF
    DEFBUF 6, COPYRIGHT_GRP

    if * > END_BUFFERS
END_BUFFERS SET *
    endif


; ;-------------------------------------------------------------------------------

;     SEG.U GS_RASTER_BLEED
;     ORG _BUFFERS

;     DEFBUF 2, RASTER_BLEED_JUMP
;     DEFBUF 6, RASTER_BLEED_GRP
;     DEFBUF 2, RASTER_BLEED_PF
;     DEFBUF 1, RASTER_BLEED_COLUPF
;     DEFBUF 1, RASTER_BLEED_COLUP0
;     DEFBUF 1, RASTER_BLEED_COLUBK

;     if * > END_BUFFERS
; END_BUFFERS SET *
;     endif


;-------------------------------------------------------------------------------


    SEG.U GS_SKULL
    ORG _BUFFERS

    DEFBUF 2, SKULL_JUMP

_SKULL_BUFFERS_START = *

    DEFBUF 1, SKULL_COLUBK
    DEFBUF 1, SKULL_COLUPF
    DEFBUF 1, SKULL_COLUP0

    DEFBUF 4, SKULL_PF
    DEFBUF 6, SKULL_GRP

_SKULL_BUFFERS_SIZE = * - _SKULL_BUFFERS_START

    ; end of grouping (10)

    if * > END_BUFFERS
END_BUFFERS SET *
    endif




;-------------------------------------------------------------------------------


    SEG.U GS_GLOBE
    ORG _BUFFERS

    DEFBUF 2, GLOBE_JUMP

_GLOBE_BUFFERS_START = *

    DEFBUF 1, GLOBE_COLUBK
    DEFBUF 1, GLOBE_COLUPF
    DEFBUF 1, GLOBE_COLUP0

    DEFBUF 6, GLOBE_PF
    DEFBUF 6, GLOBE_GRP

_GLOBE_BUFFERS_SIZE = * - _GLOBE_BUFFERS_START

    ; end of grouping (10)

    if * > END_BUFFERS
END_BUFFERS SET *
    endif

;-------------------------------------------------------------------------------

    SEG.U GS_MENU
    ORG _BUFFERS

    DEFBUF 2, MENU_JUMP

_MENU_BUFFERS_START = *

    DEFBUF 1, MENU_COLUBK
    DEFBUF 1, MENU_COLUPF
    DEFBUF 1, MENU_COLUP0

    DEFBUF 4, MENU_PF
    DEFBUF 6, MENU_GRP

_MENU_BUFFERS_SIZE = * - _MENU_BUFFERS_START

    ; end of grouping (10)

    if * > END_BUFFERS
END_BUFFERS SET *
    endif

;-------------------------------------------------------------------------------

    SEG.U GS_GAME
    ORG _BUFFERS

    DEFBUF 2, GAME_JUMP

_GAME_BUFFERS_START = *

    DEFBUF 1, GAME_COLUBK       ; assumed 1st for gameState_Game memSet block
    DEFBUF 1, GAME_COLUPF
    DEFBUF 1, GAME_COLUP0
    DEFBUF 1, GAME_COLUP1

    DEFBUF 1, GAME_PF0_LEFT
    DEFBUF 1, GAME_PF1_LEFT
    DEFBUF 1, GAME_PF2_LEFT

    DEFBUF 1, GAME_PF0_RIGHT
    DEFBUF 1, GAME_PF1_RIGHT
    DEFBUF 1, GAME_PF2_RIGHT

    DEFBUF 1, GAME_GRP0
    DEFBUF 1, GAME_GRP1

_GAME_BUFFERS_SIZE = * - _GAME_BUFFERS_START

_P0_X               ds 1
_P1_X               ds 1

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
