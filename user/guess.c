/* ============================================================================
 *  PyroOS  -  number guessing game (a ring-3 program)
 * ----------------------------------------------------------------------------
 *  Picks a secret number with SYS_RAND, then reads your guesses and gives
 *  higher/lower hints until you get it. A real interactive app in ring 3.
 * ==========================================================================*/

#define SYS_WRITE 0
#define SYS_EXIT  2
#define SYS_READ  3
#define SYS_RAND  5

static void sys_write(const char *s)
{ __asm__ volatile("int $0x80" :: "a"(SYS_WRITE), "b"(s) : "memory"); }

static int sys_read(void)
{ int c; __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_READ)); return c; }

static int sys_rand(void)
{ int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_RAND)); return r; }

static void sys_exit(void)
{ __asm__ volatile("int $0x80" :: "a"(SYS_EXIT)); }

static void put_char(char c) { char b[2]; b[0] = c; b[1] = '\0'; sys_write(b); }

static void print_int(int n)
{
    char buf[12];
    int i = 11;
    buf[i--] = '\0';
    if (n == 0) { sys_write("0"); return; }
    unsigned u = (unsigned)n;
    while (u) { buf[i--] = '0' + (u % 10); u /= 10; }
    sys_write(&buf[i + 1]);
}

static int read_int(void)
{
    int val = 0, any = 0;
    for (;;) {
        int c = sys_read();
        if (c == '\n' || c == '\r') { if (any) break; else continue; }
        if (c >= '0' && c <= '9') { val = val * 10 + (c - '0'); any = 1; put_char((char)c); }
    }
    return val;
}

__attribute__((section(".text.start")))
void _start(void)
{
    int secret = (sys_rand() % 100) + 1;    /* 1 to 100 */
    int tries = 0;

    sys_write("  [guess] I am thinking of a number between 1 and 100.\n");

    for (;;) {
        sys_write("  [guess] your guess:  ");
        int g = read_int();
        tries++;
        sys_write("\n");

        if (g == secret) {
            sys_write("  [guess] correct, it was ");
            print_int(secret);
            sys_write("! you took ");
            print_int(tries);
            sys_write(" tries.\n");
            break;
        } else if (g < secret) {
            sys_write("  [guess] too low, go higher.\n");
        } else {
            sys_write("  [guess] too high, go lower.\n");
        }
    }

    sys_exit();
    for (;;) sys_exit();
}
