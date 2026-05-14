;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ CDFJ+ Template - Driver Ver 48
;@ ARM/6502 Hybrid Framework for Atari 2600
;@
;@ Craig Daniels - Gamax Software - 2026
;@ 
;@ Many thanks to John (johnnywc) for seeding this project
;@ and to JetSetIlly for helping iron out digital samples
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	PROCESSOR 6502

DISPLAY_SIZE		= 6400			;For current framework: 4352 for 32K ROM; 6400 for 64K and 128K; 9472 for 256K and 512K
ROM_SIZE		    = 64			;in kB - 32, 64, 128, 256 or 512

;**********************************************************************
; These must be BEFORE include of cdfjplus.h
; Because that file checks if FF_OFFSET defined and if not sets it to 0

FF_OFFSET		= 200			;Fast Fetch offset: 0 to 200
FF_LDX			= $A2			;Fast Fetch for LDX: $A9 = off, $A2 = on
FF_LDY			= $A0			;Fast Fetch for LDY: $A9 = off, $A0 = on

    ;cdfjplus.h must come AFTER system constants for FF_OFFSET to apply
    INCLUDE "cdfjplus.h"

;**********************************************************************


	INCLUDE "vcs.h"
	INCLUDE "macro.h"

    MAC CHECK_OVERFLOW ; {1} bank number, {2} bank size

.BANK_SIZE{1} = * - BANK{1}_START
.BANK_END{1} = *

        IF .BANK_SIZE{1} > {2}
            echo "Error: BANK", [{1}]d, "[", [{2}]d, "] --> overflow by", [* - BANK{1}_START - {2}]d, "bytes"
            err
        ELSE
        	echo "---- BANK", [{1}]d, "[", [{2}]d, "] -->", [.BANK_SIZE{1}]d, "bytes used, ", [{2} - .BANK_SIZE{1}]d, "bytes free"
        ENDIF
    ENDM


						; <WARNING> fast fetch macros may not work properly
;	INCLUDE "tia_constants.h"


_DD_BASE		    = $40000800		;DisplayData base exported into defines file and used in CDFJ routines
_WAV_BASE		    = _DD_BASE + _waveforms
_RAM_BASE		    = _DD_BASE + DISPLAY_SIZE
_DISPLAY_SIZE32		= DISPLAY_SIZE / 4

_SCANLINES    = 192   ; number of scanlines for the arena
_ICC_SCANLINES = _SCANLINES/3
ARENA_BUFFER_SIZE   =_SCANLINES    ; PF buffer size for largest arena


	;C Stack Pointer - leave space for IAR at top of memory
	if (ROM_SIZE == 32)
C_STACK = $40001FDC
DS_SIZE = 0						;auto-generate DS_SIZE and CH_SIZE
CH_SIZE = 0						;for later use in DD RAM allocation
	endif
	if (ROM_SIZE == 64 || ROM_SIZE == 128)
C_STACK = $40003FDC
DS_SIZE = 2048
CH_SIZE = 0
	endif
	if (ROM_SIZE == 256 || ROM_SIZE == 512)
C_STACK = $40007FDC
DS_SIZE = 2048
CH_SIZE = _SCANLINES
	endif




_SND_MODE_TIA		= 0
_SND_MODE_DPC		= 1
_SND_MODE_SAMPLE	= 2


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
; C-code vector equates
; see runFunc[] in main.c

_RUN_ARM_NULL           = 0
_RUN_ARM_INIT		    = 1
_RUN_ARM_VBLANK	        = 2
_RUN_ARM_OVERSCAN   	= 3


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ User Constants
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Auto-detect TV system types (also exported to C)
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

_TV_SYSTEM_NTSC            = 0
_TV_SYSTEM_PAL             = 1
_TV_SYSTEM_SECAM           = 2
_TV_SYSTEM_PAL60           = 3

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ 2600 RIOT RAM - A mere 128 bytes
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	SEG.U VARS
	org $80

scanSK          ds 1


kernel			ds 1		;<FRAMEWORK>
tvSystem		ds 1		;<FRAMEWORK>  see TV_TYPE_ definitions

soundMode		ds 1		;<FRAMEWORK>
soundSave		ds 1		;<FRAMEWORK>
colubk          ds 1
call_fn			ds 1		;<FRAMEWORK>

audv0			ds 1		;<FRAMEWORK>
audc0			ds 1		;<FRAMEWORK>
audf0			ds 1		;<FRAMEWORK>
audv1			ds 1		;<FRAMEWORK>
audc1			ds 1		;<FRAMEWORK>
audf1			ds 1		;<FRAMEWORK>


;------------------------------------------------------------------------------

