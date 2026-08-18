/* ============================================================================
 *  PyroOS  -  the C kernel
 * ----------------------------------------------------------------------------
 *  Entry point kmain: bring up interrupts, virtual memory, the heap, and the
 *  device drivers, enable interrupts, then hand control to the shell.
 * ==========================================================================*/
#include "screen.h"
#include "idt.h"
#include "paging.h"
#include "kheap.h"
#include "timer.h"
#include "keyboard.h"
#include "shell.h"

void kmain(void)
{
    clear_screen();
    kprint_color("PyroOS kernel online.\n", COLOR_GREEN_ON_BLACK);

    idt_install();          /* interrupt table + PIC remap */
    paging_install();       /* virtual memory (identity-map first 4 MB) */
    heap_install();         /* dynamic memory (kmalloc/kfree) */
    timer_install(50);      /* 50 Hz system timer */
    keyboard_install();     /* keyboard IRQ + input buffer */

    kprint("Subsystems: interrupts, paging, heap, timer, keyboard.\n");

    __asm__ volatile("sti");/* enable interrupts */

    shell_run();            /* never returns */
}
