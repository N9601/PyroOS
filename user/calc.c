/* ============================================================================
 *  PyroOS  -  calculator (a ring-3 program)
 * ----------------------------------------------------------------------------
 *  Reads a number, an operator, and a second number through syscalls, then
 *  prints the result. Runs in ring 3, isolated, syscalls only.
 * ==========================================================================*/

#define SYS_WRITE 0
#define SYS_EXIT  2
#define SYS_READ  3

static void sys_write(const char *s)
{ __asm__ volatile("int $0x80" :: "a"(SYS_WRITE), "b"(s) : "memory"); }

static int sys_read(void)
{ int c; __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_READ)); return c; }

static void sys_exit(void)
{ __asm__ volatile("int $0x80" :: "a"(SYS_EXIT)); }

static void put_char(char c) { char b[2]; b[0] = c; b[1] = '\0'; sys_write(b); }

static void print_int(int n)
{
    char buf[12];
    int i = 11;
    buf[i--] = '\0';
    if (n == 0) { sys_write("0"); return; }
    int neg = n < 0;
    unsigned u = neg ? (unsigned)(-n) : (unsigned)n;
    while (u) { buf[i--] = '0' + (u % 10); u /= 10; }
    if (neg) buf[i--] = '-';
    sys_write(&buf[i + 1]);
}

/* Read a signed integer typed by the user (digits, optional leading -). */
static int read_int(void)
{
    int val = 0, any = 0, neg = 0;
    for (;;) {
        int c = sys_read();
        if (c == '\n' || c == '\r') { if (any) break; else continue; }
        if (c == '-' && !any) { neg = 1; any = 1; put_char('-'); continue; }
        if (c >= '0' && c <= '9') { val = val * 10 + (c - '0'); any = 1; put_char((char)c); }
    }
    return neg ? -val : val;
}

__attribute__((section(".text.start")))
void _start(void)
{
    sys_write("  [calc] first number:  ");
    int a = read_int();

    sys_write("\n  [calc] operator (+ - * /):  ");
    int op = 0;
    for (;;) {
        int c = sys_read();
        if (c == '+' || c == '-' || c == '*' || c == '/') { op = c; put_char((char)c); break; }
    }

    sys_write("\n  [calc] second number:  ");
    int b = read_int();

    sys_write("\n  [calc] ");
    print_int(a); put_char(' '); put_char((char)op); put_char(' '); print_int(b);
    sys_write(" = ");

    if (op == '/' && b == 0) {
        sys_write("cannot divide by zero.\n");
    } else {
        int r = 0;
        if (op == '+') r = a + b;
        else if (op == '-') r = a - b;
        else if (op == '*') r = a * b;
        else if (op == '/') r = a / b;
        print_int(r);
        sys_write("\n");
    }

    sys_exit();
    for (;;) sys_exit();
}
