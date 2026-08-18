/* ============================================================================
 *  PyroOS  -  interactive shell
 * ----------------------------------------------------------------------------
 *  Reads characters from the keyboard buffer, echoes them, and builds a line.
 *  On Enter it parses the line and runs a built-in command. This is the first
 *  piece of PyroOS you actually drive by typing.
 * ==========================================================================*/
#include "shell.h"
#include "screen.h"
#include "keyboard.h"
#include "kheap.h"
#include "timer.h"
#include "ports.h"
#include "ata.h"
#include "fs.h"
#include "string.h"

#include <stdint.h>

#define LINE_MAX 128

static char     line[LINE_MAX];
static uint32_t line_len = 0;

/* --- tiny string helpers (no libc) --- */
static int streq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int starts_with(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

static void prompt(void)
{
    kprint_color("pyro> ", COLOR_GREEN_ON_BLACK);
}

static void cmd_help(void)
{
    kprint("Available commands:\n");
    kprint("  help          this list\n");
    kprint("  about         what PyroOS is\n");
    kprint("  clear         clear the screen\n");
    kprint("  echo <text>   print text back\n");
    kprint("  ls            list files\n");
    kprint("  write <f> <t> write text t to file f\n");
    kprint("  cat <f>       print file f\n");
    kprint("  mem           test the heap allocator\n");
    kprint("  ticks         timer ticks since boot\n");
    kprint("  disk          check the boot disk\n");
    kprint("  reboot        restart the machine\n");
}

static void reboot(void)
{
    kprint("Rebooting...\n");
    __asm__ volatile("cli");
    /* Pulse the CPU reset line via the 8042 keyboard controller. */
    outb(0x64, 0xFE);
    for (;;)
        __asm__ volatile("hlt");
}

static void execute(const char *cmd)
{
    if (cmd[0] == '\0') {
        return;
    } else if (streq(cmd, "help")) {
        cmd_help();
    } else if (streq(cmd, "about")) {
        kprint("PyroOS: a from-scratch x86 operating system.\n");
        kprint("bootloader, protected mode, C kernel, paging, heap, and this shell.\n");
    } else if (streq(cmd, "clear")) {
        clear_screen();
    } else if (streq(cmd, "mem")) {
        void *p = kmalloc(16);
        void *q = kmalloc(16);
        kprint("  kmalloc(16) -> "); kprint_hex((uint32_t)p); kprint_char('\n');
        kprint("  kmalloc(16) -> "); kprint_hex((uint32_t)q); kprint_char('\n');
        kfree(p);
        kfree(q);
    } else if (streq(cmd, "ticks")) {
        kprint("  timer ticks since boot: ");
        kprint_dec(timer_ticks());
        kprint_char('\n');
    } else if (streq(cmd, "disk")) {
        static uint8_t sec[512];
        if (ata_read(0, 1, sec) == 0) {
            kprint("  read LBA 0, signature ");
            kprint_hex((uint32_t)(sec[510] | (sec[511] << 8)));
            kprint((sec[510] == 0x55 && sec[511] == 0xAA)
                       ? "  (valid boot sector)\n" : "\n");
        } else {
            kprint_color("  disk read failed\n", COLOR_RED_ON_BLACK);
        }
    } else if (streq(cmd, "reboot")) {
        reboot();
    } else if (streq(cmd, "echo")) {
        kprint_char('\n');
    } else if (starts_with(cmd, "echo ")) {
        kprint(cmd + 5);
        kprint_char('\n');
    } else if (streq(cmd, "ls")) {
        fs_list();
    } else if (starts_with(cmd, "write ")) {
        const char *p = cmd + 6;
        while (*p == ' ') p++;
        char name[24];
        int i = 0;
        while (*p && *p != ' ' && i < 23) name[i++] = *p++;
        name[i] = '\0';
        while (*p == ' ') p++;                       /* p now points at the text */
        if (name[0] == '\0') {
            kprint("usage: write <file> <text>\n");
        } else if (fs_write(name, p, strlen(p)) == 0) {
            kprint("  wrote "); kprint(name); kprint_char('\n');
        } else {
            kprint_color("  write failed\n", COLOR_RED_ON_BLACK);
        }
    } else if (starts_with(cmd, "cat ")) {
        const char *name = cmd + 4;
        while (*name == ' ') name++;
        static char fbuf[4096];
        uint32_t sz = 0;
        if (fs_read(name, fbuf, sizeof(fbuf), &sz) == 0) {
            for (uint32_t i = 0; i < sz; i++) kprint_char(fbuf[i]);
            kprint_char('\n');
        } else {
            kprint_color("  no such file\n", COLOR_RED_ON_BLACK);
        }
    } else {
        kprint_color("unknown command: ", COLOR_RED_ON_BLACK);
        kprint(cmd);
        kprint("\nType 'help'.\n");
    }
}

static void handle_char(char c)
{
    if (c == '\n') {
        kprint_char('\n');
        line[line_len] = '\0';
        execute(line);
        line_len = 0;
        prompt();
    } else if (c == '\b') {
        if (line_len > 0) {
            line_len--;
            kprint_char('\b');
        }
    } else if (line_len < LINE_MAX - 1) {
        line[line_len++] = c;
        kprint_char(c);
    }
}

void shell_run(void)
{
    kprint("\nPyroOS shell ready. Type 'help'.\n\n");
    prompt();

    for (;;) {
        int c = keyboard_getchar();
        if (c >= 0)
            handle_char((char)c);
        else
            __asm__ volatile("hlt");   /* sleep until the next interrupt */
    }
}
