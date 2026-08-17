; ============================================================================
;  PyroOS  -  Protected-mode string printing
; ============================================================================
;  In 32-bit protected mode the BIOS is gone, so INT 0x10 no longer works.
;  Instead we write directly to VGA text memory, which the video hardware
;  maps at physical address 0xB8000.
;
;  The screen is an 80 x 25 grid. Each cell is two bytes:
;     byte 0: the ASCII character
;     byte 1: the attribute (color). 0x0F = white text on black background.
;
;  So the character at row r, column c lives at:
;     0xB8000 + 2 * (r * 80 + c)
; ============================================================================

[bits 32]                       ; this code only ever runs in 32-bit mode

VIDEO_MEMORY equ 0xB8000
WHITE_ON_BLACK equ 0x0F         ; attribute byte: bright white on black

; ----------------------------------------------------------------------------
;  print_pm  -  print a null-terminated string starting at the top-left.
;  Input: EBX = address of the string.
; ----------------------------------------------------------------------------
print_pm:
    pusha
    mov edx, VIDEO_MEMORY       ; EDX = current cell address in video memory

.loop:
    mov al, [ebx]               ; AL = next character of the string
    mov ah, WHITE_ON_BLACK      ; AH = its color attribute

    cmp al, 0                   ; end of string?
    je .done

    mov [edx], ax               ; write char + attribute (2 bytes) to the cell

    add ebx, 1                  ; advance to next character in the string
    add edx, 2                  ; advance to next cell (2 bytes per cell)
    jmp .loop

.done:
    popa
    ret
