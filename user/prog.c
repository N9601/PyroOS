/* ============================================================================
 *  PyroOS  -  a standalone user program
 * ----------------------------------------------------------------------------
 *  Compiled entirely separately from the kernel, linked to run at 0x80000, and
 *  stored in the PyroFS filesystem. The kernel's `exec` command loads it from
 *  disk into memory and runs it in ring 3. It has no access to kernel functions
 *  or hardware; the only way it can do anything is an int 0x80 system call.
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

/* Placed in its own section so the linker puts it at the very start (0x80000),
   which is where the kernel jumps after loading the program.

   It takes argc and argv like any UNIX program. The kernel built them on this
   stack before the first instruction ran, so reading them costs nothing: the
   strings are already sitting in the user zone where ring 3 can reach them. */
__attribute__((section(".text.start")))
void _start(int argc, char **argv)
{
    sys_write("  [prog] hello from a separately-compiled program.\n");
    sys_write("  [prog] loaded from PyroFS, running in ring 3, syscalls only.\n");

    sys_write("  [prog] my arguments:\n");
    for (int i = 0; i < argc; i++) {
        char label[10] = { ' ', ' ', ' ', ' ', 'a', 'r', 'g', 'v', '[', 0 };
        char idx[2] = { (char)('0' + i), 0 };
        sys_write(label);
        sys_write(idx);
        sys_write("] = ");
        sys_write(argv[i]);
        sys_write("\n");
    }

    sys_exit();

    for (;;)                     /* never reached: sys_exit does not return */
        sys_exit();
}
