;-------------------------------------------------------------------------------
;@@@@@@@@@@@@@@@@@@@@@@@@@ Cross Bank Routine Handler @@@@@@@@@@@@@@@@@@@@@@@@@

VB_FN_OFFSET = ((.END-.START)/3)
OS_FN_OFFSET = (VB_FN_OFFSET * 2)


_KERNEL_RAINBOW     = 0
_KERNEL_01          = 1
_KERNEL_COPYRIGHT   = 2


kernelBank_L
.START
                    ; >>> kernel
                    .byte <BANK_kernelRainbow       ; 0 KERNEL_RAINBOW
                    .byte <BANK_kernel_01           ; 1 KERNEL_01
                    .byte <BANK_kernelCopyright     ; 2 KERNEL_COPYRIGHT

                    ; >>> VB
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernel_01
                    .byte <BANK_kernelCopyright

                    ; >>> OS
                    .byte <BANK_kernelRainbow
                    .byte <BANK_kernel_01
                    .byte <BANK_kernelCopyright
.END

kernelRoutine_L
                    .byte <kernelRainbow
                    .byte <kernel_01
                    .byte <kernelCopyright

                    .byte <VB_kernelRainbow
                    .byte <VB_kernel_01
                    .byte <VB_kernelCopyright

                    .byte <OS_kernelRainbow
                    .byte <OS_kernel_01
                    .byte <OS_kernelCopyright

kernelRoutine_H
                    .byte >kernelRainbow
                    .byte >kernel_01
                    .byte >kernelCopyright

                    .byte >VB_kernelRainbow
                    .byte >VB_kernel_01
                    .byte >VB_kernelCopyright

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
