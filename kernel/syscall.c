/* ============================================================================
 *  PyroOS  -  system calls (int 0x80)
 * ----------------------------------------------------------------------------
 *  A user (or kernel) caller loads a syscall number into eax and arguments
 *  into ebx/ecx/edx, then executes `int 0x80`. The CPU traps into the kernel
 *  through the IDT gate, this handler runs, and any return value is written
 *  back into the saved eax so the caller receives it.
 *
 *  The gate is installed with DPL 3 so ring-3 code is permitted to invoke it.
 * ==========================================================================*/
#include "syscall.h"
#include "idt.h"
#include "isr.h"
#include "screen.h"
#include "timer.h"
#include "usermode.h"

extern void isr128(void);       /* the int 0x80 stub in interrupt.asm */

void syscall_handler(registers_t *r)
{
    switch (r->eax) {
    case SYS_WRITE:
        kprint((const char *)r->ebx);
        r->eax = 0;
        break;
    case SYS_UPTIME:
        r->eax = timer_ticks();
        break;
    case SYS_EXIT:
        user_exit();                /* unwind back to the kernel; never returns */
        break;
    default:
        r->eax = (uint32_t)-1;
        break;
    }
}

void syscall_install(void)
{
    /* Flags 0xEE = present, DPL 3, 32-bit interrupt gate. DPL 3 is what lets
       ring-3 code trigger this via int 0x80. */
    idt_set_gate(0x80, (uint32_t)isr128, 0x08, 0xEE);
}
