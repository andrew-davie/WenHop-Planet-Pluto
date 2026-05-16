	org  CURRENT_ORG
	rorg $f000

.BANK SET  BANK0
BANK0_START

;-------------------------------------------------------------------------------

    include "startup.asm"
    include "runVectoredCode.asm"
    include "saveKey.asm"
    include "kernel01.asm"
    include "kernelRainbow.asm"

;-------------------------------------------------------------------------------

    CHECK_OVERFLOW 0, $FF0

; --------------------------------------------------------------------------
; Bank 0 Footer - needed for CDFJ+ to function

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
