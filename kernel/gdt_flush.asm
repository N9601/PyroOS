; ============================================================================
;  PyroOS  -  load the kernel GDT and TSS
; ============================================================================
[bits 32]

; void gdt_flush(uint32_t gdt_ptr);
global gdt_flush
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]                  ; load the new GDT

    mov ax, 0x10                ; reload data segment registers -> kernel data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush             ; far jump reloads CS -> kernel code
.flush:
    ret

; void tss_flush(void);
global tss_flush
tss_flush:
    mov ax, 0x28                ; TSS selector (GDT index 5)
    ltr ax                      ; load the task register
    ret
