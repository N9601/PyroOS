/* ============================================================================
 *  PyroOS  -  demand paging
 * ----------------------------------------------------------------------------
 *  A process can be promised a large region of address space without any
 *  physical memory being committed to it. The frame is allocated only when the
 *  process actually touches the page, which is what lets a system promise far
 *  more memory than it has.
 * ==========================================================================*/
#ifndef DEMAND_H
#define DEMAND_H

#include <stdint.h>
#include "vmm.h"

/* Promise a region: addresses become valid, but no frames are committed. */
void demand_region(page_dir_t dir, uint32_t start, uint32_t end, uint32_t flags);
void demand_clear(void);

/* Called from the page fault handler. Returns 1 if the fault was resolved. */
int  demand_handle(uint32_t addr, uint32_t err_code);

uint32_t demand_faults_served(void);

#endif
