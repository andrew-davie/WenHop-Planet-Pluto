	org CURRENT_ORG
	rorg $f000

BANK2_START






BANK2_CODE_SIZE = * - BANK2_START
	echo "---- BANK2", BANK2_CODE_SIZE, "bytes"
	echo "---- BANK2", ($fff0 - *), "bytes free"


CURRENT_ORG SET CURRENT_ORG + $1000


