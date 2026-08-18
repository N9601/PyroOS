/* ============================================================================
 *  PyroOS  -  the C kernel
 * ----------------------------------------------------------------------------
 *  Entry point kmain, called by the assembly stub once the CPU is in 32-bit
 *  protected mode. Sets up interrupts, the timer, and the keyboard, then
 *  enables interrupts and idles -- waking only to service hardware.
 * ==========================================================================*/
#include "screen.h"
#include "idt.h"
#include "paging.h"
#include "kheap.h"
#include "timer.h"
#include "keyboard.h"

static void heap_selftest(void)
{
    void *a = kmalloc(64);
    void *b = kmalloc(128);
    kprint("  kmalloc(64)  -> "); kprint_hex((uint32_t)a); kprint_char('\n');
    kprint("  kmalloc(128) -> "); kprint_hex((uint32_t)b); kprint_char('\n');
    kfree(a);
    void *c = kmalloc(32);  /* should reuse the block we just freed */
    kprint("  free(a), kmalloc(32) -> "); kprint_hex((uint32_t)c);
    kprint(c == a ? "  (reused freed block)\n" : "  (new block)\n");
    kfree(b);
    kfree(c);
}

void kmain(void)
{
    clear_screen();
    kprint_color("PyroOS kernel online.\n", COLOR_GREEN_ON_BLACK);

    idt_install();          /* build and load the interrupt table + remap PIC */
    paging_install();       /* enable virtual memory (identity-map first 4 MB) */
    kprint("Paging enabled: first 4 MB identity-mapped.\n");

    heap_install();         /* set up dynamic memory */
    kprint("Heap online. Self-test:\n");
    heap_selftest();

    timer_install(50);      /* start the timer at 50 Hz (ticks in background) */
    keyboard_install();     /* register the keyboard IRQ handler */

    __asm__ volatile("sti");/* enable interrupts: hardware can now reach us */

    kprint("\nInterrupts live. Type something:\n\n> ");

    /* Idle forever. hlt sleeps the CPU until the next interrupt, which is far
       more efficient than a busy spin. Each keypress or timer tick wakes it. */
    for (;;)
        __asm__ volatile("hlt");
}
