/* ============================================================================
 *  PyroOS  -  Interrupt Service Routines (ISR/IRQ) interface
 * ==========================================================================*/
#ifndef ISR_H
#define ISR_H

#include <stdint.h>
#include "context.h"

/* A snapshot of the CPU state at the moment of an interrupt. The assembly
   stub in interrupt.asm pushes exactly these fields onto the stack, then
   passes a pointer to this struct to our C handler. The field order MUST
   match the push order in the stub. */
typedef struct {
    uint32_t ds;                                     /* data segment we saved */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pushed by pusha */
    uint32_t int_no, err_code;                       /* interrupt number + error */
    uint32_t eip, cs, eflags, useresp, ss;           /* pushed by the CPU */
} registers_t;

typedef void (*isr_t)(registers_t *);

void isr_install(void);                                   /* fill IDT + remap PIC */
void register_interrupt_handler(uint8_t n, isr_t handler);/* register a C handler */

/* Fault recovery. Usage: save_context(&fault_recovery_ctx); fault_arm(); do the
   risky thing; fault_disarm(). If a CPU exception fires while armed, execution
   unwinds back to the save_context call (which then appears to return 1). The
   save_context call MUST be in a stack frame that lives until the fault. */
extern ctx_t fault_recovery_ctx;
extern volatile int g_user_faulted;   /* set to 1 when a fault triggered recovery */
void fault_arm(void);
void fault_disarm(void);

#endif
