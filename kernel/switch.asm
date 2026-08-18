; ============================================================================
;  PyroOS  -  context switch
; ----------------------------------------------------------------------------
;  void context_switch(uint32_t *old_esp, uint32_t new_esp);
;
;  Saves the current task's callee-saved registers on its own stack, stores
;  its stack pointer into *old_esp, then loads new_esp as the stack pointer and
;  restores the other task's registers. After the final `ret`, execution
;  continues in the other task exactly where IT last called context_switch.
;
;  This is the single point where one task becomes another.
; ============================================================================

[bits 32]
global context_switch
context_switch:
    push ebp
    push ebx
    push esi
    push edi

    mov eax, [esp + 20]     ; arg1: old_esp (pointer)
    mov [eax], esp          ; save this task's stack pointer
    mov esp, [esp + 24]     ; arg2: new_esp -> switch stacks

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret                     ; return into the other task
