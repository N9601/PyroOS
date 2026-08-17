; ============================================================================
;  PyroOS  -  Global Descriptor Table (GDT)
; ============================================================================
;  Protected mode won't let us touch memory until we hand the CPU a table of
;  "segment descriptors." Each descriptor is 8 bytes describing a region of
;  memory: its base address, its size (limit), and its access rules.
;
;  We build the smallest useful GDT: 3 entries.
;    1. A mandatory NULL descriptor (all zeros). The CPU requires the first
;       entry to be null as a safety net -- a selector of 0 is "invalid."
;    2. A CODE segment: base 0, limit 4 GB, executable, readable.
;    3. A DATA segment: base 0, limit 4 GB, writable.
;
;  Both real segments cover the whole address space (the "flat model"), so
;  after we switch, an address is just a plain 32-bit number.
; ============================================================================

gdt_start:

; --- Entry 0: the null descriptor (8 zero bytes) ---
gdt_null:
    dd 0x0          ; four zero bytes
    dd 0x0          ; four more zero bytes

; --- Entry 1: the code segment descriptor ---
; base  = 0x00000000
; limit = 0xFFFFF   (with 4 KB granularity below, this covers the full 4 GB)
;
; Access byte = 1001 1010b = 0x9A:
;   1......   present            -> 1 (segment is in memory)
;   .00....   privilege (ring)   -> 00 (ring 0, kernel)
;   ...1...   descriptor type    -> 1 (code or data, not a system segment)
;   ....1..   executable         -> 1 (this is code)
;   .....0.   direction/conform  -> 0 (can only be run from ring 0)
;   ......1.  readable           -> 1 (allow reading constants from code)
;   .......0  accessed           -> 0 (CPU sets this; we start it at 0)
;
; Flags nibble = 1100b = 0xC (this is the high nibble of the limit byte):
;   1...   granularity -> 1 (limit is in 4 KB pages, so 0xFFFFF -> 4 GB)
;   .1..   size        -> 1 (32-bit protected mode segment)
;   ..0.   long mode   -> 0 (not 64-bit)
;   ...0   available   -> 0 (unused)
gdt_code:
    dw 0xFFFF       ; limit bits 0-15
    dw 0x0000       ; base  bits 0-15
    db 0x00         ; base  bits 16-23
    db 0x9A         ; access byte (described above)
    db 11001111b    ; flags (high nibble 0xC) + limit bits 16-19 (0xF)
    db 0x00         ; base  bits 24-31

; --- Entry 2: the data segment descriptor ---
; Identical to the code segment EXCEPT the access byte:
; Access byte = 1001 0010b = 0x92:
;   ....0..  executable -> 0 (this is data, not code)
;   ......1. writable   -> 1 (allow writes; data must be writable)
gdt_data:
    dw 0xFFFF       ; limit bits 0-15
    dw 0x0000       ; base  bits 0-15
    db 0x00         ; base  bits 16-23
    db 0x92         ; access byte
    db 11001111b    ; flags + limit bits 16-19
    db 0x00         ; base  bits 24-31

gdt_end:            ; label here so the assembler can compute the table size

; ----------------------------------------------------------------------------
;  The GDT descriptor: this is the 6-byte value we feed to the `lgdt`
;  instruction. It is NOT a segment; it just tells the CPU where the table
;  is and how big it is.
;    - 2 bytes: size of the table minus 1
;    - 4 bytes: the linear address where the table starts
; ----------------------------------------------------------------------------
gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; table size - 1
    dd gdt_start                 ; table address

; ----------------------------------------------------------------------------
;  Convenience constants: a "selector" is a byte offset into the GDT. The
;  code segment is 0x08 bytes in (entry 1), the data segment 0x10 (entry 2).
;  We'll load these into the segment registers after the switch.
; ----------------------------------------------------------------------------
CODE_SEG equ gdt_code - gdt_start   ; = 0x08
DATA_SEG equ gdt_data - gdt_start   ; = 0x10
