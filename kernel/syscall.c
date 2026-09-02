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
#include "keyboard.h"
#include "usermode.h"
#include "fs.h"
#include "proc.h"

/* A pointer passed from ring 3 must lie inside the user zone (0x80000 to
   0xFFFFF). This stops a program from tricking the kernel into reading or
   writing kernel memory through a syscall. */
static int in_user_zone(uint32_t p, uint32_t len)
{
    return p >= 0x00080000u && (uint64_t)p + len <= 0x00100000u;
}

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
    case SYS_GETPID:
        r->eax = (uint32_t)proc_current()->pid;
        break;
    case SYS_EXIT:
        user_exit();                /* unwind back to the kernel; never returns */
        break;
    case SYS_READ: {
        /* Block until a key arrives. Enable interrupts so the keyboard IRQ can
           fill the buffer and the timer can wake us from hlt. */
        int c;
        __asm__ volatile("sti");
        while ((c = keyboard_getchar()) < 0)
            __asm__ volatile("hlt");
        r->eax = (uint32_t)c;
        break;
    }
    case SYS_SLEEP: {
        uint32_t target = timer_ticks() + r->ebx;
        __asm__ volatile("sti");
        while (timer_ticks() < target)
            __asm__ volatile("hlt");
        r->eax = 0;
        break;
    }
    case SYS_RAND: {
        /* A small linear congruential generator, seeded from the timer. */
        static uint32_t s = 0;
        if (s == 0)
            s = timer_ticks() * 2654435761u + 1u;
        s = s * 1103515245u + 12345u;
        r->eax = (s >> 16) & 0x7FFF;
        break;
    }
    case SYS_FWRITE: {
        /* ebx=name, ecx=data, edx=length. Validate the user pointers first. */
        if (in_user_zone(r->ebx, 1) && in_user_zone(r->ecx, r->edx))
            r->eax = (uint32_t)fs_write((const char *)r->ebx, (const void *)r->ecx, r->edx);
        else
            r->eax = (uint32_t)-1;
        break;
    }
    case SYS_FREAD: {
        /* ebx=name, ecx=buffer, edx=max. The kernel writes into the buffer, so
           it must be inside the user zone. Returns bytes read, or -1. */
        if (in_user_zone(r->ebx, 1) && in_user_zone(r->ecx, r->edx)) {
            uint32_t got = 0;
            int rc = fs_read((const char *)r->ebx, (void *)r->ecx, r->edx, &got);
            r->eax = (rc == 0) ? got : (uint32_t)-1;
        } else {
            r->eax = (uint32_t)-1;
        }
        break;
    }
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
