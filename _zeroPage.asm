;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ 2600 RIOT RAM - A mere 128 bytes
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	SEG.U VARS
	org $80

scanSK          ds 1


kernel			ds 1		;<FRAMEWORK>
tvSystem		ds 1		;<FRAMEWORK>  see TV_TYPE_ definitions

soundMode		ds 1		;<FRAMEWORK>
soundSave		ds 1		;<FRAMEWORK>
; colubk          ds 1
call_fn			ds 1		;<FRAMEWORK>

audv0			ds 1		;<FRAMEWORK>
audc0			ds 1		;<FRAMEWORK>
audf0			ds 1		;<FRAMEWORK>
audv1			ds 1		;<FRAMEWORK>
audc1			ds 1		;<FRAMEWORK>
audf1			ds 1		;<FRAMEWORK>


;------------------------------------------------------------------------------

jumpCodeRAM		    ds 10		    ; self-modifying bank routine call code

  ;      Code                  bytes [] = modified
  ; -----------------------------------------------------------------------
  ; cmp SelectBankX           0 = cmp, [1=SM_JumpBank_L],    2=high bank
  ; jsr .called_bank_routine  3 = jsr, [4=SM_JumpRoutine_L], [5=SM_JumpRoutine_H]
  ; cmp SelectBank0           6 = cmp, 7=low bank,           8=high bank
  ; rts                       9 = rts

SM_JumpBank_L      = (jumpCodeRAM + 1)
SM_JumpRoutine_L   = (jumpCodeRAM + 4)
SM_JumpRoutine_H   = (jumpCodeRAM + 5)

;------------------------------------------------------------------------------



test_position		ds 1		;only for demonstration of positioning method purposes

SK_SIZE = ((100+7)/8)

    ; Local (zp) vars where SK is initially read to on startup
    ; These are transferred to ARM/C via transfer to DISPLAY RAM variables
    ; via DSPTR setup and then DSWRITEs

SK_START
SK_ID             ds 1
SK_ENABLE_ICC     ds 1
SK_ODOMETER       ds 2

SK_BYTES          = (* - SK_START)

    ; following must be contiguous to above...

SK_RESET          ds 1            ; 0 = not reset, else SK was initialised

scanline                ds 1


    ; zero page overlays

ZP_OVERLAY = *
ZP_END SET *

    ;-----------------------------------------------
    
    SEG.U ZP_COPYRIGHT
    org ZP_OVERLAY

CP_temp           ds 1

    if (* > ZP_END)
ZP_END SET *
    endif

    ;-----------------------------------------------

    ORG ZP_END






	;Display Remaining RAM
	echo "---- 2600 RAM [  128 ] -->", (ZP_END - $80)d, "bytes used", ($100 - ZP_END)d, "bytes free" 

; EOF
