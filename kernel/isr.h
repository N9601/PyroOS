/* ============================================================================
 *  PyroOS  -  Interrupt Service Routines (ISR/IRQ) interface
 * ==========================================================================*/
#ifndef ISR_H
#define ISR_H

#include <stdint.h>

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

#endif
