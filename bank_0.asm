	org CURRENT_ORG
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

CURRENT_ORG SET CURRENT_ORG + $1000

; EOF
