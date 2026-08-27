/* ============================================================================
 *  PyroOS  -  synchronization primitives
 * ----------------------------------------------------------------------------
 *  Spinlocks, mutexes, and counting semaphores for guarding critical sections
 *  against concurrent access by kernel threads.
 * ==========================================================================*/
#ifndef SYNC_H
#define SYNC_H

typedef struct { volatile int locked; } spinlock_t;
typedef struct { volatile int locked; } mutex_t;
typedef struct { volatile int count;  } semaphore_t;

/* Spinlock: busy-waits. Use for very short critical sections. */
void spin_init(spinlock_t *s);
void spin_lock(spinlock_t *s);
void spin_unlock(spinlock_t *s);

/* Mutex: yields to the lock holder while waiting, instead of busy-spinning. */
void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

/* Counting semaphore. */
void sem_init(semaphore_t *s, int count);
void sem_wait(semaphore_t *s);
void sem_post(semaphore_t *s);

#endif
