/* ============================================================================
 *  PyroOS  -  the C kernel
 * ----------------------------------------------------------------------------
 *  Entry point kmain, called by the assembly stub once the CPU is in 32-bit
 *  protected mode. Uses the screen driver to print. Interrupt setup is wired
 *  in during Milestone 4.
 * ==========================================================================*/
#include "screen.h"

void kmain(void)
{
    clear_screen();
    kprint_color("PyroOS: C kernel is running. Hello from C.\n", COLOR_GREEN_ON_BLACK);
    kprint_color("We jumped from the boot sector into compiled C code.\n", COLOR_GREEN_ON_BLACK);
}
