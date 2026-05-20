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

_DISPLAY_SIZE		= 6400			; For current framework: 4352 for 32K ROM; 6400 for 64K and 128K; 9472 for 256K and 512K
ROM_SIZE		    = 128			; in kB - 32, 64, 128, 256 or 512

;**********************************************************************
; These must be BEFORE include of cdfjplus.h
; Because that file checks if FF_OFFSET defined and if not sets it to 0

FF_OFFSET		    = 200			; Fast Fetch offset: 0 to 200
FF_LDX			    = $A2			; Fast Fetch for LDX: $A9 = off, $A2 = on
FF_LDY			    = $A0			; Fast Fetch for LDY: $A9 = off, $A0 = on

    ;cdfjplus.h must come AFTER system constants for FF_OFFSET to apply
    INCLUDE "cdfjplus.h"

	INCLUDE "vcs.h"
	INCLUDE "macro.h"


_DD_BASE		    = $40000800		;DisplayData base exported into defines file and used in CDFJ routines
; _WAV_BASE		    = _DD_BASE + _waveforms
_RAM_BASE		    = _DD_BASE + _DISPLAY_SIZE


_SCANLINES    = 198   ; number of scanlines for the arena


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

_RUNARM_NULL           = 0
_RUNARM_SYSTEM_RESET   = 1
_RUNARM_LOAD_SAVEKEY   = 2
_RUNARM_VBLANK	        = 3
_RUNARM_OVERSCAN   	= 4


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


    include "_zeroPage.asm"
    include "_displayData.asm"


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Cartridge Layout
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

;@ $0000 - $07ff CDF Driver
;@ $0800 - $17ff Bank 0, 6507 code, cartridge always boots with this bank active
;@ $1800 - $XXFF Bank 1-n, 6507 banks in blocks of 4K
;@               immediately followed by ARM C-code and data
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


;-------------------------------------------------------------------------------
; 4K banks for 6502 code

	include "bank_0.asm"        ; <-- startup bank + footer for CDFJ+
	include "bank_1.asm"
	include "bank_2.asm"
 	include "bank_3.asm"
; 	include "bank_4.asm"
; 	include "bank_5.asm"
; 	include "bank_6.asm"

;-------------------------------------------------------------------------------

    org  CURRENT_ORG
    rorg CURRENT_ORG

C_CODE  ; <-- address placed in LDS by c_start tool

    incbin "main/bin/cdfj+.bin"

;-------------------------------------------------------------------------------

    ; Make ROM the correct size 

ROM_BYTES = ROM_SIZE * 1024 
	echo "---- C CODE [", [ROM_BYTES]d, "] --> ", (* - C_CODE)d, "bytes used,", [ROM_BYTES - *]d , "bytes free"
    if * < ROM_BYTES
	    org ROM_BYTES - 1
    	.byte 0
    endif

; EOF
