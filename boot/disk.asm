; ============================================================================
;  PyroOS  -  disk loading (real mode, LBA)
; ----------------------------------------------------------------------------
;  Loads the kernel from disk using BIOS INT 13h extended read (AH=42h), which
;  addresses sectors by a flat Logical Block Address instead of the old
;  cylinder/head/sector scheme. LBA reads are immune to disk-geometry quirks
;  and comfortably load a large kernel in one call.
;
;  The kernel lives at LBA 1 onward (LBA 0 is this boot sector). We read it to
;  physical address 0x0000:KERNEL_OFFSET.
; ============================================================================

[bits 16]

disk_load:
    pusha
    mov si, dap                 ; DS:SI -> disk address packet
    mov ah, 0x42                ; extended read
    mov dl, [BOOT_DRIVE]        ; drive we booted from
    int 0x13
    jc disk_error               ; carry set => failure
    popa
    ret

disk_error:
    mov bx, DISK_ERROR_MSG
    call print_string
    jmp $

DISK_ERROR_MSG: db "PyroOS: disk read error.", 0x0d, 0x0a, 0

; --- Disk Address Packet for INT 13h AH=42h ---
align 4
dap:
    db 0x10                     ; packet size (16 bytes)
    db 0x00                     ; reserved
    dw KERNEL_SECTORS           ; number of sectors to read
    dw 0x0000                   ; destination offset
    dw 0x1000                   ; destination segment (0x1000:0000 = 0x10000)
    dq 1                        ; starting LBA (sector after the boot sector)
