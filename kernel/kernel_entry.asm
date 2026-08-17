; ============================================================================
;  PyroOS  -  kernel entry stub
; ----------------------------------------------------------------------------
;  The boot sector jumps to the very first byte where the kernel was loaded.
;  A C compiler can lay out functions in any order, so we cannot assume kmain
;  is first. This tiny stub solves that: the linker script places it at the
;  front of the kernel, so it IS the first byte. All it does is call into C.
;
;  Assembled as an ELF object (nasm -f elf32) so the linker can combine it
;  with the compiled C object.
; ============================================================================

[bits 32]
[extern kmain]          ; kmain is defined in kernel.c; the linker resolves it

global _start
section .text
_start:
    call kmain          ; hand control to the C kernel
    jmp $               ; if kmain ever returns, halt here forever
