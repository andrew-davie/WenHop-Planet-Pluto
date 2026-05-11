	org CURRENT_BANK
	rorg $f000

.BANK2






BANK2_CODE_SIZE = * - .BANK2;
	echo "---- BANK2", BANK2_CODE_SIZE, "bytes"
	echo "---- BANK2", ($fff0 - *), "bytes free"