jumpCodeRAM		    ds 10		    ; self-modifying bank routine call code

  ;      Code                  bytes [] = modified
  ; -----------------------------------------------------------------------
  ; cmp SelectBankX           0 = cmp, [1=SM_JumpBank_L],    2=high bank
  ; jsr .called_bank_routine  3 = jsr, [4=SM_JumpRoutine_L], [5=SM_JumpRoutine_H]
  ; cmp SelectBank0           6 = cmp, 7=low bank,           8=high bank
  ; rts                       9 = rts

SM_JumpBank_L      = (jumpCodeRAM + 1)
SM_JumpRoutine_L   = (jumpCodeRAM + 4)
SM_JumpRoutine_H   = (jumpCodeRAM + 5)

;------------------------------------------------------------------------------



test_position		ds 1		;only for demonstration of positioning method purposes

SAVEKEY_SIZE = ((100+7)/8)

    ; Local (zp) vars where SK is initially read to on startup
    ; These are transferred to ARM/C via transfer to "RAM" variables via DSPTR setup and then DSWRITEs


SK_START
SAVEKEY_IDENT          ds 1
SAVEKEY_PERFECT        ds SAVEKEY_SIZE
SAVEKEY_UNLOCKED       ds SAVEKEY_SIZE
SAVEKEY_SOLVED         ds SAVEKEY_SIZE
SAVEKEY_LAST           ds 1
SAVEKEY_USED_SOLVES    ds 1
SAVEKEY_ENABLE_ICC     ds 1
SAVEKEY_SEEN_TROPHY    ds 1
SAVEKEY_TOTAL_SOLVES   ds 1
SAVEKEY_COUNT          ds 2
SK_END

    ; following must be contiguous to above...

SAVEKEY_RESET          ds 1            ; 0 = not reset, else SK was initialised

scanline                ds 1


	;Display Remaining RAM
	echo "---- 2600 RAM [  128 ] -->", (* - $80)d, "bytes used", ($100 - *)d, "bytes free" 


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ DisplayData RAM - Begins at $0000 and extends by DISPLAY_SIZE bytes
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	SEG.U DISPLAYDATA
	org $0000				;@@@@@ 256 Bytes: 6502 <-> ARM @@@@@

_DS_SK

_SK_ID              ds 1
_SK_PER             ds SAVEKEY_SIZE
_SK_UNL             ds SAVEKEY_SIZE
_SK_SLV             ds SAVEKEY_SIZE
_SK_LAST            ds 1
_SK_USED_SOLVES     ds 1
_SK_ENABLE_ICC      ds 1
_SK_SEEN_TROPHY     ds 1
_SK_TOTAL_SOLVES    ds 1
_SK_COUNT           ds 2

_SK_RESET           ds 1


_RUN_FUNC		ds 1			; <FRAMEWORK> (can now move as DSPTR hi=0 not assumed)
_SWCHA			ds 1			; <FRAMEWORK>
_SWCHB			ds 1			; <FRAMEWORK>
_INPT4			ds 1			; <FRAMEWORK>
_INPT5			ds 1			; <FRAMEWORK>

	align 2

_kernel			ds 1			; <FRAMEWORK>   see GAME_KERNEL definitions
_tvSystem		ds 1			; <FRAMEWORK>  see TV_TYPE_ definitions
_soundMode		ds 1			; <FRAMEWORK>
_colubk         ds 1

_AUDV0			ds 2			; <FRAMEWORK>
_AUDC0			ds 2			; <FRAMEWORK>
_AUDF0			ds 2			; <FRAMEWORK>
; _AUDV1			ds 1			; <FRAMEWORK>
; _AUDC1			ds 1			; <FRAMEWORK>
; _AUDF1			ds 1			; <FRAMEWORK>

_save_command		ds 1			; <SaveKey>
_save_count		ds 1			; <SaveKey>
_save_addr_l		ds 1			; <SaveKey>
_save_addr_h		ds 1			; <SaveKey>
_save_offset		ds 1			; <SaveKey>
_save_data		ds 64			; <SaveKey>

_P1_X               ds 1        ; position of player 1
_P0_X               ds 1        ; position of player 0



			;173 bytes free for user data here
			;things such as player positions,
			;state, flags, etc.

	org $0100
_waveforms		ds 256			;@@@@@ 256 Bytes: 8 Custom Waveforms (0-7) @@@@@

_digital_sample		ds DS_SIZE		;@@@@@ 2048 Bytes: Digital Sound Sample (on RAM >= 16k) @@@@@
						;@@@@@ playback access via waveform ID 8 @@@@@

;_buffer0		ds _SCANLINES			;@@@@@ 16x 192 Byte DS Channels @@@@@
; _buffer1		ds _SCANLINES
; _buffer2		ds _SCANLINES
; _buffer3		ds _SCANLINES
; _buffer4		ds _SCANLINES
; _buffer5		ds _SCANLINES
; _buffer6		ds _SCANLINES
; _buffer7		ds _SCANLINES
; _buffer8		ds _SCANLINES
; _buffer9		ds _SCANLINES
; _buffer10		ds _SCANLINES
; _buffer11		ds _SCANLINES
; _buffer12		ds _SCANLINES
; _buffer13		ds _SCANLINES
; _buffer14		ds _SCANLINES
; _buffer15		ds _SCANLINES

