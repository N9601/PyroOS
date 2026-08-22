/* ============================================================================
 *  PyroOS  -  an interactive user program
 * ----------------------------------------------------------------------------
 *  Runs in ring 3 and actually talks with you: it asks for your name, reads
 *  keystrokes one at a time through SYS_READ (echoing them), and greets you.
 *  It never touches hardware directly; input, output, and exit are all
 *  syscalls. Linked at 0x80000 like any program (see user/prog.ld).
 * ==========================================================================*/

#define SYS_WRITE 0
#define SYS_EXIT  2
#define SYS_READ  3

static void sys_write(const char *s)
{
    __asm__ volatile("int $0x80" :: "a"(SYS_WRITE), "b"(s) : "memory");
}

static int sys_read(void)
{
    int c;
    __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_READ));
    return c;
}

static void sys_exit(void)
{
    __asm__ volatile("int $0x80" :: "a"(SYS_EXIT));
}

/* Print a single character by writing a tiny null-terminated string. */
static void put_char(char c)
{
    char buf[2];
    buf[0] = c;
    buf[1] = '\0';
    sys_write(buf);
}

__attribute__((section(".text.start")))
void _start(void)
{
    sys_write("  [ask] What is your name? ");

    char name[64];
    int i = 0;
    for (;;) {
        int c = sys_read();
        if (c == '\n' || c == '\r')
            break;
        if (c == '\b') {              /* backspace */
            if (i > 0) {
                i--;
                put_char('\b');
            }
            continue;
        }
        if (i < 63) {
            name[i++] = (char)c;
            put_char((char)c);        /* echo the keystroke */
        }
    }
    name[i] = '\0';

    sys_write("\n  [ask] Hello, ");
    sys_write(name);
    sys_write("! Greetings from a ring-3 program.\n");

    sys_exit();
    for (;;)
        sys_exit();
}
