	org  CURRENT_ORG
	rorg $f000


BANK0_START

;-------------------------------------------------------------------------------

    ; bank-0-only code

    include "_startup.asm"
    include "_runVectoredCode.asm"
    include "_saveKey.asm"

    ; kernels are moveable to other banks...

BANK_kernelDetectConsole = BANK0
    include "kernels/kernel_DetectConsole.asm"

;-------------------------------------------------------------------------------

    CHECK_OVERFLOW 0, $FF0

; ------------------------------------------------------------------------------
; CDFJ+ Footer (MUST BE *HERE*)

BANK0_HEADER = $17F0

	org  BANK0_HEADER
	rorg $FFF0

	dc   0, 0, 0, 0		; CDFJ Hotspots
	dc.l C_STACK		; $F4	C Stack
	dc.l C_CODE+1		; $F8	C Code (+1 for THUMB Mode)
	dc.w CartReset		; $FC	Reset
	dc.w CartReset		; $FE	BRK

;-------------------------------------------------------------------------------

CURRENT_ORG SET CURRENT_ORG + $1000

; EOF
