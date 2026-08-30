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

/* ============================================================================
 *  The UNIX process calls
 * ----------------------------------------------------------------------------
 *  fork duplicates a process, exit ends one, wait lets the parent collect the
 *  result. The three are a set: without wait, a finished process would either
 *  vanish before its parent could read its exit code, or sit in the table
 *  forever. The zombie state is what resolves that -- the process is dead, but
 *  its slot is held until someone asks how it died.
 * ==========================================================================*/

int proc_fork(void)
{
    proc_t *parent = proc_current();

    proc_t *child = proc_alloc(parent->name);
    if (!child)
        return -1;                          /* process table full */

    /* The child gets its own copy of the parent's memory. Same contents, same
       virtual addresses, different physical frames. */
    child->dir = vmm_clone_dir(parent->dir);
    if (!child->dir) {
        child->state = PROC_UNUSED;         /* nothing was allocated to free */
        return -1;
    }

    child->ppid  = parent->pid;
    child->entry = parent->entry;
    child->brk   = parent->brk;
    child->state = PROC_READY;
    return child->pid;
}

void proc_exit(int code)
{
    proc_t *p = proc_current();
    if (p->pid == 0)
        return;                             /* the kernel does not exit */

    p->exit_code = code;
    p->state = PROC_ZOMBIE;

    /* If the process calling exit is the one on the CPU, its page directory is
       loaded in CR3 right now. Freeing memory we are standing on would leave
       the CPU walking reclaimed page tables, so fall back to the kernel address
       space first. This is the kernel equivalent of stepping off the plank
       before sawing it. */
    if (p == proc_current()) {
        vmm_switch(vmm_kernel_dir());
        current_idx = 0;
        table[0].state = PROC_RUNNING;
    }

    /* Its memory is no longer needed; only the exit status has to survive
       until the parent reaps it. Release the address space now rather than
       holding megabytes hostage to a parent that may be slow to call wait. */
    if (p->dir && p->dir != vmm_kernel_dir()) {
        vmm_destroy_dir(p->dir);
        p->dir = 0;
    }

    /* Orphans are re-parented to the kernel, which reaps unconditionally, so
       a dead parent cannot strand its children in the table forever. */
    for (int i = 1; i < MAX_PROCS; i++)
        if (table[i].state != PROC_UNUSED && table[i].ppid == p->pid)
            table[i].ppid = 0;
}

int proc_wait(int *status)
{
    proc_t *me = proc_current();

    for (int i = 1; i < MAX_PROCS; i++) {
        proc_t *c = &table[i];
        if (c->state != PROC_ZOMBIE || c->ppid != me->pid)
            continue;
        int pid = c->pid;
        if (status)
            *status = c->exit_code;
        proc_free(c);                       /* the slot is finally released */
        return pid;
    }
    return -1;                              /* no finished child of ours */
}

int proc_switch_to(int pid)
{
    proc_t *p = proc_get(pid);
    if (!p || p->state == PROC_ZOMBIE || !p->dir)
        return -1;

    proc_t *old = proc_current();
    if (old->state == PROC_RUNNING)
        old->state = PROC_READY;

    current_idx = (int)(p - table);
    p->state = PROC_RUNNING;
    vmm_switch(p->dir);                     /* the actual change of world */
    return 0;
}
