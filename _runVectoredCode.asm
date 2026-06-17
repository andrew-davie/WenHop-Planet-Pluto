;-------------------------------------------------------------------------------
;@@@@@@@@@@@@@@@@@@@@@@@@@ Cross Bank Routine Handler @@@@@@@@@@@@@@@@@@@@@@@@@

KNO                 SET 0

    MAC KERNEL; {name}
_KERNEL_{1}         SET KNO
KNO                 SET KNO + 1
    ENDM

    KERNEL DETECT_CONSOLE       ; 0
    KERNEL RAINBOW              ; 1
    KERNEL COPYRIGHT            ; 2
    KERNEL COUCH_COMPLIANT      ; 3
    KERNEL MENU                 ; 4
    KERNEL GAME                 ; 5
    KERNEL SKULL                ; 6

    KERNEL MAX


VB_FN_OFFSET = ((.END-.START)/3)
OS_FN_OFFSET = (VB_FN_OFFSET * 2)



kernelBank_L
.START
                    ; >>> kernel
                    .byte <BANK_kernelDetectConsole     ; 0 KERNEL_DETECT_CONSOLE
                    .byte <BANK_kernelRainbow           ; 1 KERNEL_RAINBOW
                    .byte <BANK_kernelCopyright         ; 2 KERNEL_COPYRIGHT
                    .byte <BANK_kernelCopyright         ; 3 KERNEL_COUCH_COMPLIANT (re-uses COPYRIGHT)
                    .byte <BANK_kernelMenu              ; 4 KERNEL_MENU
                    .byte <BANK_kernelGame              ; 4 KERNEL_GAME
                    .byte <BANK_kernelSkull              ; 4 KERNEL_SKULL

                    ; >>> VB
                    .byte <BANK_kernelDetectConsole
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernelCopyright
                    .byte <BANK_kernelCopyright
                    .byte <BANK_kernelMenu
                    .byte <BANK_kernelGame
                    .byte <BANK_kernelSkull

                    ; >>> OS
                    .byte <BANK_kernelDetectConsole
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernelCopyright
                    .byte <BANK_kernelCopyright
                    .byte <BANK_kernelMenu
                    .byte <BANK_kernelGame
                    .byte <BANK_kernelSkull
.END

kernelRoutine_L
                    .byte <kernelDetectConsole
                    .byte <kernelRainbow
                    .byte <kernelCopyright
                    .byte <kernelCopyright
                    .byte <kernelMenu
                    .byte <kernelGame
                    .byte <kernelSkull

                    .byte <VB_kernelDetectConsole
                    .byte <VB_kernelRainbow
                    .byte <VB_kernelCopyright
                    .byte <VB_kernelCopyright
                    .byte <VB_kernelMenu
                    .byte <VB_kernelGame
                    .byte <VB_kernelSkull

                    .byte <OS_kernelDetectConsole
                    .byte <OS_kernelRainbow
                    .byte <OS_kernelCopyright
                    .byte <OS_kernelCopyright
                    .byte <OS_kernelMenu
                    .byte <OS_kernelGame
                    .byte <OS_kernelSkull

kernelRoutine_H
                    .byte >kernelDetectConsole
                    .byte >kernelRainbow
                    .byte >kernelCopyright
                    .byte >kernelCopyright
                    .byte >kernelMenu
                    .byte >kernelGame
                    .byte >kernelSkull

                    .byte >VB_kernelDetectConsole
                    .byte >VB_kernelRainbow
                    .byte >VB_kernelCopyright
                    .byte >VB_kernelCopyright
                    .byte >VB_kernelMenu
                    .byte >VB_kernelGame
                    .byte >VB_kernelSkull

                    .byte >OS_kernelDetectConsole
                    .byte >OS_kernelRainbow
                    .byte >OS_kernelCopyright
                    .byte >OS_kernelCopyright
                    .byte >OS_kernelMenu
                    .byte >OS_kernelGame
                    .byte >OS_kernelSkull

;-------------------------------------------------------------------------------

runVectoredCode

    ; self-modify the in-RAM routine to vector to correct kernel/VB
    ; the 'rts' from jumpCodeRAM will actually return to THIS routine's caller

                	lda kernelBank_L,x
                	sta SM_JumpBank_L

	                lda kernelRoutine_L,x
	                sta SM_JumpRoutine_L
	                lda kernelRoutine_H,x
	                sta SM_JumpRoutine_H

	                sta WSYNC
                    jmp jumpCodeRAM

;-------------------------------------------------------------------------------

; EOF
