; ============================================================================
;  PyroOS  -  disk loading (real mode)
; ----------------------------------------------------------------------------
;  The BIOS only auto-loads sector 1 (our 512-byte boot sector). The kernel
;  lives in the sectors right after it. This routine uses BIOS disk service
;  INT 0x13, function 0x02 ("read sectors"), to copy the kernel into memory.
;  It must run in real mode, before we switch to protected mode, because the
;  BIOS is gone after the switch.
;
;  Sectors are numbered from 1. Sector 1 is the boot sector, so the kernel
;  starts at sector 2.
; ============================================================================

[bits 16]

; disk_load: read DH sectors from the boot drive (DL) into ES:BX.
disk_load:
    pusha
    push dx                 ; save DX; we need the original DH (sector count)
                            ; later to verify the read

    mov ah, 0x02            ; BIOS function: read sectors
    mov al, dh              ; AL = number of sectors to read
    mov ch, 0x00            ; cylinder 0
    mov dh, 0x00            ; head 0
    mov cl, 0x02            ; start reading at sector 2 (right after boot sector)
                            ; ES:BX (already set by caller) = destination buffer

    int 0x13                ; call BIOS
    jc disk_error           ; carry flag set => read failed

    pop dx                  ; restore original DX (DH = requested sector count)
    cmp al, dh              ; AL = sectors ACTUALLY read; must match request
    jne disk_error

    popa
    ret

disk_error:
    mov bx, DISK_ERROR_MSG
    call print_string       ; print_string is defined in boot.asm (real mode)
    jmp $                   ; halt

DISK_ERROR_MSG: db "PyroOS: disk read error.", 0x0d, 0x0a, 0
