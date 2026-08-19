/* ============================================================================
 *  PyroOS  -  user mode (ring 3) launcher
 * ----------------------------------------------------------------------------
 *  run_user_program saves the kernel context, then drops into a ring-3 program.
 *  In ring 3 the program has no privileges: its ONLY way to affect the system
 *  is through int 0x80 system calls. When it calls SYS_EXIT, the kernel unwinds
 *  back here (via restore_context) and returns to the shell.
 * ==========================================================================*/
#include "usermode.h"
#include "syscall.h"
#include "screen.h"
#include "context.h"

#include <stdint.h>

extern void enter_user_mode(void (*entry)(void), uint32_t user_esp);

static ctx_t   kernel_ctx;
static uint8_t user_stack[8192] __attribute__((aligned(16)));

/* The ring-3 program. It cannot touch hardware or kernel memory directly; it
   asks the kernel to print, then asks to exit. Everything is a syscall. */
static void user_program(void)
{
    const char *msg = "  [ring 3] hello from user mode, printed via a syscall\n";
    __asm__ volatile("int $0x80" :: "a"(SYS_WRITE), "b"(msg) : "memory");
    __asm__ volatile("int $0x80" :: "a"(SYS_EXIT));

    for (;;)                        /* unreachable; SYS_EXIT does not return */
        __asm__ volatile("int $0x80" :: "a"(SYS_EXIT));
}

/* Called from the SYS_EXIT syscall handler (running in ring 0). */
void user_exit(void)
{
    restore_context(&kernel_ctx);   /* jumps back into run_user_at */
}

/* Enter ring 3 at `entry`, run until SYS_EXIT, then return here. Works for both
   the built-in demo function and a program loaded from disk. */
void run_user_at(uint32_t entry)
{
    if (save_context(&kernel_ctx) == 0) {
        enter_user_mode((void (*)(void))entry,
                        (uint32_t)&user_stack[sizeof(user_stack)]);
    }
    /* SYS_EXIT unwound us back here. */
    kprint("  back in the kernel (ring 0); program exited cleanly.\n");
}

void run_user_program(void)
{
    run_user_at((uint32_t)user_program);
}
