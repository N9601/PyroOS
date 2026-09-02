/* ============================================================================
 *  PyroOS  -  a program that asks the kernel who it is
 * ----------------------------------------------------------------------------
 *  getpid is the simplest process-aware system call: the program does not know
 *  its own pid, because the kernel assigned it. This asks, and prints the
 *  answer. Run it twice and the number changes, because the first process was
 *  reaped and the second got a fresh id.
 * ==========================================================================*/
#define SYS_WRITE  0
#define SYS_EXIT   2
#define SYS_GETPID 8

static void sys_write(const char *s)
{
    __asm__ volatile("int $0x80" :: "a"(SYS_WRITE), "b"(s) : "memory");
}
static int sys_getpid(void)
{
    int pid;
    __asm__ volatile("int $0x80" : "=a"(pid) : "a"(SYS_GETPID));
    return pid;
}
static void sys_exit(void)
{
    __asm__ volatile("int $0x80" :: "a"(SYS_EXIT));
}

/* Print a small non-negative integer with no libc. */
static void put_int(int n)
{
    char buf[12];
    int i = 0;
    if (n == 0) {
        sys_write("0");
        return;
    }
    while (n > 0 && i < 11) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    char out[12];
    int j = 0;
    while (i > 0)
        out[j++] = buf[--i];
    out[j] = '\0';
    sys_write(out);
}

__attribute__((section(".text.start")))
void _start(void)
{
    sys_write("  [whoami] my process id is ");
    put_int(sys_getpid());
    sys_write("\n");
    sys_exit();
}
