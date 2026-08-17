; ============================================================================
;  PyroOS  -  The switch from 16-bit real mode to 32-bit protected mode
; ============================================================================
;  This is the transition itself. It runs in real mode, does the five steps
;  needed to enter protected mode, then far-jumps into 32-bit code.
;
;  Requires the GDT (gdt.asm) and the protected-mode printer (print_pm.asm)
;  to be included in the same build.
; ============================================================================

[bits 16]
switch_to_pm:
    cli                     ; 1. Turn off interrupts. In protected mode the
                            ;    old real-mode interrupt vectors are invalid,
                            ;    and we have no IDT yet, so any interrupt now
                            ;    would triple-fault (reboot) the CPU.

    ; 2. Enable the A20 line via the "fast A20 gate" on system port 0x92.
    ;    Bit 1 of that port ungates address line 20, making memory above
    ;    1 MB addressable. (This fast method is fine on QEMU and modern PCs;
    ;    some ancient machines need the slower keyboard-controller method.)
    in al, 0x92
    or al, 00000010b
    out 0x92, al

    lgdt [gdt_descriptor]   ; 3. Load our GDT. Now the CPU knows about our
                            ;    flat code and data segments.

    mov eax, cr0            ; 4. Set the PE (Protection Enable) bit: bit 0 of
    or eax, 0x1             ;    control register CR0. The instant this is
    mov cr0, eax            ;    written, the CPU is technically in protected
                            ;    mode. But CS still holds a real-mode value...

    ; 5. ...so we perform a FAR jump. A far jump reloads CS, and reloading CS
    ;    is what forces the CPU to flush its prefetch pipeline and start
    ;    decoding instructions as 32-bit using our new code segment. Without
    ;    this jump the CPU would keep running stale 16-bit assumptions.
    jmp CODE_SEG:init_pm

; ----------------------------------------------------------------------------
;  We are now in 32-bit protected mode. The far jump above landed here.
;  CS is correct, but the OTHER segment registers (ds, ss, es, fs, gs) still
;  hold real-mode junk. We point them all at our flat DATA segment, then set
;  up a fresh stack, and finally jump to the kernel entry point BEGIN_PM
;  (defined in boot.asm).
; ----------------------------------------------------------------------------
[bits 32]
init_pm:
    mov ax, DATA_SEG        ; all data/stack segment registers -> flat data seg
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000        ; give protected mode a fresh stack, well clear of
    mov esp, ebp            ; our code at 0x7C00

    call BEGIN_PM           ; hand off to the 32-bit entry point in boot.asm
