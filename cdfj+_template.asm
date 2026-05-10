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

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ CDFJ+ System Constants and Includes
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


DISPLAY_SIZE		= 6400			;For current framework: 4352 for 32K ROM; 6400 for 64K and 128K; 9472 for 256K and 512K
ROM_SIZE		= 128			;in kB - 32, 64, 128, 256 or 512
FF_LDX			= 1			;Fast Fetch for LDX: 0 = off, NZ = on
FF_LDY			= 1			;Fast Fetch for LDY: 0 = off, NZ = on
FF_OFFSET		= 200			;Fast Fetch offset: 0 to 200
C_START			= $7800			;$1800, $2800, $3800, $4800, $5800, $6800, $7800
						; With current examples, C_START=$1800 will error because Bank1 contains 6507 routines.
						; Move/remove those routines to Bank0 before starting ARM C at $1800.

	INCLUDE "cdfjplus.h"			;cdfjplus.h must come AFTER system constants for FF_OFFSET to apply
	INCLUDE "vcs.h"
	INCLUDE "macro.h"

						; <WARNING> fast fetch macros may not work properly
;	INCLUDE "tia_constants.h"



_DD_BASE		= $40000800		;DisplayData base exported into defines file and used in CDFJ routines
_WAV_BASE		= _DD_BASE + _waveforms
_RAM_BASE		= _DD_BASE + DISPLAY_SIZE
_DISPLAY_SIZE32		= DISPLAY_SIZE / 4

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
CH_SIZE = 192
	endif

_SND_MODE_TIA		= 0
_SND_MODE_DPC		= 1
_SND_MODE_SAMPLE	= 2


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
; C-code vector equates
; ignore --> Ordering/values important - see VectorToHandler in main.c

_RUN_NULL               = 0
_RUN_ARM_INIT		    = 1
_RUN_ARM_VBLANK	        = 2
_RUN_ARM_OVERSCAN   	= 3


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ User Constants
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

UPPER_BLANK_TIMER_PAL60	= (43+30)
LOWER_BLANK_TIMER_PAL60	= (35+29)

UPPER_BLANK_TIMER_PAL	= 50
LOWER_BLANK_TIMER_PAL	= 45

UPPER_BLANK_TIMER_NTSC	= 43
LOWER_BLANK_TIMER_NTSC	= 35

UPPER_BLANK_TIMER_SECAM	= 43
LOWER_BLANK_TIMER_SECAM	= 35

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
tv_system		ds 1		;<FRAMEWORK>  see TV_TYPE_ definitions

sound_mode		ds 1		;<FRAMEWORK>
sound_save		ds 1		;<FRAMEWORK>
call_fn			ds 1		;<FRAMEWORK>

audv0			ds 1		;<FRAMEWORK>
audc0			ds 1		;<FRAMEWORK>
audf0			ds 1		;<FRAMEWORK>
audv1			ds 1		;<FRAMEWORK>
audc1			ds 1		;<FRAMEWORK>
audf1			ds 1		;<FRAMEWORK>

.jump_code_RAM		ds 1		;dedicated area of RAM for bank routine jumping/calling	<FRAMEWORK>
.jump_code_RAM_t_bank	ds 3		;cmp SelectBankX					<FRAMEWORK>
.jump_code_RAM_t_r_l	ds 1		;jsr .called_bank_routine				<FRAMEWORK>
.jump_code_RAM_t_r_h	ds 2		;cmp SelectBank0					<FRAMEWORK>
.jump_code_RAM_r_bank	ds 3		;rts							<FRAMEWORK>

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



	;Display Remaining RAM
	echo "---- 2600 RAM", ($100 - *)d, "bytes free" 


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

_kernel			ds 1			; <FRAMEWORK>
_tv_system		ds 1			; <FRAMEWORK>  see TV_TYPE_ definitions
_sound_mode		ds 1			; <FRAMEWORK>
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




			;173 bytes free for user data here
			;things such as player positions,
			;state, flags, etc.

	org $0100
_waveforms		ds 256			;@@@@@ 256 Bytes: 8 Custom Waveforms (0-7) @@@@@

_digital_sample		ds DS_SIZE		;@@@@@ 2048 Bytes: Digital Sound Sample (on RAM >= 16k) @@@@@
						;@@@@@ playback access via waveform ID 8 @@@@@

