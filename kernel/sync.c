/* ============================================================================
 *  PyroOS  -  synchronization primitives
 * ----------------------------------------------------------------------------
 *  Built on the x86 atomic exchange (xchg is implicitly locked), so lock
 *  acquisition is a single atomic operation. Mutexes and semaphores yield the
 *  CPU while waiting so the lock holder can run and release.
 * ==========================================================================*/
#include "sync.h"
#include "task.h"

/* Atomically set *p to v and return the previous value. xchg carries an
   implicit lock prefix, so this is a safe test-and-set. */
static inline int atomic_xchg(volatile int *p, int v)
{
    __asm__ volatile("xchg %0, %1" : "+r"(v), "+m"(*p) :: "memory");
    return v;
}

/* --- spinlock --- */
void spin_init(spinlock_t *s)   { s->locked = 0; }
void spin_lock(spinlock_t *s)
{
    while (atomic_xchg(&s->locked, 1))
        __asm__ volatile("pause");          /* hint: spin-wait loop */
}
void spin_unlock(spinlock_t *s) { __asm__ volatile("" ::: "memory"); s->locked = 0; }

/* --- mutex --- */
void mutex_init(mutex_t *m) { m->locked = 0; }
void mutex_lock(mutex_t *m)
{
    while (atomic_xchg(&m->locked, 1))
        task_yield();                        /* let the holder run and release */
}
void mutex_unlock(mutex_t *m) { __asm__ volatile("" ::: "memory"); m->locked = 0; }

/* --- counting semaphore ---
   The check-and-decrement runs with interrupts disabled so it is atomic even
   under preemption. */
void sem_init(semaphore_t *s, int count) { s->count = count; }
void sem_wait(semaphore_t *s)
{
    for (;;) {
        __asm__ volatile("cli");
        if (s->count > 0) {
            s->count--;
            __asm__ volatile("sti");
            return;
        }
        __asm__ volatile("sti");
        task_yield();
    }
}
void sem_post(semaphore_t *s)
{
    __asm__ volatile("cli");
    s->count++;
    __asm__ volatile("sti");
}
