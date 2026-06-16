	org  CURRENT_ORG
	rorg $f000

BANK1_START

;-------------------------------------------------------------------------------

BANK_kernelCopyright = BANK1
    include "kernels/kernel_Copyright.asm"

BANK_kernelSkull = BANK1
    include "kernels/kernel_Skull.asm"

BANK_kernelCouchCompliant = BANK1
    include "kernels/kernel_CouchCompliant.asm"

BANK_kernelMenu = BANK1
    include "kernels/kernel_Menu.asm"

BANK_kernelGame = BANK1
    include "kernels/kernel_game.asm"

;-------------------------------------------------------------------------------


    CHECK_OVERFLOW 1, $1000

CURRENT_ORG SET CURRENT_ORG + $1000
