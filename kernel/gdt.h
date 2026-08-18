/* ============================================================================
 *  PyroOS  -  kernel GDT with user segments and a TSS
 * ==========================================================================*/
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* Segment selectors (index * 8, with the low bits for privilege level). */
#define KERNEL_CODE 0x08
#define KERNEL_DATA 0x10
#define USER_CODE   (0x18 | 3)   /* ring 3 */
#define USER_DATA   (0x20 | 3)   /* ring 3 */

void gdt_install(void);
void tss_set_stack(uint32_t esp0);   /* set the ring-0 stack for the next trap */

#endif
