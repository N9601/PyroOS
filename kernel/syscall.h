/* ============================================================================
 *  PyroOS  -  system calls (int 0x80)
 * ==========================================================================*/
#ifndef SYSCALL_H
#define SYSCALL_H

/* Syscall numbers (passed in eax). */
#define SYS_WRITE   0    /* ebx = pointer to a null-terminated string */
#define SYS_UPTIME  1    /* returns timer ticks in eax */
#define SYS_EXIT    2    /* leave user mode, return to the kernel */
#define SYS_READ    3    /* block until a key is pressed; returns the char in eax */
#define SYS_SLEEP   4    /* ebx = ticks to sleep (50 ticks = 1 second) */

void syscall_install(void);   /* register the int 0x80 gate */

#endif
