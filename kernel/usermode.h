/* ============================================================================
 *  PyroOS  -  user mode (ring 3) launcher
 * ==========================================================================*/
#ifndef USERMODE_H
#define USERMODE_H

void run_user_program(void);   /* drop to ring 3, run the demo program, return */
void user_exit(void);          /* called by SYS_EXIT to unwind back to kernel */

#endif