_buffer0		ds 192			;@@@@@ 16x 192 Byte DS Channels @@@@@
; _buffer1		ds 192
; _buffer2		ds 192
; _buffer3		ds 192
; _buffer4		ds 192
; _buffer5		ds 192
; _buffer6		ds 192
; _buffer7		ds 192
; _buffer8		ds 192
; _buffer9		ds 192
; _buffer10		ds 192
; _buffer11		ds 192
; _buffer12		ds 192
; _buffer13		ds 192
; _buffer14		ds 192
; _buffer15		ds 192

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


	IF (* <= DISPLAY_SIZE)
	echo "------",(DISPLAY_SIZE - *)d , "bytes of Display Data RAM left"
	ELSE
	echo "FATAL ERROR - Display Data exceeds",(DISPLAY_SIZE)d ,"bytes"
	err
	ENDIF  


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Cartridge Layout	(with C_START = $7800)
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ $0000 - $07ff CDF Driver
;@ $0800 - $17ff Bank 0, 6507 code, cartridge always boots with this bank active
;@ $1800 - $27ff Bank 1, 6507 code
;@ $2800 - $37ff Bank 2, 6507 code
;@ $3800 - $47ff Bank 3, 6507 code
;@ $4800 - $57ff Bank 4, 6507 code
;@ $5800 - $67ff Bank 5, 6507 code
;@ $6800 - $77ff Bank 6, 6507 code
;@ $7800 - $87ff Bank 7, Starting location of ARM C-code and data
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Cartridge Layout	(with C_START = $1800)
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ $0000 - $07ff CDF Driver
;@ $0800 - $17ff Bank 0, 6507 code, cartridge always boots with this bank active
;@ $1800 - $27ff Bank 1, Starting location of ARM C-code and data
;@ $2800 - $37ff Bank 2, ARM C-code and data
;@ $3800 - $47ff Bank 3, ARM C-code and data
;@ $4800 - $57ff Bank 4, ARM C-code and data
;@ $5800 - $67ff Bank 5, ARM C-code and data
;@ $6800 - $77ff Bank 6, ARM C-code and data
;@ $7800 - $87ff Bank 7, ARM C-code and data
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


        
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ CDF driver - The Harmony/Melody driver is located at Start of Cartridge ROM    
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	SEG CODE
	org $0000
    
CDFJPLUS_DRIVER:

	incbin "./cdfjplus48_p1.bin"

	if FF_LDX = 0
	.byte #$a9
	else
	.byte #$a2
	endif

	incbin "./cdfjplus48_p2.bin"

	if FF_LDY = 0
	.byte #$a9
	else
	.byte #$a0
	endif

	incbin "./cdfjplus48_p3.bin"

	.byte #FF_OFFSET

	incbin "./cdfjplus48_p4.bin"


CDFJPLUS_DRIVER_SIZE = [* - CDFJPLUS_DRIVER]d
	echo "---- CDFJPLUS DRIVER SIZE", CDFJPLUS_DRIVER_SIZE, "bytes"



; TODO: Check changes to CSTART rorg/org usage below
;  instead of the conditional weirdness

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Bank 0 - Startup bank
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_BANK set $0800
	ORG CURRENT_BANK

	.include "bank_0.asm"

	; if C_START = $1800
	; org $1800
	; rorg $1800

	; else

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Bank 1
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_BANK set $1800
	ORG CURRENT_BANK

	.include "bank_1.asm"

	; if C_START = $2800
	; org $2800
	; rorg $2800

	; else

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Bank 2
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_BANK set $2800
	ORG CURRENT_BANK

	.include "bank_2.asm"

	; if C_START = $3800
	; org $3800
	; rorg $3800

	; else

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Bank 3
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_BANK set $3800
	ORG CURRENT_BANK

	.include "bank_3.asm"

	; if C_START = $4800
	; org $4800
	; rorg $4800

	; else

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Bank 4
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_BANK set $4800
	ORG CURRENT_BANK

	.include "bank_4.asm"

	; if C_START = $5800
	; org $5800
	; rorg $5800

	; else

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Bank 5
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_BANK set $5800
	ORG CURRENT_BANK

	.include "bank_5.asm"

	; if C_START = $6800
	; org $6800
	; rorg $6800

	; else

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Bank 6
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

CURRENT_BANK set $6800
	ORG CURRENT_BANK

	.include "bank_6.asm"


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ C-Code
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	; org $7800
	; rorg $7800

	; endif
	; endif
	; endif
	; endif
	; endif
	; endif


    org C_START
    rorg C_START


C_CODE:
	INCBIN "main/bin/cdfj+.bin"






C_CODE_SIZE = * - C_CODE;
	echo "---- C CODE uses", (C_CODE_SIZE)d, "bytes"

 
	IF ROM_SIZE = 32
	echo "----",($8000 - *) , "C CODE bytes free"
	org $7fff
	ENDIF

	IF ROM_SIZE = 64
	echo "----",($10000 - *) , "C CODE bytes free"
	org $ffff
	ENDIF

	IF ROM_SIZE = 128
	echo "----",($20000 - *) , "C CODE bytes free"
	org $1ffff
	ENDIF

	IF ROM_SIZE = 256
	echo "----",($40000 - *) , "C CODE bytes free"
	org $3ffff
	ENDIF

	IF ROM_SIZE = 512
	echo "----",($80000 - *) , "C CODE bytes free"
	org $7ffff
	ENDIF

	.byte 0


    
