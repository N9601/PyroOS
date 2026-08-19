/* ============================================================================
 *  PyroOS  -  a deliberately misbehaving user program
 * ----------------------------------------------------------------------------
 *  Runs in ring 3 and tries to write to kernel memory. Because the kernel's
 *  pages are supervisor-only, the CPU raises a page fault, the kernel's fault
 *  handler terminates this program, and PyroOS keeps running. This is what
 *  memory protection buys you: a broken or hostile program cannot corrupt the
 *  kernel. Linked at 0x80000 like any program (see user/prog.ld).
 * ==========================================================================*/

#define SYS_WRITE 0
#define SYS_EXIT  2

static void sys_write(const char *s)
{
    __asm__ volatile("int $0x80" :: "a"(SYS_WRITE), "b"(s) : "memory");
}

static void sys_exit(void)
{
    __asm__ volatile("int $0x80" :: "a"(SYS_EXIT));
}

__attribute__((section(".text.start")))
void _start(void)
{
    sys_write("  [crash] attempting to write to kernel memory at 0x10000...\n");
    *(volatile unsigned int *)0x10000 = 0xdeadbeef;   /* page fault: ring 3 -> supervisor page */
    sys_write("  [crash] this line should never run.\n");
    sys_exit();

    for (;;)
        sys_exit();
}
