;-------------------------------------------------------------------------------
;@@@@@@@@@@@@@@@@@@@@@@@@@ Cross Bank Routine Handler @@@@@@@@@@@@@@@@@@@@@@@@@

KNO                 SET 0

    MAC KERNEL; {name}
_KERNEL_{1}         SET KNO
KNO                 SET KNO + 1
    ENDM

    KERNEL DETECT_CONSOLE       ; 0
    KERNEL RAINBOW              ; 1
    KERNEL 01                   ; 2
    KERNEL COPYRIGHT            ; 3

    KERNEL MAX


VB_FN_OFFSET = ((.END-.START)/3)
OS_FN_OFFSET = (VB_FN_OFFSET * 2)



kernelBank_L
.START
                    ; >>> kernel
                    .byte <BANK_kernelDetectConsole ; 0 KERNEL_DETECT_CONSOLE
                    .byte <BANK_kernelRainbow       ; 1 KERNEL_RAINBOW
                    .byte <BANK_kernel_01           ; 2 KERNEL_01
                    .byte <BANK_kernelCopyright     ; 3 KERNEL_COPYRIGHT

                    ; >>> VB
                    .byte <BANK_kernelDetectConsole
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernel_01
                    .byte <BANK_kernelCopyright

                    ; >>> OS
                    .byte <BANK_kernelDetectConsole
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernel_01
                    .byte <BANK_kernelCopyright
.END

kernelRoutine_L
                    .byte <kernelDetectConsole
                    .byte <kernelRainbow
                    .byte <kernel_01
                    .byte <_kernelCopyright

                    .byte <VB_kernelDetectConsole
                    .byte <VB_kernelRainbow
                    .byte <VB_kernel_01
                    .byte <VB_kernelCopyright

                    .byte <OS_kernelDetectConsole
                    .byte <OS_kernelRainbow
                    .byte <OS_kernel_01
                    .byte <OS_kernelCopyright

kernelRoutine_H
                    .byte >kernelDetectConsole
                    .byte >kernelRainbow
                    .byte >kernel_01
                    .byte >_kernelCopyright

                    .byte >VB_kernelDetectConsole
                    .byte >VB_kernelRainbow
                    .byte >VB_kernel_01
                    .byte >VB_kernelCopyright

                    .byte >OS_kernelDetectConsole
                    .byte >OS_kernelRainbow
                    .byte >OS_kernel_01
                    .byte >OS_kernelCopyright

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

	                jmp jumpCodeRAM

;-------------------------------------------------------------------------------

; EOF
