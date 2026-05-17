
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ DisplayData RAM - Begins at $0000 and extends by DISPLAY_SIZE bytes
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	SEG.U DISPLAYDATA
	org $0000                       ; 6502 <-> ARM


    ;---------------------------------------------
    ; SaveKey
    
_DS_SK

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

_kernel             ds 1        ; see GAME_KERNEL definitions
_tvSystem           ds 1        ; see TV_TYPE_ definitions
_soundMode          ds 1
_colubk             ds 1

_AUDV0              ds 2
_AUDC0              ds 2
_AUDF0              ds 2

_P1_X               ds 1        ; position of player 1
_P0_X               ds 1        ; position of player 0



			; 173 bytes free for user data here
			; things such as player positions,
			; state, flags, etc.

; 	org $0100
; _waveforms		    ds 256			;@@@@@ 256 Bytes: 8 Custom Waveforms (0-7) @@@@@

; _digital_sample		ds DS_SIZE		;@@@@@ 2048 Bytes: Digital Sound Sample (on RAM >= 16k) @@@@@
						;@@@@@ playback access via waveform ID 8 @@@@@


_jump_table_1		ds _SCANLINES * 2
_jump_table_2		ds _SCANLINES * 2



;------------------------------------------------------------------------------
; OVERLAID VARIABLES

; BUFFERS MUST BE LAST
; Reason: they share memory and anything after the shortest will be stomped

BUFN SET 0
    MAC DEFBUF ;name
_BUF_{1}             ds _SCANLINES
BUFN SET BUFN + 1
    ENDM

_END_BUFFERS SET 0


    ALIGN 4         ; allowing 32-bit word access to clearing buffers in ARM

_BUFFERS = *

;-------------------------------------------------------------------------------

    SEG.U GS_RAINBOW
    ORG _BUFFERS
    DEFBUF RAINBOW_COLUBK

    if * > _END_BUFFERS
_END_BUFFERS SET *
    endif

;-------------------------------------------------------------------------------

    SEG.U GS_COUCH_COMPLIANT
    ORG _BUFFERS

    DEFBUF COUCH_COMPLIANT_COLUP0
    DEFBUF COUCH_COMPLIANT_GRP0A

    if * > _END_BUFFERS
_END_BUFFERS SET *
    endif

;-------------------------------------------------------------------------------

    SEG.U GS_COPYRIGHT
    ORG _BUFFERS

    DEFBUF COPYRIGHT_GRP0A
    DEFBUF COPYRIGHT_PF2_LEFT
    DEFBUF COPYRIGHT_PF2_RIGHT
    DEFBUF COPYRIGHT_COLUPF
    DEFBUF COPYRIGHT_COLUP0

    if * > _END_BUFFERS
_END_BUFFERS SET *
    endif

;------------------------------------------------------------------------------

    org _END_BUFFERS
    ; more vars here if required (but strange - put them before buffers!)

;------------------------------------------------------------------------------

	IF (_END_BUFFERS <= _DISPLAY_SIZE)
    	echo "----- DISPLAY RAM [", (_DISPLAY_SIZE)d, "] -->",(_END_BUFFERS)d, "bytes used,",(_DISPLAY_SIZE - _END_BUFFERS)d , "bytes free"
	ELSE
	    echo "FATAL ERROR - Display Data exceeds",(_DISPLAY_SIZE)d ,"bytes"
	    err
	ENDIF  

; EOF
