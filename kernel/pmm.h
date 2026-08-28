/* ============================================================================
 *  PyroOS  -  physical memory manager
 * ----------------------------------------------------------------------------
 *  Hands out and reclaims 4 KB frames of real RAM. Everything above it (page
 *  tables, per-process address spaces, demand paging) needs a source of free
 *  physical pages, and this is it.
 * ==========================================================================*/
#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096u

void     pmm_install(uint32_t mem_bytes);   /* set up over a region of RAM */
uint32_t pmm_alloc(void);                   /* one frame, physical addr, 0 if none */
void     pmm_free(uint32_t phys);           /* give a frame back */
uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);

#endif
