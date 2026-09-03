/* ============================================================================
 *  PyroOS  -  user mode (ring 3) launcher
 * ==========================================================================*/
#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

void run_user_program(void);   /* drop to ring 3, run the built-in demo, return */
void run_user_at(uint32_t entry); /* drop to ring 3 at an arbitrary entry point */
void run_user_at_sp(uint32_t entry, uint32_t esp); /* ...with a prepared stack */
void user_exit(void);          /* called by SYS_EXIT to unwind back to kernel */

/* Where the kernel loads programs from disk before running them. Must match the
   base address in user/prog.ld. */
#define USER_LOAD_ADDR 0x00080000

/* The ring-3 stack lives at the top of the user zone and grows down. Together
   with USER_LOAD_ADDR this bounds the region a program may occupy. */
#define USER_STACK_TOP 0x000F0000u

/* The user heap: a slice of the already-mapped user zone between the program
   image (near USER_LOAD_ADDR) and the stack (below USER_STACK_TOP). Because the
   whole zone is identity-mapped and present, sbrk hands out addresses that are
   already backed by RAM; it only has to track the break and refuse to run past
   the end. 256 KB, clear of both code and stack. */
#define USER_HEAP_BASE 0x000A0000u
#define USER_HEAP_END  0x000E0000u

#endif
