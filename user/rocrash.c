/* ============================================================================
 *  PyroOS  -  a program that writes to its own code
 * ----------------------------------------------------------------------------
 *  Its .text segment is read-only once the ELF loader applies segment
 *  permissions. Storing through a pointer into its own code must therefore
 *  fault in ring 3, and the kernel must survive and reclaim it. Before segment
 *  protection existed, this write silently succeeded and corrupted the program.
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
    sys_write("  [rocrash] about to write to my own code at _start...\n");

    /* _start lives in .text, which the loader marked read-only. This store
       should fault before it ever completes. */
    volatile unsigned char *code = (volatile unsigned char *)_start;
    *code = 0x90;                /* try to scribble a NOP over the entry point */

    sys_write("  [rocrash] if you can read this, the write was NOT blocked.\n");
    sys_exit();
}
