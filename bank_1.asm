	org CURRENT_ORG
	rorg $f000

.BANK SET BANK1

BANK1_START

    include "kernelCopyright.asm"

;-------------------------------------------------------------------------------


amp_table2
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
	.byte #%00000000
amp_table1
	.byte #%00000000
	.byte #%10000000
	.byte #%11000000
	.byte #%11100000
	.byte #%11110000
	.byte #%11111000
	.byte #%11111100
	.byte #%11111110
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111
	.byte #%11111111


    CHECK_OVERFLOW 1, $1000

CURRENT_ORG SET CURRENT_ORG + $1000
