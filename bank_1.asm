	org  CURRENT_ORG
	rorg $f000

BANK1_START

;-------------------------------------------------------------------------------

BANK_kernelCopyright = BANK1
BANK_kernelCouchCompliant = BANK1
BANK_kernelMenu = BANK1

    include "kernel/kernel_Copyright.asm"
    include "kernel/kernel_CouchCompliant.asm"
    include "kernel/kernel_Menu.asm"

;-------------------------------------------------------------------------------


    CHECK_OVERFLOW 1, $1000

CURRENT_ORG SET CURRENT_ORG + $1000
