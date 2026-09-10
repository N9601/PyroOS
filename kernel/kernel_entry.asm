; ============================================================================
;  PyroOS  -  kernel entry stub
; ----------------------------------------------------------------------------
;  Placed first in the linked kernel, so it is the first instruction the boot
;  sector jumps to. It zeroes the .bss section (uninitialized globals -- the
;  C runtime would normally do this, but we are the OS), moves onto the
;  kernel's own stack, and then calls kmain.
; ============================================================================

[bits 32]
[extern kmain]
[extern bss_start]              ; provided by the linker script
[extern bss_end]

KERNEL_STACK_SIZE equ 16384

; The kernel stack lives in .bss, inside the kernel's own supervisor-only
; pages. The boot sector's stack at 0x90000 is not good enough to keep: it sits
; in the user zone (0x80000 to 0xFFFFF), which ring 3 can write, so a user
; program could overwrite the shell's saved frames and return addresses.
section .bss
align 16
kernel_stack_bottom:
    resb KERNEL_STACK_SIZE
kernel_stack_top:

global _start
section .text
_start:
    ; Zero the .bss range so global variables that start at 0 really are 0.
    ; This runs on the boot sector's stack, and touches no stack at all.
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi                ; ecx = number of bytes in .bss
    xor eax, eax
    rep stosb                   ; store AL (0) ECX times starting at [edi]

    mov esp, kernel_stack_top   ; leave the boot stack for good
    xor ebp, ebp                ; end of the frame-pointer chain

    call kmain                  ; hand control to the C kernel
    jmp $                       ; if kmain returns, halt here forever
