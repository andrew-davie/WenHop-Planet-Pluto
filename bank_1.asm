	org  CURRENT_ORG
	rorg $f000

.BANK SET BANK1
BANK1_START


;-------------------------------------------------------------------------------

    include "kernelCopyright.asm"

;-------------------------------------------------------------------------------


    CHECK_OVERFLOW 1, $1000

CURRENT_ORG SET CURRENT_ORG + $1000
