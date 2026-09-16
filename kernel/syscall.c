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

/* A string is only safe to read if all of it, terminator included, lies in
   the user zone. Checking the first byte is not enough: a string that starts
   inside the zone can run straight on into the kernel heap at 0x100000. */
static int user_string_ok(uint32_t p)
{
    if (!in_user_zone(p, 1))
        return 0;
    for (const char *s = (const char *)p; (uint32_t)s < 0x00100000u; s++)
        if (*s == '\0')
            return 1;
    return 0;
}

/* Did this trap come from ring 3? The kernel itself also uses int 0x80 (the
   shell's `syscall` command), and its pointers are kernel pointers by design.
   The low two bits of the saved CS are the privilege level it was running at. */
static int from_user(const registers_t *r)
{
    return (r->cs & 3) == 3;
}

extern void isr128(void);       /* the int 0x80 stub in interrupt.asm */

void syscall_handler(registers_t *r)
{
    switch (r->eax) {
    case SYS_WRITE:
        if (from_user(r) && !user_string_ok(r->ebx)) {
            r->eax = (uint32_t)-1;      /* would read kernel memory */
            break;
        }
        kprint((const char *)r->ebx);
        r->eax = 0;
        break;
    case SYS_UPTIME:
        r->eax = timer_ticks();
        break;
    case SYS_GETPID:
        r->eax = (uint32_t)proc_current()->pid;
        break;
    case SYS_SBRK: {
        /* Grow the process heap by ebx bytes and return the OLD break, which is
           where the freshly granted region begins. This mirrors UNIX sbrk: the
           caller gets a pointer to memory it did not have a moment ago. The
           memory itself already exists (the user zone is identity-mapped), so
           this only moves and bounds-checks the break. Returns -1 if the growth
           would run past the end of the heap, leaving the break untouched. */
        proc_t *p = proc_current();
        uint32_t old = p->brk ? p->brk : USER_HEAP_BASE;
        uint32_t want = old + r->ebx;
        if (r->ebx == 0) {
            r->eax = old;               /* sbrk(0) queries the current break */
        } else if (want < old || want > USER_HEAP_END) {
            r->eax = (uint32_t)-1;      /* overflow or out of heap */
        } else {
            p->brk = want;
            r->eax = old;
        }
        break;
    }
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
        if (user_string_ok(r->ebx) && in_user_zone(r->ecx, r->edx))
            r->eax = (uint32_t)fs_write((const char *)r->ebx, (const void *)r->ecx, r->edx);
        else
            r->eax = (uint32_t)-1;
        break;
    }
    case SYS_FREAD: {
        /* ebx=name, ecx=buffer, edx=max. The kernel writes into the buffer, so
           it must be inside the user zone. Returns bytes read, or -1. */
        if (user_string_ok(r->ebx) && in_user_zone(r->ecx, r->edx)) {
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
