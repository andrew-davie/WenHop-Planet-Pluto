	org CURRENT_ORG
	rorg $f000

.BANK SET  BANK2

BANK2_START



    include "kernelRainbow.asm"





    CHECK_OVERFLOW 2, $1000

CURRENT_ORG SET CURRENT_ORG + $1000


