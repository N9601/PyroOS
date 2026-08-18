/* ============================================================================
 *  PyroOS  -  saved execution context (setjmp/longjmp style)
 * ----------------------------------------------------------------------------
 *  save_context stores the callee-saved registers, stack pointer, return
 *  address, and flags, returning 0. A later restore_context jumps back to that
 *  point, making save_context appear to return 1. Used to leave ring 3 on
 *  SYS_EXIT and to recover from a CPU fault. Implemented in ring3.asm.
 * ==========================================================================*/
#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>

typedef struct {
    uint32_t ebx, esi, edi, ebp, esp, eip, eflags;
} ctx_t;

int  save_context(ctx_t *ctx);       /* returns 0 now, 1 when restored */
void restore_context(ctx_t *ctx);    /* jump back to a saved context */

#endif
