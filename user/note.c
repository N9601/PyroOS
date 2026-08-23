/* ============================================================================
 *  PyroOS  -  note, a tiny text editor (a ring-3 program)
 * ----------------------------------------------------------------------------
 *  Asks for a filename, reads multi-line text (end with a single dot on its
 *  own line), saves it to PyroFS with SYS_FWRITE, then reads it back with
 *  SYS_FREAD to confirm. Afterwards you can `cat <file>` from the shell: the
 *  data a ring-3 program wrote is on disk, persisted. Runs in ring 3, syscalls
 *  only, protected from the kernel.
 * ==========================================================================*/

#define SYS_WRITE  0
#define SYS_EXIT   2
#define SYS_READ   3
#define SYS_FWRITE 6
#define SYS_FREAD  7

static void sys_write(const char *s)
{ __asm__ volatile("int $0x80" :: "a"(SYS_WRITE), "b"(s) : "memory"); }

static int sys_read(void)
{ int c; __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_READ)); return c; }

static int sys_fwrite(const char *name, const void *data, int len)
{ int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_FWRITE), "b"(name), "c"(data), "d"(len) : "memory"); return r; }

static int sys_fread(const char *name, void *buf, int max)
{ int r; __asm__ volatile("int $0x80" : "=a"(r) : "a"(SYS_FREAD), "b"(name), "c"(buf), "d"(max) : "memory"); return r; }

static void sys_exit(void)
{ __asm__ volatile("int $0x80" :: "a"(SYS_EXIT)); }

static void put_char(char c) { char b[2]; b[0] = c; b[1] = '\0'; sys_write(b); }

static void print_int(int n)
{
    char t[12]; int i = 11; t[i--] = '\0';
    if (n == 0) { sys_write("0"); return; }
    unsigned u = (unsigned)n;
    while (u) { t[i--] = '0' + (u % 10); u /= 10; }
    sys_write(&t[i + 1]);
}

/* Read a filename (letters and digits) into dst. */
static void read_name(char *dst, int max)
{
    int i = 0;
    for (;;) {
        int c = sys_read();
        if (c == '\n' || c == '\r') { if (i > 0) break; else continue; }
        if (c == '\b') { if (i > 0) { i--; put_char('\b'); } continue; }
        if (i < max - 1) { dst[i++] = (char)c; put_char((char)c); }
    }
    dst[i] = '\0';
}

static char body[2048];
static char back[2048];
static char name[24];

__attribute__((section(".text.start")))
void _start(void)
{
    sys_write("  [note] filename: ");
    read_name(name, sizeof(name));

    sys_write("\n  [note] type your text. End with a single dot on its own line.\n");

    int len = 0, line_start = 0;
    for (;;) {
        int c = sys_read();
        if (c == '\n' || c == '\r') {
            if (len - line_start == 1 && body[line_start] == '.') {
                len = line_start;          /* drop the dot line and stop */
                break;
            }
            if (len < (int)sizeof(body) - 1) { body[len++] = '\n'; put_char('\n'); }
            line_start = len;
        } else if (c == '\b') {
            if (len > line_start) { len--; put_char('\b'); }
        } else if (len < (int)sizeof(body) - 1) {
            body[len++] = (char)c;
            put_char((char)c);
        }
    }
    body[len] = '\0';

    if (sys_fwrite(name, body, len) == 0) {
        sys_write("\n  [note] saved ");
        print_int(len);
        sys_write(" bytes to ");
        sys_write(name);
        sys_write("\n  [note] reading it back from disk:\n  ");
        int got = sys_fread(name, back, sizeof(back) - 1);
        if (got >= 0) {
            back[got] = '\0';
            sys_write(back);
        }
        sys_write("\n  [note] tip: run 'cat ");
        sys_write(name);
        sys_write("' from the shell to see it persisted.\n");
    } else {
        sys_write("\n  [note] save failed.\n");
    }

    sys_exit();
    for (;;) sys_exit();
}
