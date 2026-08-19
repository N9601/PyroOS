/* ============================================================================
 *  PyroOS  -  the C kernel
 * ----------------------------------------------------------------------------
 *  Entry point kmain: bring up interrupts, virtual memory, the heap, and the
 *  device drivers, enable interrupts, then hand control to the shell.
 * ==========================================================================*/
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "syscall.h"
#include "paging.h"
#include "kheap.h"
#include "timer.h"
#include "keyboard.h"
#include "fs.h"
#include "logo.h"
#include "shell.h"

void kmain(void)
{
    clear_screen();
    kprint_color("PyroOS kernel online.\n", COLOR_GREEN_ON_BLACK);

    gdt_install();          /* kernel GDT with user segments + TSS */
    idt_install();          /* interrupt table + PIC remap */
    syscall_install();      /* int 0x80 system-call gate */
    paging_install();       /* virtual memory (identity-map first 4 MB) */
    heap_install();         /* dynamic memory (kmalloc/kfree) */
    timer_install(50);      /* 50 Hz system timer */
    keyboard_install();     /* keyboard IRQ + input buffer */
    fs_init();              /* mount the filesystem (format the disk if new) */

    /* Install the bundled user program into the filesystem so `exec prog`
       (and `ls`) can find it. Compiled separately, embedded as a byte array. */
    extern const unsigned char user_prog[];
    extern unsigned int user_prog_len;
    extern const unsigned char crash_prog[];
    extern unsigned int crash_prog_len;
    fs_write("prog", user_prog, user_prog_len);
    fs_write("crash", crash_prog, crash_prog_len);

    kprint("Subsystems: interrupts, paging, heap, timer, keyboard, fs.\n");

    __asm__ volatile("sti");/* enable interrupts */

    logo_splash(75);        /* fire logo for ~1.5s, then clear */

    shell_run();            /* never returns */
}
