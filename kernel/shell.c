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
#include "task.h"
#include "syscall.h"
#include "usermode.h"
#include "isr.h"
#include "pmm.h"
#include "vmm.h"
#include "demand.h"
#include "proc.h"
#include "context.h"
#include "logo.h"

#include <stdint.h>

#define LINE_MAX 128

static char     line[LINE_MAX];
static uint32_t line_len = 0;

/* Command history: up/down arrows recall previous commands. */
#define HIST_MAX 16
static char history[HIST_MAX][LINE_MAX];
static int  hist_count  = 0;   /* how many commands are stored */
static int  hist_browse = 0;   /* current position while browsing */

static void copy_line(char *dst, const char *src)
{
    int i = 0;
    for (; src[i] && i < LINE_MAX - 1; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

static void history_add(const char *s)
{
    if (s[0] != '\0') {
        if (hist_count < HIST_MAX) {
            copy_line(history[hist_count++], s);
        } else {
            for (int k = 0; k < HIST_MAX - 1; k++)
                copy_line(history[k], history[k + 1]);
            copy_line(history[HIST_MAX - 1], s);
        }
    }
    hist_browse = hist_count;   /* reset browsing to the fresh line */
}

/* Erase whatever is typed and replace it with s, on screen and in the buffer. */
static void replace_line(const char *s)
{
    while (line_len > 0) { kprint_char('\b'); line_len--; }
    int i = 0;
    for (; s[i] && i < LINE_MAX - 1; i++) { line[i] = s[i]; kprint_char(s[i]); }
    line_len = (uint32_t)i;
    line[line_len] = '\0';
}

static void history_prev(void)
{
    if (hist_count > 0 && hist_browse > 0)
        replace_line(history[--hist_browse]);
}

static void history_next(void)
{
    if (hist_browse < hist_count) {
        hist_browse++;
        replace_line(hist_browse == hist_count ? "" : history[hist_browse]);
    }
}

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
    kprint("  logo          show the PyroOS flame logo\n");
    kprint("  echo <text>   print text back\n");
    kprint("  ls            list files\n");
    kprint("  write <f> <t> write text t to file f\n");
    kprint("  cat <f>       print file f\n");
    kprint("  mem           test the heap allocator\n");
    kprint("  ticks         timer ticks since boot\n");
    kprint("  disk          check the boot disk\n");
    kprint("  tasks         cooperative multitasking demo\n");
    kprint("  spin          preemptive multitasking demo\n");
    kprint("  threads       mutex vs race-condition demo\n");
    kprint("  vm            address spaces and demand paging\n");
    kprint("  ps            list the process table\n");
    kprint("  syscall       invoke a system call (int 0x80)\n");
    kprint("  user          run the built-in ring-3 demo\n");
    kprint("  exec <f>      load a program from disk and run it in ring 3\n");
    kprint("  fault         trigger and recover from a page fault\n");
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
    } else if (streq(cmd, "logo")) {
        logo_splash(150);       /* show the flame for ~3s, then clear */
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
    } else if (streq(cmd, "tasks")) {
        kprint("  three tasks yield in turn (round-robin):\n");
        tasking_demo();
    } else if (streq(cmd, "spin")) {
        kprint("  two non-yielding tasks, preempted by the timer (~2s):\n");
        preempt_demo();
    } else if (streq(cmd, "vm")) {
        kprint("  physical frames: ");
        kprint_dec(pmm_free_frames()); kprint(" free of ");
        kprint_dec(pmm_total_frames()); kprint("\n");

        /* Two address spaces, same virtual address, different physical memory. */
        page_dir_t a = vmm_create_dir();
        page_dir_t b = vmm_create_dir();
        if (!a || !b) {
            kprint_color("  out of memory\n", COLOR_RED_ON_BLACK);
        } else {
            uint32_t v = 0x00800000;      /* 8 MB, above the shared kernel range */
            vmm_map_alloc(a, v, PF_PRESENT | PF_RW);
            vmm_map_alloc(b, v, PF_PRESENT | PF_RW);
            kprint("  virtual "); kprint_hex(v);
            kprint(" maps to "); kprint_hex(vmm_translate(a, v));
            kprint(" in space A, "); kprint_hex(vmm_translate(b, v));
            kprint(" in space B");
            kprint(vmm_translate(a, v) != vmm_translate(b, v)
                   ? "  (isolated)\n" : "  (SHARED, wrong)\n");

            /* Demand paging: promise 4 MB, commit nothing, then touch it. */
            uint32_t before = demand_faults_served();
            demand_region(vmm_kernel_dir(), 0x01000000, 0x01400000,
                          PF_PRESENT | PF_RW);
            kprint("  promised 4 MB at 0x01000000, no frames committed\n");
            volatile uint32_t *p = (volatile uint32_t *)0x01000000;
            p[0] = 0xC0FFEE;
            p[2048] = 0xBEEF;
            p[4096] = 0xF00D;
            kprint("  touched 3 pages, faults served ");
            kprint_dec(demand_faults_served() - before);
            kprint(", read back ");
            kprint_hex(p[0]); kprint(" ");
            kprint_hex(p[2048]); kprint(" ");
            kprint_hex(p[4096]); kprint("\n");

            vmm_destroy_dir(a);
            vmm_destroy_dir(b);
            kprint("  both spaces freed, frames free now ");
            kprint_dec(pmm_free_frames()); kprint("\n");
        }
    } else if (streq(cmd, "ps")) {
        proc_list();

    } else if (streq(cmd, "threads")) {
        kprint("  synchronization: a race condition and its fix with a mutex.\n");
        threads_demo();
    } else if (streq(cmd, "syscall")) {
        uint32_t ret;
        const char *msg = "  hello from a system call (int 0x80)\n";
        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(SYS_WRITE), "b"(msg));
        uint32_t up;
        __asm__ volatile("int $0x80" : "=a"(up) : "a"(SYS_UPTIME) : "ebx");
        kprint("  sys_uptime returned "); kprint_dec(up); kprint(" ticks\n");
    } else if (streq(cmd, "user")) {
        kprint("  dropping to ring 3 (user mode)...\n");
        run_user_program();
    } else if (starts_with(cmd, "exec ")) {
        const char *name = cmd + 5;
        while (*name == ' ') name++;
        uint32_t size = 0;
        if (fs_read(name, (void *)USER_LOAD_ADDR, 64 * 1024, &size) == 0 && size > 0) {
            kprint("  loaded "); kprint(name); kprint(" (");
            kprint_dec(size); kprint(" bytes) at "); kprint_hex(USER_LOAD_ADDR);
            kprint(", running in ring 3:\n");
            run_user_at(USER_LOAD_ADDR);
        } else {
            kprint_color("  no such program\n", COLOR_RED_ON_BLACK);
        }
    } else if (streq(cmd, "fault")) {
        kprint("  reading unmapped memory at 0x00800000 (above the 4 MB map)...\n");
        /* save_context is called directly here so its stack frame survives
           until the fault; fault_arm makes the page-fault handler unwind to it. */
        if (save_context(&fault_recovery_ctx) == 0) {
            fault_arm();
            volatile int v = *(volatile int *)0x00800000;   /* page fault */
            (void)v;
            kprint("  (no fault occurred?)\n");
        } else {
            kprint_color("  recovered! the shell is still alive.\n",
                         COLOR_GREEN_ON_BLACK);
        }
        fault_disarm();
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
    if (c == KEY_UP)   { history_prev(); return; }
    if (c == KEY_DOWN) { history_next(); return; }
    if (c == KEY_LEFT || c == KEY_RIGHT) return;   /* not handled yet */

    if (c == '\n') {
        kprint_char('\n');
        line[line_len] = '\0';
        history_add(line);
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
