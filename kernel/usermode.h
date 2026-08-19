/* ============================================================================
 *  PyroOS  -  user mode (ring 3) launcher
 * ==========================================================================*/
#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

void run_user_program(void);   /* drop to ring 3, run the built-in demo, return */
void run_user_at(uint32_t entry); /* drop to ring 3 at an arbitrary entry point */
void user_exit(void);          /* called by SYS_EXIT to unwind back to kernel */

/* Where the kernel loads programs from disk before running them. Must match the
   base address in user/prog.ld. */
#define USER_LOAD_ADDR 0x00080000

#endif
