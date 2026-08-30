/* ============================================================================
 *  PyroOS  -  process model
 * ----------------------------------------------------------------------------
 *  A process is an address space plus the bookkeeping that lets the kernel
 *  create it, run it, wait for it and reclaim it. This is the structure the
 *  UNIX calls operate on: fork duplicates one, exec replaces its image, exit
 *  ends it, and wait collects the result.
 * ==========================================================================*/
#ifndef PROC_H
#define PROC_H

#include <stdint.h>
#include "vmm.h"

#define MAX_PROCS   16
#define PROC_NAME   16

typedef enum {
    PROC_UNUSED = 0,   /* slot is free */
    PROC_READY,        /* runnable, not currently running */
    PROC_RUNNING,      /* on the CPU now */
    PROC_ZOMBIE        /* finished, exit status not yet collected by the parent */
} proc_state_t;

typedef struct {
    int          pid;
    int          ppid;              /* parent, so wait knows who to notify */
    proc_state_t state;
    page_dir_t   dir;               /* its address space */
    uint32_t     entry;             /* where execution starts */
    uint32_t     brk;               /* top of its allocated image, for growth */
    int          exit_code;
    char         name[PROC_NAME];
} proc_t;

void    proc_install(void);
proc_t *proc_current(void);
proc_t *proc_get(int pid);
proc_t *proc_alloc(const char *name);      /* an empty slot with a new pid */
void    proc_free(proc_t *p);              /* tear down the address space */

int     proc_count(void);
void    proc_list(void);                   /* print the table */
const char *proc_state_name(proc_state_t s);

#endif
