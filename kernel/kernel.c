/* ============================================================================
 *  PyroOS  -  the C kernel
 * ----------------------------------------------------------------------------
 *  Entry point kmain, called by the assembly stub once the CPU is in 32-bit
 *  protected mode. Sets up interrupts, the timer, and the keyboard, then
 *  enables interrupts and idles -- waking only to service hardware.
 * ==========================================================================*/
#include "screen.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"

void kmain(void)
{
    clear_screen();
    kprint_color("PyroOS kernel online.\n", COLOR_GREEN_ON_BLACK);

    idt_install();          /* build and load the interrupt table + remap PIC */
    timer_install(50);      /* start the timer at 50 Hz (ticks in background) */
    keyboard_install();     /* register the keyboard IRQ handler */

    __asm__ volatile("sti");/* enable interrupts: hardware can now reach us */

    kprint("Interrupts are live. The timer is ticking and the keyboard works.\n");
    kprint("Type something:\n\n> ");

    /* Idle forever. hlt sleeps the CPU until the next interrupt, which is far
       more efficient than a busy spin. Each keypress or timer tick wakes it. */
    for (;;)
        __asm__ volatile("hlt");
}
