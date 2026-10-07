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
   image (near USER_LOAD_ADDR) and the stack (below USER_STACK_TOP). The zone is
   identity-mapped, so sbrk hands out physical memory as it is; it only has to
   track the break and refuse to run past the end.

   That only works where the zone is real RAM, which on a PC is conventional
   memory below 0xA0000. Above it sit the VGA window (0xA0000 to 0xBFFFF, which
   includes the text screen at 0xB8000) and then option and BIOS ROM: writes
   there are dropped or land on the screen. So the heap is the 60 KB from
   0x90000 up to just below the extended BIOS data area at 0x9FC00, leaving
   the 64 KB from USER_LOAD_ADDR for the program image. */
#define USER_HEAP_BASE 0x00090000u
#define USER_HEAP_END  0x0009F000u

#endif
