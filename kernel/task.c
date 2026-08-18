/* ============================================================================
 *  PyroOS  -  cooperative multitasking + round-robin scheduler
 * ----------------------------------------------------------------------------
 *  Each task has its own stack. task_yield() saves the current task and
 *  switches to the next active one via context_switch (switch.asm). Tasks run
 *  until they voluntarily yield -- "cooperative" scheduling. The caller (the
 *  shell) is task 0, so when all the worker tasks finish, control returns here.
 *
 *  A brand-new task's stack is hand-crafted to look exactly as if it had just
 *  called context_switch: four saved registers followed by a return address
 *  pointing at the task's entry function. The first switch into it therefore
 *  "returns" straight into that function.
 * ==========================================================================*/
#include "task.h"
#include "screen.h"

#include <stdint.h>

#define MAX_TASKS  8
#define STACK_SIZE 4096

typedef struct {
    uint32_t esp;       /* saved stack pointer */
    int      active;    /* 1 while the task still wants to run */
} task_t;

extern void context_switch(uint32_t *old_esp, uint32_t new_esp);

static task_t  tasks[MAX_TASKS];
static uint8_t stacks[MAX_TASKS][STACK_SIZE] __attribute__((aligned(16)));
static int     num_tasks = 0;
static int     current   = 0;

static int task_create(void (*entry)(void))
{
    int id = num_tasks++;

    uint32_t *sp = (uint32_t *)(stacks[id] + STACK_SIZE);
    *(--sp) = (uint32_t)entry;   /* return address -> the task body */
    *(--sp) = 0;                 /* ebp */
    *(--sp) = 0;                 /* ebx */
    *(--sp) = 0;                 /* esi */
    *(--sp) = 0;                 /* edi */

    tasks[id].esp = (uint32_t)sp;
    tasks[id].active = 1;
    return id;
}

static void task_yield(void)
{
    int prev = current;

    /* Look for the next active worker (indices 1..num_tasks-1). Task 0 is the
       scheduler and is skipped here; we only fall back to it when no worker
       is left to run. */
    for (int off = 1; off <= num_tasks; off++) {
        int cand = (prev + off) % num_tasks;
        if (cand == 0)
            continue;
        if (tasks[cand].active) {
            current = cand;
            context_switch(&tasks[prev].esp, tasks[cand].esp);
            return;
        }
    }

    /* No workers remain: hand control back to the scheduler task (0). */
    if (prev != 0) {
        current = 0;
        context_switch(&tasks[prev].esp, tasks[0].esp);
    }
}

static void task_exit(void)
{
    tasks[current].active = 0;
    task_yield();                /* leave for good; never returns here */
    for (;;)
        ;
}

/* --- three demo worker tasks --- */
static void worker(const char *label)
{
    for (int i = 0; i < 5; i++) {
        kprint(label);
        task_yield();            /* give the CPU to the next task */
    }
    task_exit();
}

static void task_a(void) { worker("A "); }
static void task_b(void) { worker("B "); }
static void task_c(void) { worker("C "); }

void tasking_demo(void)
{
    num_tasks = 0;
    current   = 0;

    /* Task 0 is us (the caller): the scheduler. It is not part of the worker
       rotation; its esp gets filled in on the first switch. */
    tasks[0].active = 0;
    tasks[0].esp    = 0;
    num_tasks       = 1;

    task_create(task_a);
    task_create(task_b);
    task_create(task_c);

    kprint("  ");
    task_yield();                /* dive into the workers; returns when done */
    kprint("\n  all tasks finished, back in the shell.\n");
}
