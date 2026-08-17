; ============================================================================
;  PyroOS  -  Stage 1 Boot Sector
; ============================================================================
;  When you power on an x86 PC, the BIOS runs first. It looks at the boot
;  device, reads the very first sector (512 bytes) into memory at physical
;  address 0x7C00, and jumps to it -- but ONLY if the last two bytes of that
;  sector are 0x55 0xAA (the boot signature). If they aren't, the BIOS
;  decides this isn't bootable and moves on.
;
;  So THIS file must assemble to exactly 512 bytes, ending in 0xAA55.
;  The CPU starts in "real mode": 16-bit, no memory protection, only ~1 MB
;  addressable. Our whole job in stage 1 is to prove we're alive by printing
;  a message, then halt. Later stages will switch to 32-bit protected mode
;  and load a C kernel.
; ============================================================================

[org 0x7c00]        ; Tell NASM: assume this code is loaded at 0x7C00.
                    ; This makes label addresses (like MSG_HELLO) resolve to
                    ; the correct absolute addresses at run time.

[bits 16]           ; We are in 16-bit real mode.

start:
    ; The BIOS puts the boot drive number in DL before jumping to us.
    ; Save it now; we'll need it later when we load more sectors from disk.
    mov [BOOT_DRIVE], dl

    ; ------------------------------------------------------------------
    ; Set up a stack. In real mode there's no OS to give us one, so we
    ; carve out our own. The stack grows DOWNWARD from SS:SP. We point it
    ; just below our code at 0x7C00, giving the stack room to grow toward
    ; lower addresses without clobbering us.
    ; ------------------------------------------------------------------
    xor ax, ax          ; AX = 0
    mov ds, ax          ; Data segment  = 0  (so [label] means 0x0000:label)
    mov es, ax          ; Extra segment = 0
    mov ss, ax          ; Stack segment = 0
    mov bp, 0x7c00      ; Base pointer at 0x7C00
    mov sp, bp          ; Stack pointer starts at 0x7C00 and grows down

    ; Print our banner.
    mov bx, MSG_HELLO
    call print_string

    ; Nothing more to do in stage 1 -- hang forever.
    jmp $               ; "$" means "the current address": jump to self = spin.

; ============================================================================
;  print_string  -  print a null-terminated string
;  Input: BX = address of string
;  Uses BIOS teletype service (INT 0x10, AH=0x0E) which prints one character
;  in AL to the screen and advances the cursor. This service only exists in
;  real mode -- once we leave for protected mode we lose BIOS calls and must
;  write to VGA memory directly (that's Milestone 3).
; ============================================================================
print_string:
    pusha               ; save all general registers
    mov ah, 0x0e        ; BIOS function: teletype output
.next_char:
    mov al, [bx]        ; load the character at BX
    cmp al, 0           ; null terminator?
    je .done            ; yes -> finished
    int 0x10            ; no  -> print AL
    inc bx              ; advance to next character
    jmp .next_char
.done:
    popa                ; restore registers
    ret

; ----------------------------------------------------------------------------
;  Data
; ----------------------------------------------------------------------------
BOOT_DRIVE: db 0
MSG_HELLO:  db "PyroOS: stage 1 boot sector alive.", 0x0d, 0x0a, 0

; ----------------------------------------------------------------------------
;  Boot signature padding
;  "$$" = start of this section, "$" = current position, so ($ - $$) is how
;  many bytes we've emitted so far. Pad with zeros up to offset 510, then
;  place the 2-byte 0xAA55 magic. Total: exactly 512 bytes.
; ----------------------------------------------------------------------------
    times 510 - ($ - $$) db 0
    dw 0xaa55
