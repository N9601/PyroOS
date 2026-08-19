/* ============================================================================
 *  PyroOS  -  user mode (ring 3) launcher
 * ----------------------------------------------------------------------------
 *  run_user_at saves the kernel context, arms fault recovery, and drops into a
 *  ring-3 program. In ring 3 the program has no privileges: it can only touch
 *  the user zone (0x80000..0xFFFFF) and reach the kernel through int 0x80. Two
 *  ways back to the kernel:
 *    - the program calls SYS_EXIT (clean exit), or
 *    - the program faults (e.g. touches kernel memory) and the fault handler
 *      unwinds it. Either way PyroOS survives.
 * ==========================================================================*/
#include "usermode.h"
#include "syscall.h"
#include "screen.h"
#include "context.h"
#include "isr.h"
#include "string.h"

#include <stdint.h>

extern void enter_user_mode(void (*entry)(void), uint32_t user_esp);

/* Bundled program, embedded by the build. */
extern const unsigned char user_prog[];
extern unsigned int user_prog_len;

static ctx_t kernel_ctx;

/* The ring-3 stack lives at the top of the user zone and grows down. */
#define USER_STACK_TOP 0x000F0000u

/* Called from the SYS_EXIT syscall handler (running in ring 0). */
void user_exit(void)
{
    restore_context(&kernel_ctx);   /* jumps back into run_user_at */
}

/* Enter ring 3 at `entry`, run until the program exits or faults, then return.
   `entry` must point into the user zone. */
void run_user_at(uint32_t entry)
{
    g_user_faulted = 0;

    if (save_context(&kernel_ctx) == 0) {
        /* A fault in ring 3 should unwind to the same place as a clean exit. */
        fault_recovery_ctx = kernel_ctx;
        fault_arm();
        enter_user_mode((void (*)(void))entry, USER_STACK_TOP);
    }

    fault_disarm();

    if (g_user_faulted)
        kprint_color("  the program faulted and was terminated; the kernel survived.\n",
                     COLOR_RED_ON_BLACK);
    else
        kprint("  program exited cleanly; back in the kernel (ring 0).\n");
}

/* The `user` command: copy the bundled program into the user zone and run it.
   (It cannot run in place, because the kernel's own pages are supervisor-only
   now, so ring-3 code must live in the user zone.) */
void run_user_program(void)
{
    memcpy((void *)USER_LOAD_ADDR, user_prog, user_prog_len);
    run_user_at(USER_LOAD_ADDR);
}
