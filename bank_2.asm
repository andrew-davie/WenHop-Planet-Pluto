	org CURRENT_BANK
	rorg $f000

BANK2_START






BANK2_CODE_SIZE = * - BANK2_START
	echo "---- BANK2", BANK2_CODE_SIZE, "bytes"
	echo "---- BANK2", ($fff0 - *), "bytes free"


CURRENT_BANK SET CURRENT_BANK + $1000


