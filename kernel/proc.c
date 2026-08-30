/* ============================================================================
 *  PyroOS  -  process table
 * ----------------------------------------------------------------------------
 *  A fixed table of process slots. Fixed rather than dynamically allocated
 *  because the table must remain valid while an address space is being torn
 *  down, and a fixed array cannot move under us.
 *
 *  Process 0 is the kernel itself. It owns the kernel address space, is never
 *  freed, and is the parent of anything started from the shell.
 * ==========================================================================*/
#include "proc.h"
#include "pmm.h"
#include "screen.h"
#include "string.h"

static proc_t table[MAX_PROCS];
static int    next_pid = 1;
static int    current_idx;

void proc_install(void)
{
    memset(table, 0, sizeof(table));

    /* slot 0 is the kernel, already running in the kernel address space */
    table[0].pid = 0;
    table[0].ppid = 0;
    table[0].state = PROC_RUNNING;
    table[0].dir = vmm_kernel_dir();
    table[0].entry = 0;
    table[0].brk = 0;
    strncpy_z(table[0].name, "kernel", PROC_NAME);
    current_idx = 0;
}

proc_t *proc_current(void)
{
    return &table[current_idx];
}

proc_t *proc_get(int pid)
{
    for (int i = 0; i < MAX_PROCS; i++)
        if (table[i].state != PROC_UNUSED && table[i].pid == pid)
            return &table[i];
    return 0;
}

proc_t *proc_alloc(const char *name)
{
    for (int i = 1; i < MAX_PROCS; i++) {
        if (table[i].state != PROC_UNUSED)
            continue;
        proc_t *p = &table[i];
        memset(p, 0, sizeof(*p));
        p->pid = next_pid++;
        p->ppid = proc_current()->pid;
        p->state = PROC_READY;
        strncpy_z(p->name, name ? name : "proc", PROC_NAME);
        return p;
    }
    return 0;                       /* process table full */
}

void proc_free(proc_t *p)
{
    if (!p || p->pid == 0)
        return;                     /* never free the kernel */
    if (p->dir && p->dir != vmm_kernel_dir())
        vmm_destroy_dir(p->dir);
    memset(p, 0, sizeof(*p));
    p->state = PROC_UNUSED;
}

int proc_count(void)
{
    int n = 0;
    for (int i = 0; i < MAX_PROCS; i++)
        if (table[i].state != PROC_UNUSED)
            n++;
    return n;
}

const char *proc_state_name(proc_state_t s)
{
    switch (s) {
    case PROC_READY:   return "ready";
    case PROC_RUNNING: return "running";
    case PROC_ZOMBIE:  return "zombie";
    default:           return "unused";
    }
}

void proc_list(void)
{
    kprint("  pid  ppid  state    name        address space\n");
    for (int i = 0; i < MAX_PROCS; i++) {
        proc_t *p = &table[i];
        if (p->state == PROC_UNUSED)
            continue;
        kprint("  ");
        kprint_dec((uint32_t)p->pid);
        kprint("    ");
        kprint_dec((uint32_t)p->ppid);
        kprint("    ");
        kprint(proc_state_name(p->state));
        kprint("  ");
        kprint(p->name);
        kprint("  ");
        kprint_hex((uint32_t)p->dir);
        kprint_char('\n');
    }
    kprint("  frames in use: ");
    kprint_dec(pmm_used_frames());
    kprint(" of ");
    kprint_dec(pmm_total_frames());
    kprint_char('\n');
}
