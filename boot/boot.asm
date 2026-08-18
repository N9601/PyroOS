; ============================================================================
;  PyroOS  -  Stage 1 Boot Sector
; ============================================================================
;  Milestone 2: boot in 16-bit real mode, print a banner via the BIOS, then
;  switch the CPU to 32-bit protected mode and print again -- this time by
;  writing directly to VGA memory, which only works once we're in 32-bit mode.
;
;  Still exactly 512 bytes, still ending in 0xAA55, still loaded at 0x7C00.
; ============================================================================

[org 0x7c00]
[bits 16]

KERNEL_OFFSET  equ 0x10000  ; where we load the kernel: 64 KB, safely ABOVE the
                            ; boot sector (0x7C00) and real-mode stack (0x9000),
                            ; so the disk load can't overwrite running code. The
                            ; linker script builds the kernel to run here.
KERNEL_SECTORS equ 60       ; how many 512-byte sectors of kernel to load
                            ; (60 = 30 KB; raise with the image size in the
                            ;  Makefile if the kernel outgrows it)

start:
    mov [BOOT_DRIVE], dl     ; save BIOS boot drive for later use

    ; Set up real-mode segments and a stack just below our code.
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov bp, 0x9000          ; put the real-mode stack safely above 0x7C00
    mov sp, bp

    mov bx, MSG_REAL_MODE
    call print_string       ; still using BIOS teletype -- we're in real mode

    ; Load the kernel from disk into memory at KERNEL_OFFSET, while the BIOS is
    ; still available. Destination and sector count live in the disk address
    ; packet (see disk.asm); the drive number comes from BOOT_DRIVE.
    call disk_load

    call switch_to_pm       ; leave real mode. This never returns: it ends in
                            ; a far jump into 32-bit code (init_pm -> BEGIN_PM).

    jmp $                   ; safety net; execution should never reach here

; ----------------------------------------------------------------------------
;  Real-mode BIOS string printer (16-bit). Input: BX = string address.
; ----------------------------------------------------------------------------
print_string:
    pusha
    mov ah, 0x0e
.next_char:
    mov al, [bx]
    cmp al, 0
    je .done
    int 0x10
    inc bx
    jmp .next_char
.done:
    popa
    ret

; ----------------------------------------------------------------------------
;  Pull in the protected-mode machinery. Order matters only in that all of
;  this must fit within the 512-byte sector alongside the code above.
; ----------------------------------------------------------------------------
%include "gdt.asm"          ; the Global Descriptor Table
%include "disk.asm"         ; disk_load (read kernel sectors via BIOS)
%include "switch_pm.asm"    ; switch_to_pm + init_pm (the actual transition)
%include "print_pm.asm"     ; print_pm (VGA memory printer)

; ----------------------------------------------------------------------------
;  32-bit entry point. init_pm (in switch_pm.asm) calls here after the CPU is
;  fully in protected mode with segments and a stack set up.
; ----------------------------------------------------------------------------
[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_pm           ; write straight to VGA memory at 0xB8000

    call KERNEL_OFFSET      ; jump into the kernel we loaded at 0x1000. Its
                            ; first byte is the _start stub, which calls kmain.

    jmp $                   ; if the kernel returns, halt here

; ----------------------------------------------------------------------------
;  Data
; ----------------------------------------------------------------------------
BOOT_DRIVE:    db 0
MSG_REAL_MODE: db "PyroOS: real mode OK, switching to 32-bit...", 0x0d, 0x0a, 0
MSG_PROT_MODE: db "PyroOS: landed in 32-bit protected mode.", 0

; ----------------------------------------------------------------------------
;  Pad to 510 bytes and add the boot signature.
; ----------------------------------------------------------------------------
    times 510 - ($ - $$) db 0
    dw 0xaa55
