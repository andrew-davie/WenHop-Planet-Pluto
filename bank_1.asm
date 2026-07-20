	org  CURRENT_ORG
	rorg $f000

BANK1_START

;-------------------------------------------------------------------------------
; keep GAME early so the NOP lock will have lots of time
BANK_kernelGame = BANK1
    include "kernels/kernel_game.asm"


BANK_kernelCopyright = BANK1
    include "kernels/kernel_Copyright.asm"

BANK_kernelSkull = BANK1
    include "kernels/kernel_Skull.asm"

BANK_kernelCouchCompliant = BANK1
    include "kernels/kernel_CouchCompliant.asm"

BANK_kernelMenu = BANK1
    include "kernels/kernel_Menu.asm"


BANK_kernelGlobe = BANK1
    include "kernels/kernel_Globe.asm"

;-------------------------------------------------------------------------------


    CHECK_OVERFLOW 1, $1000

CURRENT_ORG SET CURRENT_ORG + $1000