; _buffer16		ds CH_SIZE		;@@@@@ 16x additional 192 Byte DS Channels (on RAM = 32k) @@@@@
; _buffer17		ds CH_SIZE
; _buffer18		ds CH_SIZE
; _buffer19		ds CH_SIZE
; _buffer20		ds CH_SIZE
; _buffer21		ds CH_SIZE
; _buffer22		ds CH_SIZE
; _buffer23		ds CH_SIZE
; _buffer24		ds CH_SIZE
; _buffer25		ds CH_SIZE
; _buffer26		ds CH_SIZE
; _buffer27		ds CH_SIZE
; _buffer28		ds CH_SIZE
; _buffer29		ds CH_SIZE
; _buffer30		ds CH_SIZE
; _buffer31		ds CH_SIZE

_jump_table_1		ds 384
_jump_table_2		ds 384



;------------------------------------------------------------------------------
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
    DEFBUF COPYRIGHT_PF2_LEFT;
    DEFBUF COPYRIGHT_PF2_RIGHT;
    DEFBUF COPYRIGHT_COLUPF;
    DEFBUF COPYRIGHT_COLUP0;

    if * > _END_BUFFERS
_END_BUFFERS SET *
    endif

;------------------------------------------------------------------------------

    org _END_BUFFERS
    ; more vars here if required (but strange - put them before buffers!)

;------------------------------------------------------------------------------

	IF (* <= DISPLAY_SIZE)
    	echo "----- DISPLAY RAM [", (DISPLAY_SIZE)d, "] -->",(*)d, "bytes used,",(DISPLAY_SIZE - *)d , "bytes free"
	ELSE
	    echo "FATAL ERROR - Display Data exceeds",(DISPLAY_SIZE)d ,"bytes"
	    err
	ENDIF  





;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Cartridge Layout
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

;@ $0000 - $07ff CDF Driver
;@ $0800 - $17ff Bank 0, 6507 code, cartridge always boots with this bank active
;@ $1800 - $XXFF Bank 1-n, 6507 banks in blocks of 4K
;@               immediately followed by ARM C-code and data
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


        
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ CDF driver - The Harmony/Melody driver is located at Start of Cartridge ROM    
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_ORG SET 0

	SEG CODE
	org CURRENT_ORG
    

CDFJPLUS_DRIVER


    ; 2K, located at start of ROM

	incbin "./cdfjplus48_p1.bin"
    .byte FF_LDX
	incbin "./cdfjplus48_p2.bin"
    .byte FF_LDY
	incbin "./cdfjplus48_p3.bin"
	.byte FF_OFFSET
	incbin "./cdfjplus48_p4.bin"


CDFJPLUS_DRIVER_SIZE = [* - CDFJPLUS_DRIVER]d
	echo "---- CDFJPLUS DRIVER SIZE", CDFJPLUS_DRIVER_SIZE, "bytes"


CURRENT_ORG SET CURRENT_ORG + $800



;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Now we have 4K banks for 6502 code
;@ Bank 0 is the startup bank
;@ It requires a 'footer' for CDFJ+
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	include "bank_0.asm"

    ; --------------------------------------------------------------------------
    ; Bank 0 Footer - needed for CDFJ+ to function

BANK0_HEADER = $17F0

	org BANK0_HEADER
	rorg $FFF0

	DC.B 0, 0, 0, 0		;CDFJ Hotspots
	DC.L C_STACK		;$F4	C Stack
	DC.L C_CODE+1		;$F8	C Code (+1 for THUMB Mode)
	DC.W CartReset		;$FC	Reset
	DC.W CartReset		;$FE	BRK


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Other banks...
    
	include "bank_1.asm"
	include "bank_2.asm"
; 	include "bank_3.asm"
; 	include "bank_4.asm"
; 	include "bank_5.asm"
; 	include "bank_6.asm"

;    include "champKernel.asm"

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ C-Code
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    org CURRENT_ORG
    rorg CURRENT_ORG

    ; Note: c_start tool parses the "bootstrap_defines"-generated symbol table/file
    ; for the label C_CODE, which will hold the address to write to the LDS file

C_CODE
	INCBIN "main/bin/cdfj+.bin"

    ; Make ROM the correct size 
ROM_BYTES = ROM_SIZE * 1024 
	echo "---- C CODE [", [ROM_BYTES]d, "] --> ", (* - C_CODE)d, "bytes used,", [ROM_BYTES - *]d , "bytes free"
    if * < ROM_BYTES
	    org ROM_BYTES - 1
    	.byte 0
    endif

; EOF
