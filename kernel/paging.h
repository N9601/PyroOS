/* ============================================================================
 *  PyroOS  -  paging (virtual memory)
 * ==========================================================================*/
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void paging_install(void);   /* identity-map the first 4 MB and enable paging */
uint32_t *paging_boot_directory(void);   /* the kernel page directory */

/* Flip the ring-3 writable bit on user-zone pages, so the ELF loader can make a
   read-only segment actually fault on write. Clamped to the user zone. */
void paging_set_user_writable(uint32_t addr, uint32_t len, int writable);
void paging_reset_user_zone(void);   /* back to all-writable, the load default */

#endif
