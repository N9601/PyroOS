/* ============================================================================
 *  PyroOS  -  kernel heap (dynamic memory)
 * ==========================================================================*/
#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

void  heap_install(void);         /* set up the heap region */
void *kmalloc(uint32_t size);     /* allocate `size` bytes (0 if out of memory) */
void  kfree(void *ptr);           /* return a block to the heap */

#endif
