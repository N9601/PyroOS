; ============================================================================
;  PyroOS  -  enter ring 3, and a setjmp/longjmp pair to leave it
; ============================================================================
[bits 32]

; void enter_user_mode(void (*entry)(void), uint32_t user_esp);
;   Builds an inter-privilege iret frame and drops to ring 3. iret is the only
;   way to lower the privilege level.
global enter_user_mode
enter_user_mode:
    cli
    mov ecx, [esp + 4]      ; entry point (user EIP)
    mov edx, [esp + 8]      ; user stack top (user ESP)

    mov ax, 0x23            ; user data selector (RPL 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword 0x23         ; SS   = user data
    push edx                ; ESP  = user stack
    push dword 0x202        ; EFLAGS with IF set
    push dword 0x1B         ; CS   = user code (RPL 3)
    push ecx                ; EIP  = entry
    iret                    ; -> ring 3

; int save_context(ctx_t *ctx);   returns 0 now, 1 when restored later
; ctx layout (offsets): 0 ebx, 4 esi, 8 edi, 12 ebp, 16 esp, 20 eip, 24 eflags
global save_context
save_context:
    mov eax, [esp + 4]
    mov [eax + 0], ebx
    mov [eax + 4], esi
    mov [eax + 8], edi
    mov [eax + 12], ebp
    mov [eax + 16], esp
    mov ecx, [esp]          ; return address
    mov [eax + 20], ecx
    pushf
    pop ecx
    mov [eax + 24], ecx     ; saved eflags
    xor eax, eax            ; return 0
    ret

; void restore_context(ctx_t *ctx);   resumes save_context as if it returned 1
global restore_context
restore_context:
    mov eax, [esp + 4]
    mov ebx, [eax + 0]
    mov esi, [eax + 4]
    mov edi, [eax + 8]
    mov ebp, [eax + 12]
    mov ecx, [eax + 20]     ; saved return address
    mov edx, [eax + 24]     ; saved eflags
    mov esp, [eax + 16]     ; switch back to the saved stack...
    add esp, 4              ; ...and drop the return address the ret consumed
    push edx
    popf                    ; restore eflags (re-enables interrupts)
    mov eax, 1              ; return 1 (the "restored" path)
    jmp ecx
