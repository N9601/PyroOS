/* ============================================================================
 *  PyroOS  -  paging (virtual memory)
 * ==========================================================================*/
#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

void paging_install(void);   /* identity-map the first 4 MB and enable paging */
uint32_t *paging_boot_directory(void);   /* the kernel page directory */

#endif
