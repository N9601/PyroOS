/* ============================================================================
 *  PyroOS  -  cooperative multitasking
 * ==========================================================================*/
#ifndef TASK_H
#define TASK_H

/* Cooperative demo: spawn several tasks that interleave via yielding, then
   return once they have all finished. */
void tasking_demo(void);

/* Preemptive demo: two tasks that never yield still both run, because the
   timer interrupt forcibly switches between them. */
void preempt_demo(void);

/* Called from the timer interrupt; drives preemptive switching when armed. */
void preempt_tick(void);

/* Yield the CPU to the next runnable task (used by the scheduler and by mutex
   and semaphore waits). */
void task_yield(void);

/* Run the synchronization demo: threads race on a shared counter without a
   lock, then safely with a mutex. */
void threads_demo(void);

#endif
