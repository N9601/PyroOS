; ============================================================================
;  PyroOS  -  kernel entry stub
; ----------------------------------------------------------------------------
;  Placed first in the linked kernel, so it is the first instruction the boot
;  sector jumps to. It zeroes the .bss section (uninitialized globals -- the
;  C runtime would normally do this, but we are the OS) and then calls kmain.
; ============================================================================

[bits 32]
[extern kmain]
[extern bss_start]              ; provided by the linker script
[extern bss_end]

global _start
section .text
_start:
    ; Zero the .bss range so global variables that start at 0 really are 0.
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi                ; ecx = number of bytes in .bss
    xor eax, eax
    rep stosb                   ; store AL (0) ECX times starting at [edi]

    call kmain                  ; hand control to the C kernel
    jmp $                       ; if kmain returns, halt here forever
