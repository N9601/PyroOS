; ============================================================================
;  PyroOS  -  interrupt assembly stubs
; ----------------------------------------------------------------------------
;  When an interrupt fires, the CPU jumps to a raw address with almost no
;  bookkeeping. These stubs do the bookkeeping: push the interrupt number (and
;  a dummy error code where the CPU doesn't provide one), save all registers,
;  call our C dispatcher, then restore everything and return with iret.
;
;  Two families:
;    isr0..isr31  -> CPU exceptions (divide-by-zero, page fault, etc.)
;    irq0..irq15  -> hardware interrupts (timer, keyboard, ...) after PIC remap
; ============================================================================

[bits 32]
[extern isr_handler]            ; C function, in isr.c
[extern irq_handler]            ; C function, in isr.c
[extern syscall_handler]        ; C function, in syscall.c

; --- macro: exception with NO CPU error code (we push a dummy 0) ---
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push dword 0            ; dummy error code, keeps the stack layout uniform
        push dword %1           ; interrupt number
        jmp isr_common_stub
%endmacro

; --- macro: exception where the CPU already pushed an error code ---
%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        push dword %1           ; interrupt number (error code already on stack)
        jmp isr_common_stub
%endmacro

; --- macro: hardware IRQ. arg1 = irq number, arg2 = remapped vector ---
%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push dword 0            ; dummy error code
        push dword %2           ; remapped interrupt number (32..47)
        jmp irq_common_stub
%endmacro

; The CPU pushes an error code only for these exception numbers: 8, 10-14, 17.
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; IRQs 0..15 map to interrupt vectors 32..47 (we remap the PIC to these).
IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; --- shared tail for CPU exceptions ---
isr_common_stub:
    pusha                       ; push edi,esi,ebp,esp,ebx,edx,ecx,eax
    mov ax, ds
    push eax                    ; save the data segment selector
    mov ax, 0x10                ; load the kernel data segment (GDT data = 0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp                ; eax -> the registers_t we just built
    push eax                    ; pass it as the argument
    call isr_handler
    add esp, 4                  ; drop the argument
    pop eax                     ; restore the saved data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8                  ; drop the pushed int_no and err_code
    iret                        ; return to the interrupted code

; --- shared tail for hardware IRQs (identical, but calls irq_handler) ---
irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax
    call irq_handler
    add esp, 4
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret

; --- system call entry (int 0x80) ---
; Same shape as the IRQ stub, but calls syscall_handler. The handler may write
; a return value into the saved eax (via the registers_t pointer), which popa
; then restores for the caller.
global isr128
isr128:
    push dword 0                ; dummy error code
    push dword 128              ; interrupt number
    pusha
    mov ax, ds
    push eax                    ; save data segment
    mov ax, 0x10                ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp                ; eax -> registers_t
    push eax
    call syscall_handler
    add esp, 4
    pop eax                     ; restore data segment (temp in eax)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa                        ; restores saved regs incl. eax (return value)
    add esp, 8                  ; drop int_no and err_code
    iret

; --- load the IDT (called from idt.c as idt_flush(&idtp)) ---
global idt_flush
idt_flush:
    mov eax, [esp + 4]          ; the pointer argument
    lidt [eax]
    ret
